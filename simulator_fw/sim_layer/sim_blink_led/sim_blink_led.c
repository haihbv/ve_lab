/**
 * @file    sim_blink_led.c
 * @brief   Self-contained blink LED timing checker.
 */

#include "sim_blink_led.h"

#include <string.h>

#include "FreeRTOS.h"
#include "FreeRTOSConfig.h"
#include "driver_gpio.h"
#include "driver_tim.h"
#include "driver_uart.h"
#include "semphr.h"
#include "stm32f10x_exti.h"
#include "stm32f10x_gpio.h"
#include "stm32f10x_rcc.h"
#include "task.h"

/* Private macros ----------------------------------------------------------*/

extern int printf(const char* fmt, ...);

#define BLINK_GPIO_PORT GPIOA
#define BLINK_GPIO_PIN GPIO_Pin_5
#define BLINK_GPIO_PORT_SOURCE GPIO_PortSourceGPIOA
#define BLINK_GPIO_PIN_SOURCE GPIO_PinSource5
#define BLINK_EXTI_LINE EXTI_Line5
#define BLINK_EXTI_IRQN EXTI9_5_IRQn
#define BLINK_TIM_INST TIM2

#define BLINK_TASK_STACK 256u
#define BLINK_TASK_PRIORITY (tskIDLE_PRIORITY + 2u)

#define BLINK_TIMER_HZ 10000u
#define BLINK_TICKS_PER_MS (BLINK_TIMER_HZ / 1000u)
#define BLINK_TIMER_PERIOD 0xFFFFu
#define BLINK_EXPECTED_MS 500u
#define BLINK_TOLERANCE_MS 10u

#ifndef SIM_BLINK_LOG_ENABLE
#define SIM_BLINK_LOG_ENABLE 0
#endif

#ifndef SIM_BLINK_EDGE_LOG_ENABLE
#define SIM_BLINK_EDGE_LOG_ENABLE 0
#endif

#if (SIM_BLINK_LOG_ENABLE == 1)
#define SIM_BLINK_LOG(...) printf(__VA_ARGS__)
#else
#define SIM_BLINK_LOG(...) \
    do {                   \
    } while (0)
#endif

#if (SIM_BLINK_EDGE_LOG_ENABLE == 1)
#define SIM_BLINK_EDGE_LOG(...) SIM_BLINK_LOG(__VA_ARGS__)
#else
#define SIM_BLINK_EDGE_LOG(...) \
    do {                        \
    } while (0)
#endif

/* Private types -----------------------------------------------------------*/

typedef enum
{
    BLINK_EDGE_WAIT_RISING = 0,
    BLINK_EDGE_WAIT_FALLING
} blink_edge_state_t;

typedef struct
{
    gpio_t input;
    timer_t timebase;
    SemaphoreHandle_t event_sem;
    SemaphoreHandle_t data_mutex;
    TaskHandle_t task_handle;
    sim_blink_led_last_t last;
} sim_blink_led_t;

/* Static objects ----------------------------------------------------------*/

static sim_blink_led_t g_blink_sim;

static volatile blink_edge_state_t g_edge_state;
static volatile uint32_t g_rise_tick;
static volatile uint32_t g_fall_tick;
static volatile uint32_t g_high_ms;
static volatile uint32_t g_low_ms;
static volatile uint8_t g_has_rise_tick;
static volatile uint8_t g_has_fall_tick;
static volatile uint8_t g_measure_ready;
static volatile uint8_t g_edge_event;

/* Private function prototypes ---------------------------------------------*/

static void prv_task(void* arg);
static void prv_config_clocks(void);
static void prv_config_exti(uint32_t trigger);
static uint32_t prv_get_tim2_clock_hz(void);
static uint32_t prv_timer_delta(uint32_t newer_tick, uint32_t older_tick);
static uint32_t prv_ticks_to_ms(uint32_t ticks);
static uint8_t prv_validate_duration(uint32_t duration_ms);
static void prv_store_last(sim_blink_led_t* sim, uint32_t high_ms, uint32_t low_ms, uint8_t valid);

/* Public functions --------------------------------------------------------*/

void sim_blink_led_init(void)
{
    uint16_t prescaler;
    uint32_t tim_clock_hz;

    memset(&g_blink_sim, 0, sizeof(g_blink_sim));

    prv_config_clocks();

    gpio_setup(&g_blink_sim.input, BLINK_GPIO_PORT, BLINK_GPIO_PIN, GPIO_Mode_IN_FLOATING, GPIO_Speed_50MHz);
    gpio_enable(&g_blink_sim.input);

    uart_setup(&g_uart1, USART1, 115200u);
    uart_enable(&g_uart1);

    tim_clock_hz = prv_get_tim2_clock_hz();
    prescaler = (uint16_t) (tim_clock_hz / BLINK_TIMER_HZ);
    timer_setup(&g_blink_sim.timebase, BLINK_TIM_INST);
    timer_base_setup(&g_blink_sim.timebase, prescaler, BLINK_TIMER_PERIOD);
    timer_reset_counter(&g_blink_sim.timebase);
    timer_enable(&g_blink_sim.timebase);

    SIM_BLINK_LOG("BLINK INIT PA5 EXTI5 TIM2=%luHz 0.1ms\n", (unsigned long) tim_clock_hz);

    g_blink_sim.event_sem = xSemaphoreCreateBinary();
    g_blink_sim.data_mutex = xSemaphoreCreateMutex();

    g_edge_state = BLINK_EDGE_WAIT_RISING;
    g_has_rise_tick = 0u;
    g_has_fall_tick = 0u;
    g_measure_ready = 0u;
    g_edge_event = 0u;

    prv_config_exti(EXTI_Trigger_Rising_Falling);
    NVIC_SetPriority(BLINK_EXTI_IRQN, configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY);
    NVIC_EnableIRQ(BLINK_EXTI_IRQN);

    xTaskCreate(prv_task, "Sim_Blink", BLINK_TASK_STACK, &g_blink_sim, BLINK_TASK_PRIORITY, &g_blink_sim.task_handle);
}

uint8_t sim_blink_led_get_last(sim_blink_led_last_t* last)
{
    uint8_t available = 0u;

    if (last == NULL) {
        return 0u;
    }

    if (g_blink_sim.data_mutex == NULL) {
        return 0u;
    }

    if (xSemaphoreTake(g_blink_sim.data_mutex, portMAX_DELAY) == pdTRUE) {
        *last = g_blink_sim.last;
        available = (g_blink_sim.last.high_ms != 0u || g_blink_sim.last.low_ms != 0u) ? 1u : 0u;
        xSemaphoreGive(g_blink_sim.data_mutex);
    }

    return available;
}

/* IRQ Handlers ------------------------------------------------------------*/

void EXTI9_5_IRQHandler(void)
{
    BaseType_t woken = pdFALSE;
    uint32_t now_ms;
    uint8_t pin_level;

    if (EXTI_GetITStatus(BLINK_EXTI_LINE) == RESET) {
        return;
    }

    timer_get_counter(&g_blink_sim.timebase, &now_ms);
    gpio_read(&g_blink_sim.input, &pin_level);

    if (pin_level != 0u) {
        if (g_has_fall_tick != 0u) {
            g_low_ms = prv_ticks_to_ms(prv_timer_delta(now_ms, g_fall_tick));
        }

        g_rise_tick = now_ms;
        g_has_rise_tick = 1u;
        g_edge_state = BLINK_EDGE_WAIT_FALLING;
        g_edge_event = 1u;
        xSemaphoreGiveFromISR(g_blink_sim.event_sem, &woken);
    } else {
        g_fall_tick = now_ms;
        if (g_has_rise_tick != 0u) {
            g_high_ms = prv_ticks_to_ms(prv_timer_delta(g_fall_tick, g_rise_tick));
            g_measure_ready = 1u;
        }
        g_has_fall_tick = 1u;
        g_edge_state = BLINK_EDGE_WAIT_RISING;
        g_edge_event = 2u;
        xSemaphoreGiveFromISR(g_blink_sim.event_sem, &woken);
    }

    EXTI_ClearITPendingBit(BLINK_EXTI_LINE);
    portYIELD_FROM_ISR(woken);
}

/* Private functions -------------------------------------------------------*/

static void prv_task(void* arg)
{
    sim_blink_led_t* sim = (sim_blink_led_t*) arg;
    uint32_t high_ms;
    uint32_t low_ms;
    uint8_t valid;
    uint8_t edge_event;

    for (;;) {
        xSemaphoreTake(sim->event_sem, portMAX_DELAY);

        edge_event = g_edge_event;
        high_ms = g_high_ms;
        low_ms = g_low_ms;

        if (edge_event == 1u) {
            SIM_BLINK_EDGE_LOG("BLINK RISING=%lu\n", (unsigned long) g_rise_tick);
        } else if (edge_event == 2u) {
            SIM_BLINK_EDGE_LOG("BLINK FALLING=%lu HIGH=%lums\n", (unsigned long) g_fall_tick, (unsigned long) high_ms);
        }

        if (edge_event != 1u || g_has_rise_tick == 0u || g_has_fall_tick == 0u || g_measure_ready == 0u ||
            low_ms == 0u) {
            continue;
        }

        valid = (prv_validate_duration(high_ms) != 0u && prv_validate_duration(low_ms) != 0u) ? 1u : 0u;

        prv_store_last(sim, high_ms, low_ms, valid);
        SIM_BLINK_LOG("BLINK HIGH=%lums LOW=%lums VALID=%u\n", (unsigned long) high_ms, (unsigned long) low_ms,
                      (unsigned int) valid);
    }
}

static void prv_config_clocks(void)
{
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_AFIO, ENABLE);
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);
}

static void prv_config_exti(uint32_t trigger)
{
    EXTI_InitTypeDef exti_init;

    GPIO_EXTILineConfig(BLINK_GPIO_PORT_SOURCE, BLINK_GPIO_PIN_SOURCE);
    EXTI_ClearITPendingBit(BLINK_EXTI_LINE);
    exti_init.EXTI_Line = BLINK_EXTI_LINE;
    exti_init.EXTI_Mode = EXTI_Mode_Interrupt;
    exti_init.EXTI_Trigger = trigger;
    exti_init.EXTI_LineCmd = ENABLE;
    EXTI_Init(&exti_init);
}

static uint32_t prv_get_tim2_clock_hz(void)
{
    RCC_ClocksTypeDef clocks;
    uint32_t tim_clock_hz;

    RCC_GetClocksFreq(&clocks);
    tim_clock_hz = clocks.PCLK1_Frequency;

    if ((RCC->CFGR & RCC_CFGR_PPRE1) != 0u) {
        tim_clock_hz *= 2u;
    }

    return tim_clock_hz;
}

static uint32_t prv_timer_delta(uint32_t newer_tick, uint32_t older_tick)
{
    if (newer_tick >= older_tick) {
        return newer_tick - older_tick;
    }

    return (BLINK_TIMER_PERIOD - older_tick) + newer_tick + 1u;
}

static uint32_t prv_ticks_to_ms(uint32_t ticks)
{
    return (ticks + (BLINK_TICKS_PER_MS / 2u)) / BLINK_TICKS_PER_MS;
}

static uint8_t prv_validate_duration(uint32_t duration_ms)
{
    uint32_t min_ms = BLINK_EXPECTED_MS - BLINK_TOLERANCE_MS;
    uint32_t max_ms = BLINK_EXPECTED_MS + BLINK_TOLERANCE_MS;

    return (duration_ms >= min_ms && duration_ms <= max_ms) ? 1u : 0u;
}

static void prv_store_last(sim_blink_led_t* sim, uint32_t high_ms, uint32_t low_ms, uint8_t valid)
{
    if (xSemaphoreTake(sim->data_mutex, portMAX_DELAY) == pdTRUE) {
        sim->last.high_ms = high_ms;
        sim->last.low_ms = low_ms;
        sim->last.valid = valid;
        xSemaphoreGive(sim->data_mutex);
    }
}
