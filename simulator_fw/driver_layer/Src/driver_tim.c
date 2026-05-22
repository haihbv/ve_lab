/**
 * @file    driver_tim.c
 * @brief   Timer driver implementation.
 */

#include "driver_tim.h"
#include <string.h>

/* Private function prototypes ---------------------------------------------*/

static int prv_channel_is_valid(timer_channel_t channel);
static uint16_t prv_channel_to_spl(timer_channel_t channel);
static uint16_t prv_channel_to_interrupt(timer_channel_t channel);

static uint32_t prv_read_capture(TIM_TypeDef *instance, timer_channel_t channel);
static void prv_write_compare(TIM_TypeDef *instance, timer_channel_t channel, uint16_t value);

static uint8_t prv_get_ic_filter(TIM_TypeDef *instance, timer_channel_t channel);
static timer_event_t prv_get_cc_event_type(TIM_TypeDef *instance, timer_channel_t channel);

static int prv_is_advanced(TIM_TypeDef *instance);

static void prv_emit_event(timer_t *dev,
                           timer_event_t type,
                           timer_channel_t channel,
                           uint32_t value,
                           uint32_t error);

static void prv_handle_cc_irq(timer_t *dev, uint16_t interrupt, timer_channel_t channel);
static driver_status_t prv_base_configure(timer_t *dev, uint16_t prescaler, uint32_t period);

/* Public functions --------------------------------------------------------*/

driver_status_t timer_setup(timer_t *dev, TIM_TypeDef *instance)
{
    if ((dev == NULL) || (instance == NULL))
    {
        return DRIVER_ERR_INVALID_ARG;
    }

    memset(dev, 0, sizeof(*dev));
    dev->instance = instance;

    return DRIVER_OK;
}

void timer_register_callback(timer_t *dev, timer_event_cb cb, void *ctx)
{
    if (dev == NULL) return;
    dev->callback = cb;
    dev->ctx      = ctx;
}

void timer_handle_irq(timer_t *dev)
{
    if ((dev == NULL) || (dev->instance == NULL)) return;

    TIM_TypeDef *instance = dev->instance;

    /* Update interrupt */
    if (TIM_GetITStatus(instance, TIM_IT_Update) != RESET)
    {
        TIM_ClearITPendingBit(instance, TIM_IT_Update);
        prv_emit_event(dev, TIMER_EVT_UPDATE, TIMER_CH1, instance->CNT, 0u);
    }

    /* Capture/Compare interrupts */
    prv_handle_cc_irq(dev, TIM_IT_CC1, TIMER_CH1);
    prv_handle_cc_irq(dev, TIM_IT_CC2, TIMER_CH2);
    prv_handle_cc_irq(dev, TIM_IT_CC3, TIMER_CH3);
    prv_handle_cc_irq(dev, TIM_IT_CC4, TIMER_CH4);
}

driver_status_t timer_base_setup(timer_t *dev, uint16_t prescaler, uint32_t period)
{
    return prv_base_configure(dev, prescaler, period);
}

driver_status_t timer_enable(timer_t *dev)
{
    if ((dev == NULL) || (dev->instance == NULL)) return DRIVER_ERR_INVALID_ARG;
    TIM_Cmd(dev->instance, ENABLE);
    return DRIVER_OK;
}

driver_status_t timer_disable(timer_t *dev)
{
    if ((dev == NULL) || (dev->instance == NULL)) return DRIVER_ERR_INVALID_ARG;
    TIM_Cmd(dev->instance, DISABLE);
    return DRIVER_OK;
}

driver_status_t timer_set_period(timer_t *dev, uint32_t period)
{
    if ((dev == NULL) || (dev->instance == NULL)) return DRIVER_ERR_INVALID_ARG;
    dev->instance->ARR = period;
    return DRIVER_OK;
}

driver_status_t timer_get_counter(timer_t *dev, uint32_t *counter)
{
    if ((dev == NULL) || (dev->instance == NULL) || (counter == NULL)) return DRIVER_ERR_INVALID_ARG;
    *counter = dev->instance->CNT;
    return DRIVER_OK;
}

driver_status_t timer_set_counter(timer_t *dev, uint32_t counter)
{
    if ((dev == NULL) || (dev->instance == NULL)) return DRIVER_ERR_INVALID_ARG;
    dev->instance->CNT = counter;
    return DRIVER_OK;
}

driver_status_t timer_reset_counter(timer_t *dev)
{
    return timer_set_counter(dev, 0u);
}

driver_status_t timer_enable_update_irq(timer_t *dev)
{
    if ((dev == NULL) || (dev->instance == NULL)) return DRIVER_ERR_INVALID_ARG;
    TIM_ClearITPendingBit(dev->instance, TIM_IT_Update);
    TIM_ITConfig(dev->instance, TIM_IT_Update, ENABLE);
    return DRIVER_OK;
}

driver_status_t timer_disable_update_irq(timer_t *dev)
{
    if ((dev == NULL) || (dev->instance == NULL)) return DRIVER_ERR_INVALID_ARG;
    TIM_ITConfig(dev->instance, TIM_IT_Update, DISABLE);
    return DRIVER_OK;
}

driver_status_t timer_pwm_setup(timer_t *dev, 
                                timer_channel_t channel, 
                                uint16_t prescaler,
                                uint32_t period,
                                uint16_t pulse,
                                uint16_t polarity)
{
    TIM_OCInitTypeDef tim_init;
    if (!prv_channel_is_valid(channel)) return DRIVER_ERR_INVALID_ARG;
    if (prv_base_configure(dev, prescaler, period) != DRIVER_OK) return DRIVER_ERR_INVALID_ARG;

    TIM_OCStructInit(&tim_init);
    tim_init.TIM_OCMode      = TIM_OCMode_PWM1;
    tim_init.TIM_OutputState = TIM_OutputState_Enable;
    tim_init.TIM_Pulse       = pulse;
    tim_init.TIM_OCPolarity  = polarity;

    switch (channel)
    {
        case TIMER_CH1: TIM_OC1Init(dev->instance, &tim_init); TIM_OC1PreloadConfig(dev->instance, TIM_OCPreload_Enable); break;
        case TIMER_CH2: TIM_OC2Init(dev->instance, &tim_init); TIM_OC2PreloadConfig(dev->instance, TIM_OCPreload_Enable); break;
        case TIMER_CH3: TIM_OC3Init(dev->instance, &tim_init); TIM_OC3PreloadConfig(dev->instance, TIM_OCPreload_Enable); break;
        case TIMER_CH4: TIM_OC4Init(dev->instance, &tim_init); TIM_OC4PreloadConfig(dev->instance, TIM_OCPreload_Enable); break;
        default: return DRIVER_ERR_INVALID_ARG;
    }
    TIM_ARRPreloadConfig(dev->instance, ENABLE);
    return DRIVER_OK;
}

driver_status_t timer_pwm_enable(timer_t *dev, timer_channel_t channel)
{
    if ((dev == NULL) || (dev->instance == NULL) || !prv_channel_is_valid(channel)) return DRIVER_ERR_INVALID_ARG;
    TIM_CCxCmd(dev->instance, prv_channel_to_spl(channel), ENABLE);
    if (prv_is_advanced(dev->instance)) TIM_CtrlPWMOutputs(dev->instance, ENABLE);
    TIM_Cmd(dev->instance, ENABLE);
    return DRIVER_OK;
}

driver_status_t timer_pwm_disable(timer_t *dev, timer_channel_t channel)
{
    if ((dev == NULL) || (dev->instance == NULL) || !prv_channel_is_valid(channel)) return DRIVER_ERR_INVALID_ARG;
    TIM_CCxCmd(dev->instance, prv_channel_to_spl(channel), DISABLE);
    return DRIVER_OK;
}

driver_status_t timer_pwm_set_duty(timer_t *dev, timer_channel_t channel, uint16_t duty)
{
    if ((dev == NULL) || (dev->instance == NULL) || !prv_channel_is_valid(channel) || (duty > 100u)) return DRIVER_ERR_INVALID_ARG;
    uint32_t auto_reload_value = dev->instance->ARR + 1u;
    uint32_t pulse_value       = (auto_reload_value * duty) / 100u;
    return timer_pwm_set_pulse(dev, channel, (uint16_t)pulse_value);
}

driver_status_t timer_pwm_set_pulse(timer_t *dev, timer_channel_t channel, uint16_t pulse)
{
    if ((dev == NULL) || (dev->instance == NULL) || !prv_channel_is_valid(channel)) return DRIVER_ERR_INVALID_ARG;
    prv_write_compare(dev->instance, channel, pulse);
    return DRIVER_OK;
}

driver_status_t timer_ic_setup(timer_t *dev, 
                               timer_channel_t channel, 
                               uint16_t prescaler,
                               uint32_t period,
                               uint16_t polarity,
                               uint8_t filter)
{
    TIM_ICInitTypeDef tim_init;
    if (!prv_channel_is_valid(channel)) return DRIVER_ERR_INVALID_ARG;
    if (prv_base_configure(dev, prescaler, period) != DRIVER_OK) return DRIVER_ERR_INVALID_ARG;

    TIM_ICStructInit(&tim_init);
    tim_init.TIM_Channel     = prv_channel_to_spl(channel);
    tim_init.TIM_ICPolarity  = polarity;
    tim_init.TIM_ICSelection = TIM_ICSelection_DirectTI;
    tim_init.TIM_ICPrescaler = TIM_ICPSC_DIV1;
    tim_init.TIM_ICFilter    = filter;

    TIM_ICInit(dev->instance, &tim_init);
    TIM_ClearITPendingBit(dev->instance, prv_channel_to_interrupt(channel));
    return DRIVER_OK;
}

driver_status_t timer_ic_enable(timer_t *dev, timer_channel_t channel)
{
    if ((dev == NULL) || (dev->instance == NULL) || !prv_channel_is_valid(channel)) return DRIVER_ERR_INVALID_ARG;
    TIM_CCxCmd(dev->instance, prv_channel_to_spl(channel), ENABLE);
    TIM_ITConfig(dev->instance, prv_channel_to_interrupt(channel), ENABLE);
    TIM_Cmd(dev->instance, ENABLE);
    return DRIVER_OK;
}

driver_status_t timer_ic_disable(timer_t *dev, timer_channel_t channel)
{
    if ((dev == NULL) || (dev->instance == NULL) || !prv_channel_is_valid(channel)) return DRIVER_ERR_INVALID_ARG;
    TIM_ITConfig(dev->instance, prv_channel_to_interrupt(channel), DISABLE);
    TIM_CCxCmd(dev->instance, prv_channel_to_spl(channel), DISABLE);
    return DRIVER_OK;
}

driver_status_t timer_ic_get_capture(timer_t *dev, timer_channel_t channel, uint32_t *capture)
{
    if ((dev == NULL) || (dev->instance == NULL) || !prv_channel_is_valid(channel) || (capture == NULL)) return DRIVER_ERR_INVALID_ARG;
    *capture = prv_read_capture(dev->instance, channel);
    return DRIVER_OK;
}

driver_status_t timer_ic_set_polarity(timer_t *dev, timer_channel_t channel, uint16_t polarity)
{
    TIM_ICInitTypeDef tim_init;
    if ((dev == NULL) || (dev->instance == NULL) || !prv_channel_is_valid(channel)) return DRIVER_ERR_INVALID_ARG;
    if ((polarity != TIM_ICPolarity_Rising) && (polarity != TIM_ICPolarity_Falling)) return DRIVER_ERR_INVALID_ARG;

    TIM_ICStructInit(&tim_init);
    tim_init.TIM_Channel     = prv_channel_to_spl(channel);
    tim_init.TIM_ICPolarity  = polarity;
    tim_init.TIM_ICSelection = TIM_ICSelection_DirectTI;
    tim_init.TIM_ICPrescaler = TIM_ICPSC_DIV1;
    tim_init.TIM_ICFilter    = prv_get_ic_filter(dev->instance, channel);

    TIM_ICInit(dev->instance, &tim_init);
    TIM_ClearITPendingBit(dev->instance, prv_channel_to_interrupt(channel));
    return DRIVER_OK;
}

driver_status_t timer_oc_setup(timer_t *dev, 
                               timer_channel_t channel, 
                               uint16_t prescaler,
                               uint32_t period,
                               uint16_t pulse,
                               uint16_t oc_mode)
{
    TIM_OCInitTypeDef tim_init;
    if (!prv_channel_is_valid(channel)) return DRIVER_ERR_INVALID_ARG;
    if (prv_base_configure(dev, prescaler, period) != DRIVER_OK) return DRIVER_ERR_INVALID_ARG;

    TIM_OCStructInit(&tim_init);
    tim_init.TIM_OCMode      = oc_mode;
    tim_init.TIM_OutputState = TIM_OutputState_Enable;
    tim_init.TIM_Pulse       = pulse;
    tim_init.TIM_OCPolarity  = TIM_OCPolarity_High;

    switch (channel)
    {
        case TIMER_CH1: TIM_OC1Init(dev->instance, &tim_init); break;
        case TIMER_CH2: TIM_OC2Init(dev->instance, &tim_init); break;
        case TIMER_CH3: TIM_OC3Init(dev->instance, &tim_init); break;
        case TIMER_CH4: TIM_OC4Init(dev->instance, &tim_init); break;
        default: return DRIVER_ERR_INVALID_ARG;
    }
    TIM_ClearITPendingBit(dev->instance, prv_channel_to_interrupt(channel));
    return DRIVER_OK;
}

driver_status_t timer_oc_enable(timer_t *dev, timer_channel_t channel)
{
    if ((dev == NULL) || (dev->instance == NULL) || !prv_channel_is_valid(channel)) return DRIVER_ERR_INVALID_ARG;
    TIM_CCxCmd(dev->instance, prv_channel_to_spl(channel), ENABLE);
    TIM_ITConfig(dev->instance, prv_channel_to_interrupt(channel), ENABLE);
    TIM_Cmd(dev->instance, ENABLE);
    return DRIVER_OK;
}

driver_status_t timer_oc_disable(timer_t *dev, timer_channel_t channel)
{
    if ((dev == NULL) || (dev->instance == NULL) || !prv_channel_is_valid(channel)) return DRIVER_ERR_INVALID_ARG;
    TIM_ITConfig(dev->instance, prv_channel_to_interrupt(channel), DISABLE);
    TIM_CCxCmd(dev->instance, prv_channel_to_spl(channel), DISABLE);
    return DRIVER_OK;
}

driver_status_t timer_oc_set_compare(timer_t *dev, timer_channel_t channel, uint16_t compare)
{
    if ((dev == NULL) || (dev->instance == NULL) || !prv_channel_is_valid(channel)) return DRIVER_ERR_INVALID_ARG;
    prv_write_compare(dev->instance, channel, compare);
    return DRIVER_OK;
}

/* Private functions -------------------------------------------------------*/

static int prv_channel_is_valid(timer_channel_t channel) { return (channel >= TIMER_CH1) && (channel <= TIMER_CH4); }
static uint16_t prv_channel_to_spl(timer_channel_t channel) {
    switch (channel) {
        case TIMER_CH1: return TIM_Channel_1;
        case TIMER_CH2: return TIM_Channel_2;
        case TIMER_CH3: return TIM_Channel_3;
        case TIMER_CH4: return TIM_Channel_4;
        default: return 0u;
    }
}
static uint16_t prv_channel_to_interrupt(timer_channel_t channel) {
    switch (channel) {
        case TIMER_CH1: return TIM_IT_CC1;
        case TIMER_CH2: return TIM_IT_CC2;
        case TIMER_CH3: return TIM_IT_CC3;
        case TIMER_CH4: return TIM_IT_CC4;
        default: return 0u;
    }
}
static uint32_t prv_read_capture(TIM_TypeDef *instance, timer_channel_t channel) {
    switch (channel) {
        case TIMER_CH1: return TIM_GetCapture1(instance);
        case TIMER_CH2: return TIM_GetCapture2(instance);
        case TIMER_CH3: return TIM_GetCapture3(instance);
        case TIMER_CH4: return TIM_GetCapture4(instance);
        default: return 0u;
    }
}
static void prv_write_compare(TIM_TypeDef *instance, timer_channel_t channel, uint16_t value) {
    switch (channel) {
        case TIMER_CH1: TIM_SetCompare1(instance, value); break;
        case TIMER_CH2: TIM_SetCompare2(instance, value); break;
        case TIMER_CH3: TIM_SetCompare3(instance, value); break;
        case TIMER_CH4: TIM_SetCompare4(instance, value); break;
        default: break;
    }
}
static uint8_t prv_get_ic_filter(TIM_TypeDef *instance, timer_channel_t channel) {
    switch (channel) {
        case TIMER_CH1: return (uint8_t)((instance->CCMR1 & TIM_CCMR1_IC1F) >> 4);
        case TIMER_CH2: return (uint8_t)((instance->CCMR1 & TIM_CCMR1_IC2F) >> 12);
        case TIMER_CH3: return (uint8_t)((instance->CCMR2 & TIM_CCMR2_IC3F) >> 4);
        case TIMER_CH4: return (uint8_t)((instance->CCMR2 & TIM_CCMR2_IC4F) >> 12);
        default: return 0u;
    }
}
static timer_event_t prv_get_cc_event_type(TIM_TypeDef *instance, timer_channel_t channel) {
    uint32_t channel_mode;
    switch (channel) {
        case TIMER_CH1: channel_mode = instance->CCMR1 & TIM_CCMR1_CC1S; break;
        case TIMER_CH2: channel_mode = instance->CCMR1 & TIM_CCMR1_CC2S; break;
        case TIMER_CH3: channel_mode = instance->CCMR2 & TIM_CCMR2_CC3S; break;
        case TIMER_CH4: channel_mode = instance->CCMR2 & TIM_CCMR2_CC4S; break;
        default: return TIMER_EVT_ERROR;
    }
    return (channel_mode != 0u) ? TIMER_EVT_CAPTURE : TIMER_EVT_COMPARE;
}
static int prv_is_advanced(TIM_TypeDef *instance) { return (instance == TIM1); }
static void prv_emit_event(timer_t *dev, timer_event_t type, timer_channel_t channel, uint32_t value, uint32_t error) {
    if ((dev == NULL) || (dev->callback == NULL)) return;
    timer_evt_t event = { .type = type, .channel = channel, .value = value, .error = error };
    dev->callback(dev->ctx, &event);
}
static void prv_handle_cc_irq(timer_t *dev, uint16_t interrupt, timer_channel_t channel) {
    if (TIM_GetITStatus(dev->instance, interrupt) == RESET) return;
    TIM_ClearITPendingBit(dev->instance, interrupt);
    prv_emit_event(dev, prv_get_cc_event_type(dev->instance, channel), channel, prv_read_capture(dev->instance, channel), 0u);
}
static driver_status_t prv_base_configure(timer_t *dev, uint16_t prescaler, uint32_t period) {
    if ((dev == NULL) || (dev->instance == NULL)) return DRIVER_ERR_INVALID_ARG;
    TIM_TimeBaseInitTypeDef tim_base_init;
    TIM_TimeBaseStructInit(&tim_base_init);
    tim_base_init.TIM_Prescaler = (uint16_t)((prescaler > 0u) ? (prescaler - 1u) : 0u);
    tim_base_init.TIM_Period    = period;
    TIM_TimeBaseInit(dev->instance, &tim_base_init);
    return DRIVER_OK;
}
