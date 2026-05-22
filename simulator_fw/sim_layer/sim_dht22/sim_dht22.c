/**
 * @file    sim_dht22.c
 * @brief   Self-contained DHT22/AM2302 Simulation Module.
 *
 * Protocol timing (DHT22/AM2302 datasheet):
 *   Host start LOW  : >= 1 ms
 *   Sensor response : LOW 84us -> HIGH 84us
 *   Bit LOW phase   : 54 us
 *   Bit '0' HIGH    : 30 us
 *   Bit '1' HIGH    : 74 us
 *   Stop LOW        : 54 us -> release HIGH
 */

#include "sim_dht22.h"
#include <string.h>

/* Private macros ----------------------------------------------------------*/

#define DHT22_GPIO_PORT         GPIOA
#define DHT22_GPIO_PIN          GPIO_Pin_1
#define DHT22_TIM_IC_INST       TIM4
#define DHT22_TIM_OC_INST       TIM5

#define DHT22_TASK_STACK        256u
#define DHT22_TASK_PRIORITY     (tskIDLE_PRIORITY + 2u)

#define DHT_RESP_LOW_US         84u
#define DHT_RESP_HIGH_US        84u
#define DHT_BIT_LOW_US          54u
#define DHT_BIT_0_HIGH_US       30u
#define DHT_BIT_1_HIGH_US       74u
#define DHT_STOP_LOW_US         54u
#define DHT_STOP_CLEANUP_US     10u
#define DHT_START_MIN_US        1000u   /* DHT22 spec: host LOW >= 1 ms */

/* Private types -----------------------------------------------------------*/

typedef enum {
    RESP_IDLE = 0,
    RESP_START_LOW,
    RESP_START_HIGH,
    RESP_BIT_LOW,
    RESP_BIT_HIGH,
    RESP_STOP_LOW,
    RESP_DONE
} dht22_resp_state_t;

/* Static objects ----------------------------------------------------------*/

static sim_dht22_t               g_dht22_sim;
static volatile dht22_resp_state_t g_resp_state;
static volatile uint8_t          g_ic_awaiting_rise;
static volatile uint32_t         g_ic_fall_capture;

/* Private function prototypes ---------------------------------------------*/

static void prv_task(void *arg);
static void prv_oc_cb(void *ctx, const timer_evt_t *evt);
static void prv_ic_cb(void *ctx, const timer_evt_t *evt);
static void prv_set_pin_output(void);
static void prv_set_pin_input(void);
static void prv_schedule_oc(uint16_t us);

/* Public functions --------------------------------------------------------*/

void sim_dht22_init(void)
{
    uint16_t psc;
    uint32_t arr;

    memset(&g_dht22_sim, 0, sizeof(sim_dht22_t));

    dht22_reset(&g_dht22_sim.logic);

    g_dht22_sim.start_sem  = xSemaphoreCreateBinary();
    g_dht22_sim.data_queue = xQueueCreate(1u, sizeof(sim_dht22_data_t));

    gpio_setup(&g_dht22_sim.pin, DHT22_GPIO_PORT, DHT22_GPIO_PIN,
               GPIO_Mode_IPU, GPIO_Speed_50MHz);
    gpio_enable(&g_dht22_sim.pin);

    psc = (uint16_t)((SystemCoreClock / 1000000u) - 1u);
    arr = 0xFFFFu;

    timer_setup(&g_dht22_sim.ic, DHT22_TIM_IC_INST);
    timer_setup(&g_dht22_sim.oc, DHT22_TIM_OC_INST);

    timer_register_callback(&g_dht22_sim.ic, prv_ic_cb, &g_dht22_sim);
    timer_register_callback(&g_dht22_sim.oc, prv_oc_cb, &g_dht22_sim);

    timer_base_setup(&g_dht22_sim.ic, psc, arr);
    timer_base_setup(&g_dht22_sim.oc, psc, arr);

    timer_ic_setup(&g_dht22_sim.ic, TIMER_CH1, psc, arr,
                   TIM_ICPolarity_Falling, 0x0Fu);
    timer_oc_setup(&g_dht22_sim.oc, TIMER_CH1, psc, arr,
                   0u, TIM_OCMode_Timing);

    xTaskCreate(prv_task, "Sim_DHT22", DHT22_TASK_STACK,
                &g_dht22_sim, DHT22_TASK_PRIORITY, &g_dht22_sim.task_handle);

    timer_ic_enable(&g_dht22_sim.ic, TIMER_CH1);
    timer_enable(&g_dht22_sim.ic);
    timer_enable(&g_dht22_sim.oc);
}

void sim_dht22_set_data(const sim_dht22_data_t *data)
{
    if (data == NULL) {
        return;
    }

    /* xQueueOverwrite keeps the queue depth at 1; only the latest value matters */
    xQueueOverwrite(g_dht22_sim.data_queue, data);
}

/* IRQ Handlers ------------------------------------------------------------*/

void TIM4_IRQHandler(void) { timer_handle_irq(&g_dht22_sim.ic); }
void TIM5_IRQHandler(void) { timer_handle_irq(&g_dht22_sim.oc); }

/* Private functions -------------------------------------------------------*/

static void prv_task(void *arg)
{
    sim_dht22_t     *sim = (sim_dht22_t *)arg;
    sim_dht22_data_t pending;

    for (;;) {
        xSemaphoreTake(sim->start_sem, portMAX_DELAY);

        /* Apply latest data update if one was queued */
        if (xQueueReceive(sim->data_queue, &pending, 0) == pdTRUE) {
            dht22_set(&sim->logic, pending.humidity_x10, pending.temp_x10);
        }

        /* Disable IC while driving the bus to avoid spurious captures */
        timer_ic_disable(&sim->ic, TIMER_CH1);
        dht22_reset_comm(&sim->logic);

        prv_set_pin_output();
        GPIO_WriteBit(DHT22_GPIO_PORT, DHT22_GPIO_PIN, Bit_RESET);
        g_resp_state = RESP_START_LOW;
        prv_schedule_oc(DHT_RESP_LOW_US);

        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
    }
}

static void prv_oc_cb(void *ctx, const timer_evt_t *evt)
{
    sim_dht22_t *sim = (sim_dht22_t *)ctx;
    BaseType_t   woken = pdFALSE;
    uint8_t      bit_val;

    if (evt == NULL || evt->type != TIMER_EVT_COMPARE) {
        return;
    }

    switch (g_resp_state) {

    case RESP_START_LOW:
        GPIO_WriteBit(DHT22_GPIO_PORT, DHT22_GPIO_PIN, Bit_SET);
        g_resp_state = RESP_START_HIGH;
        prv_schedule_oc(DHT_RESP_HIGH_US);
        break;

    case RESP_START_HIGH:
        GPIO_WriteBit(DHT22_GPIO_PORT, DHT22_GPIO_PIN, Bit_RESET);
        g_resp_state = RESP_BIT_LOW;
        prv_schedule_oc(DHT_BIT_LOW_US);
        break;

    case RESP_BIT_LOW:
        GPIO_WriteBit(DHT22_GPIO_PORT, DHT22_GPIO_PIN, Bit_SET);
        g_resp_state = RESP_BIT_HIGH;
        bit_val = dht22_tx(&sim->logic);
        prv_schedule_oc(bit_val ? DHT_BIT_1_HIGH_US : DHT_BIT_0_HIGH_US);
        break;

    case RESP_BIT_HIGH:
        /* current_bit was already incremented by dht22_tx() */
        if (sim->logic.current_bit < 40u) {
            GPIO_WriteBit(DHT22_GPIO_PORT, DHT22_GPIO_PIN, Bit_RESET);
            g_resp_state = RESP_BIT_LOW;
            prv_schedule_oc(DHT_BIT_LOW_US);
        } else {
            GPIO_WriteBit(DHT22_GPIO_PORT, DHT22_GPIO_PIN, Bit_RESET);
            g_resp_state = RESP_STOP_LOW;
            prv_schedule_oc(DHT_STOP_LOW_US);
        }
        break;

    case RESP_STOP_LOW:
        GPIO_WriteBit(DHT22_GPIO_PORT, DHT22_GPIO_PIN, Bit_SET);
        g_resp_state = RESP_DONE;
        prv_schedule_oc(DHT_STOP_CLEANUP_US);
        break;

    case RESP_DONE:
        timer_oc_disable(&sim->oc, TIMER_CH1);
        g_resp_state = RESP_IDLE;

        /* Re-arm IC for the next host transaction */
        prv_set_pin_input();
        g_ic_awaiting_rise = 0u;
        timer_ic_set_polarity(&sim->ic, TIMER_CH1, TIM_ICPolarity_Falling);
        timer_ic_enable(&sim->ic, TIMER_CH1);

        vTaskNotifyGiveFromISR(sim->task_handle, &woken);
        portYIELD_FROM_ISR(woken);
        break;

    default:
        timer_oc_disable(&sim->oc, TIMER_CH1);
        g_resp_state = RESP_IDLE;
        break;
    }
}

static void prv_ic_cb(void *ctx, const timer_evt_t *evt)
{
    sim_dht22_t *sim = (sim_dht22_t *)ctx;
    BaseType_t   woken = pdFALSE;
    uint32_t     t_rise;
    uint32_t     pulse_us;

    if (evt == NULL || evt->type != TIMER_EVT_CAPTURE) {
        return;
    }

    if (g_ic_awaiting_rise == 0u) {
        g_ic_fall_capture  = evt->value;
        g_ic_awaiting_rise = 1u;
        timer_ic_set_polarity(&sim->ic, TIMER_CH1, TIM_ICPolarity_Rising);
    } else {
        t_rise = evt->value;
        /* Handle 16-bit counter wrap-around */
        pulse_us = (t_rise >= g_ic_fall_capture) ?
                   (t_rise - g_ic_fall_capture) :
                   (0xFFFFu - g_ic_fall_capture + t_rise + 1u);

        g_ic_awaiting_rise = 0u;
        timer_ic_set_polarity(&sim->ic, TIMER_CH1, TIM_ICPolarity_Falling);

        /* DHT22 only requires >= 1 ms LOW; no upper bound in datasheet */
        if (pulse_us >= DHT_START_MIN_US) {
            xSemaphoreGiveFromISR(sim->start_sem, &woken);
            portYIELD_FROM_ISR(woken);
        }
    }
}

/* Open-drain output: allows pulling bus LOW and letting it float HIGH */
static void prv_set_pin_output(void)
{
    GPIO_InitTypeDef gpio_init;

    gpio_init.GPIO_Pin   = DHT22_GPIO_PIN;
    gpio_init.GPIO_Mode  = GPIO_Mode_Out_OD;
    gpio_init.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(DHT22_GPIO_PORT, &gpio_init);
}

static void prv_set_pin_input(void)
{
    GPIO_InitTypeDef gpio_init;

    gpio_init.GPIO_Pin  = DHT22_GPIO_PIN;
    gpio_init.GPIO_Mode = GPIO_Mode_IPU;
    GPIO_Init(DHT22_GPIO_PORT, &gpio_init);
}

static void prv_schedule_oc(uint16_t us)
{
    uint32_t cnt;

    timer_get_counter(&g_dht22_sim.oc, &cnt);
    timer_oc_set_compare(&g_dht22_sim.oc, TIMER_CH1, (uint16_t)(cnt + us));
    timer_oc_enable(&g_dht22_sim.oc, TIMER_CH1);
}
