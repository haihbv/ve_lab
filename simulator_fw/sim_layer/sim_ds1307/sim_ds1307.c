/**
 * @file    sim_ds1307.c
 * @brief   Self-contained DS1307 Simulation Module implementation.
 */

#include "sim_ds1307.h"
#include "stm32f10x.h"
#include <string.h>

/* Private macros ----------------------------------------------------------*/

#define SIM_DS1307_I2C_INST     I2C1
#define SIM_DS1307_ADDR         0x68
#define SIM_DS1307_I2C_SPEED    100000
#define SIM_DS1307_STACK_SIZE   256
#define SIM_DS1307_TASK_PRIO    (tskIDLE_PRIORITY + 1)

/* Private function prototypes ---------------------------------------------*/

static void    prv_hw_init(void);
static void    prv_i2c_event_cb(void *ctx, const i2c_slave_evt_t *evt);
static uint8_t prv_i2c_tx_cb(void *ctx);
static void    prv_sim_task(void *argument);

/* Static objects ----------------------------------------------------------*/

static sim_ds1307_t g_ds1307_sim;

/* Public functions --------------------------------------------------------*/

/**
 * @brief Initialize and start the DS1307 simulation module.
 */
void sim_ds1307_init(void)
{
    memset(&g_ds1307_sim, 0, sizeof(sim_ds1307_t));

    /* 1. Low-level Hardware Initialization */
    prv_hw_init();

    /* 2. Logic layer initialization */
    ds1307_reset(&g_ds1307_sim.logic);

    /* 3. RTOS synchronization objects */
    g_ds1307_sim.mutex = xSemaphoreCreateMutex();
    g_ds1307_sim.write_sem = xSemaphoreCreateBinary();

    /* 4. Driver layer setup and registration */
    i2c_slave_setup(&g_ds1307_sim.i2c, 
                    SIM_DS1307_I2C_INST, 
                    SIM_DS1307_ADDR, 
                    SIM_DS1307_I2C_SPEED, 
                    0);
                    
    i2c_slave_register_event(&g_ds1307_sim.i2c, prv_i2c_event_cb, &g_ds1307_sim);
    i2c_slave_register_tx(&g_ds1307_sim.i2c, prv_i2c_tx_cb, &g_ds1307_sim);
    
    /* Enable hardware communication */
    i2c_slave_enable(&g_ds1307_sim.i2c);

    /* 5. Simulation task creation */
    xTaskCreate(prv_sim_task, 
                "Sim_DS1307", 
                SIM_DS1307_STACK_SIZE, 
                &g_ds1307_sim, 
                SIM_DS1307_TASK_PRIO, 
                &g_ds1307_sim.task_handle);
}

/* IRQ Handlers ------------------------------------------------------------*/

/**
 * @brief I2C1 Event Interrupt Handler.
 */
void I2C1_EV_IRQHandler(void)
{
    i2c_slave_handle_ev(&g_ds1307_sim.i2c);
}

/**
 * @brief I2C1 Error Interrupt Handler.
 */
void I2C1_ER_IRQHandler(void)
{
    i2c_slave_handle_er(&g_ds1307_sim.i2c);
}

/* Private functions -------------------------------------------------------*/

/**
 * @brief Setup clocks, GPIOs, and NVIC for I2C communication.
 */
static void prv_hw_init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    NVIC_InitTypeDef NVIC_InitStructure;

    /* Enable Clocks for I2C1, GPIOB, and AFIO */
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_I2C1, ENABLE);
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB | RCC_APB2Periph_AFIO, ENABLE);

    /* Configure GPIO Pins (PB6=SCL, PB7=SDA) */
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6 | GPIO_Pin_7;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_OD;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOB, &GPIO_InitStructure);

    /* Configure NVIC for I2C1 Events */
    NVIC_InitStructure.NVIC_IRQChannel = I2C1_EV_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 5;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);

    /* Configure NVIC for I2C1 Errors */
    NVIC_InitStructure.NVIC_IRQChannel = I2C1_ER_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 5;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);
}

/**
 * @brief Handle I2C slave events from the driver.
 */
static void prv_i2c_event_cb(void *ctx, const i2c_slave_evt_t *evt)
{
    sim_ds1307_t *sim = (sim_ds1307_t *)ctx;
    BaseType_t woken = pdFALSE;
    
    switch (evt->type)
    {
        case I2C_SLAVE_EVT_WRITE_START:
            /* Start of write transaction */
            sim->logic.write_pending = 0;
            sim->logic.write_len = 0;
            break;

        case I2C_SLAVE_EVT_BYTE_RECEIVED:
            /* Forward byte to pure logic layer */
            ds1307_rx(&sim->logic, evt->data, (sim->logic.write_pending == 0 && sim->logic.write_len == 0));
            break;

        case I2C_SLAVE_EVT_WRITE_DONE:
            /* Signal simulation task to apply the writes safely */
            if (sim->logic.write_len > 0)
            {
                xSemaphoreGiveFromISR(sim->write_sem, &woken);
            }
            break;

        case I2C_SLAVE_EVT_READ_START:
            /* Lock the active bank to prevent tearing during read */
            ds1307_read_start(&sim->logic);
            break;

        case I2C_SLAVE_EVT_READ_DONE:
            /* Release the bank lock */
            ds1307_read_done(&sim->logic);
            break;

        default:
            break;
    }
    
    portYIELD_FROM_ISR(woken);
}

/**
 * @brief Provide the next byte to be transmitted by the I2C driver.
 */
static uint8_t prv_i2c_tx_cb(void *ctx)
{
    sim_ds1307_t *sim = (sim_ds1307_t *)ctx;
    return ds1307_tx(&sim->logic);
}

/**
 * @brief Main simulation task handling 1s ticks and asynchronous I2C writes.
 */
static void prv_sim_task(void *argument)
{
    sim_ds1307_t *sim = (sim_ds1307_t *)argument;
    
    for (;;)
    {
        /* Wait up to 1 second for an I2C write transaction to complete */
        if (xSemaphoreTake(sim->write_sem, pdMS_TO_TICKS(1000)) == pdTRUE)
        {
            if (xSemaphoreTake(sim->mutex, portMAX_DELAY) == pdTRUE)
            {
                ds1307_apply_writes(&sim->logic);
                xSemaphoreGive(sim->mutex);
            }
        }
        else
        {
            /* Timeout occurred, meaning 1 second has elapsed. Tick the clock. */
            if (xSemaphoreTake(sim->mutex, portMAX_DELAY) == pdTRUE)
            {
                ds1307_tick(&sim->logic);
                xSemaphoreGive(sim->mutex);
            }
        }
    }
}
