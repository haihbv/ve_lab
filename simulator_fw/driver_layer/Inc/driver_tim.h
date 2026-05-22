/**
 * @file    driver_tim.h
 * @brief   Timer driver interface.
 * @details Provides APIs to setup, enable, disable, and control Timers.
 */

#ifndef DRIVER_TIM_H
#define DRIVER_TIM_H

#ifdef __cplusplus
extern "C"
{
#endif

/* Includes ----------------------------------------------------------------*/
#include <stdint.h>
#include <stddef.h>

#include "stm32f10x.h"
#include "driver_common.h"

/* Exported types ----------------------------------------------------------*/

/**
 * @brief Timer channel selection.
 */
typedef enum
{
    TIMER_CH1 = 0, /**< Timer Channel 1 */
    TIMER_CH2,     /**< Timer Channel 2 */
    TIMER_CH3,     /**< Timer Channel 3 */
    TIMER_CH4      /**< Timer Channel 4 */
} timer_channel_t;

/**
 * @brief Timer event types.
 */
typedef enum
{
    TIMER_EVT_UPDATE = 0, /**< Counter update/overflow event */
    TIMER_EVT_CAPTURE,    /**< Input capture event */
    TIMER_EVT_COMPARE,    /**< Output compare event */
    TIMER_EVT_DMA_DONE,   /**< DMA transfer complete event */
    TIMER_EVT_ERROR       /**< Timer error event */
} timer_event_t;

/**
 * @brief Timer event object.
 */
typedef struct
{
    timer_event_t   type;    /**< Event type */
    timer_channel_t channel; /**< Channel associated with the event */
    uint32_t        value;   /**< Capture or counter value */
    uint32_t        error;   /**< Error flags */
} timer_evt_t;

/**
 * @brief Timer event callback function type.
 */
typedef void (*timer_event_cb)(void *ctx, const timer_evt_t *evt);

typedef struct timer timer_t;

/**
 * @brief Timer driver object.
 */
struct timer
{
    TIM_TypeDef   *instance; /**< TIM peripheral instance */
    timer_event_cb callback; /**< Event callback function */
    void          *ctx;      /**< Event callback user context */
};

/* Public API --------------------------------------------------------------*/

/**
 * @brief Setup timer driver object.
 *
 * @param dev Pointer to timer driver object.
 * @param instance TIM peripheral instance.
 *
 * @return DRIVER_OK if successful, otherwise an error code.
 */
driver_status_t timer_setup(timer_t *dev, TIM_TypeDef *instance);

/**
 * @brief Enable timer counter.
 *
 * @param dev Pointer to timer driver object.
 *
 * @return DRIVER_OK if successful, otherwise an error code.
 */
driver_status_t timer_enable(timer_t *dev);

/**
 * @brief Disable timer counter.
 *
 * @param dev Pointer to timer driver object.
 *
 * @return DRIVER_OK if successful, otherwise an error code.
 */
driver_status_t timer_disable(timer_t *dev);

/**
 * @brief Register timer event callback.
 *
 * @param dev Pointer to timer driver object.
 * @param cb Event callback function.
 * @param ctx User callback context.
 */
void timer_register_callback(timer_t *dev, timer_event_cb cb, void *ctx);

/**
 * @brief Handle timer interrupt.
 *
 * @note Must be called from the corresponding TIM IRQ handler.
 *
 * @param dev Pointer to timer driver object.
 */
void timer_handle_irq(timer_t *dev);

/**
 * @brief Initialize basic timer frequency and period.
 *
 * @param dev Pointer to timer driver object.
 * @param prescaler Timer prescaler value.
 * @param period Timer auto-reload period.
 *
 * @return DRIVER_OK if successful, otherwise an error code.
 */
driver_status_t timer_base_setup(timer_t *dev, uint16_t prescaler, uint32_t period);

/**
 * @brief Set timer auto-reload period.
 *
 * @param dev Pointer to timer driver object.
 * @param period Auto-reload value.
 *
 * @return DRIVER_OK if successful, otherwise an error code.
 */
driver_status_t timer_set_period(timer_t *dev, uint32_t period);

/**
 * @brief Get current timer counter value.
 *
 * @param dev Pointer to timer driver object.
 * @param counter Pointer to store the counter value.
 *
 * @return DRIVER_OK if successful, otherwise an error code.
 */
driver_status_t timer_get_counter(timer_t *dev, uint32_t *counter);

/**
 * @brief Set current timer counter value.
 *
 * @param dev Pointer to timer driver object.
 * @param counter New counter value.
 *
 * @return DRIVER_OK if successful, otherwise an error code.
 */
driver_status_t timer_set_counter(timer_t *dev, uint32_t counter);

/**
 * @brief Reset timer counter to zero.
 *
 * @param dev Pointer to timer driver object.
 *
 * @return DRIVER_OK if successful, otherwise an error code.
 */
driver_status_t timer_reset_counter(timer_t *dev);

/**
 * @brief Enable timer update interrupt.
 *
 * @param dev Pointer to timer driver object.
 *
 * @return DRIVER_OK if successful, otherwise an error code.
 */
driver_status_t timer_enable_update_irq(timer_t *dev);

/**
 * @brief Disable timer update interrupt.
 *
 * @param dev Pointer to timer driver object.
 *
 * @return DRIVER_OK if successful, otherwise an error code.
 */
driver_status_t timer_disable_update_irq(timer_t *dev);

/**
 * @brief Setup PWM mode on a selected channel.
 *
 * @param dev Pointer to timer driver object.
 * @param channel Selected timer channel.
 * @param prescaler Timer prescaler value.
 * @param period Timer auto-reload period.
 * @param pulse PWM pulse width (duty).
 * @param polarity PWM output polarity (TIM_OCPolarity_High/Low).
 *
 * @return DRIVER_OK if successful, otherwise an error code.
 */
driver_status_t timer_pwm_setup(timer_t *dev, 
                                timer_channel_t channel, 
                                uint16_t prescaler,
                                uint32_t period,
                                uint16_t pulse,
                                uint16_t polarity);

/**
 * @brief Start PWM output on a selected channel.
 *
 * @param dev Pointer to timer driver object.
 * @param channel Selected timer channel.
 *
 * @return DRIVER_OK if successful, otherwise an error code.
 */
driver_status_t timer_pwm_enable(timer_t *dev, timer_channel_t channel);

/**
 * @brief Stop PWM output on a selected channel.
 *
 * @param dev Pointer to timer driver object.
 * @param channel Selected timer channel.
 *
 * @return DRIVER_OK if successful, otherwise an error code.
 */
driver_status_t timer_pwm_disable(timer_t *dev, timer_channel_t channel);

/**
 * @brief Set PWM duty cycle (0-100%).
 *
 * @param dev Pointer to timer driver object.
 * @param channel Selected timer channel.
 * @param duty Duty cycle percentage.
 *
 * @return DRIVER_OK if successful, otherwise an error code.
 */
driver_status_t timer_pwm_set_duty(timer_t *dev, timer_channel_t channel, uint16_t duty);

/**
 * @brief Set PWM pulse width in timer ticks.
 *
 * @param dev Pointer to timer driver object.
 * @param channel Selected timer channel.
 * @param pulse Pulse width value.
 *
 * @return DRIVER_OK if successful, otherwise an error code.
 */
driver_status_t timer_pwm_set_pulse(timer_t *dev, timer_channel_t channel, uint16_t pulse);

/**
 * @brief Setup input capture mode on a selected channel.
 *
 * @param dev Pointer to timer driver object.
 * @param channel Selected timer channel.
 * @param prescaler Timer prescaler value.
 * @param period Timer auto-reload period.
 * @param polarity Capture edge polarity (TIM_ICPolarity_Rising/Falling).
 * @param filter Input filter configuration (0x0-0xF).
 *
 * @return DRIVER_OK if successful, otherwise an error code.
 */
driver_status_t timer_ic_setup(timer_t *dev, 
                               timer_channel_t channel, 
                               uint16_t prescaler,
                               uint32_t period,
                               uint16_t polarity,
                               uint8_t filter);

/**
 * @brief Start input capture on a selected channel.
 *
 * @param dev Pointer to timer driver object.
 * @param channel Selected timer channel.
 *
 * @return DRIVER_OK if successful, otherwise an error code.
 */
driver_status_t timer_ic_enable(timer_t *dev, timer_channel_t channel);

/**
 * @brief Stop input capture on a selected channel.
 *
 * @param dev Pointer to timer driver object.
 * @param channel Selected timer channel.
 *
 * @return DRIVER_OK if successful, otherwise an error code.
 */
driver_status_t timer_ic_disable(timer_t *dev, timer_channel_t channel);

/**
 * @brief Get latest captured value from a selected channel.
 *
 * @param dev Pointer to timer driver object.
 * @param channel Selected timer channel.
 * @param capture Pointer to store the captured value.
 *
 * @return DRIVER_OK if successful, otherwise an error code.
 */
driver_status_t timer_ic_get_capture(timer_t *dev, timer_channel_t channel, uint32_t *capture);

/**
 * @brief Set input capture edge polarity at runtime.
 *
 * @param dev Pointer to timer driver object.
 * @param channel Selected timer channel.
 * @param polarity TIM_ICPolarity_Rising or TIM_ICPolarity_Falling.
 *
 * @return DRIVER_OK if successful, otherwise an error code.
 */
driver_status_t timer_ic_set_polarity(timer_t *dev, timer_channel_t channel, uint16_t polarity);

/**
 * @brief Setup output compare mode on a selected channel.
 *
 * @param dev Pointer to timer driver object.
 * @param channel Selected timer channel.
 * @param prescaler Timer prescaler value.
 * @param period Timer auto-reload period.
 * @param pulse Compare value.
 * @param oc_mode Output compare mode (TIM_OCMode_PWM1/Toggle/etc).
 *
 * @return DRIVER_OK if successful, otherwise an error code.
 */
driver_status_t timer_oc_setup(timer_t *dev, 
                               timer_channel_t channel, 
                               uint16_t prescaler,
                               uint32_t period,
                               uint16_t pulse,
                               uint16_t oc_mode);

/**
 * @brief Start output compare on a selected channel.
 *
 * @param dev Pointer to timer driver object.
 * @param channel Selected timer channel.
 *
 * @return DRIVER_OK if successful, otherwise an error code.
 */
driver_status_t timer_oc_enable(timer_t *dev, timer_channel_t channel);

/**
 * @brief Stop output compare on a selected channel.
 *
 * @param dev Pointer to timer driver object.
 * @param channel Selected timer channel.
 *
 * @return DRIVER_OK if successful, otherwise an error code.
 */
driver_status_t timer_oc_disable(timer_t *dev, timer_channel_t channel);

/**
 * @brief Set output compare value in timer ticks.
 *
 * @param dev Pointer to timer driver object.
 * @param channel Selected timer channel.
 * @param compare New compare value.
 *
 * @return DRIVER_OK if successful, otherwise an error code.
 */
driver_status_t timer_oc_set_compare(timer_t *dev, timer_channel_t channel, uint16_t compare);

#ifdef __cplusplus
}
#endif

#endif /* DRIVER_TIM_H */
