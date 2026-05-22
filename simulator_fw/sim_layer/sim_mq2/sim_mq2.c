/**
 * @file    sim_mq2.c
 * @brief   Self-contained MQ-2 Simulation Module.
 */

#include "sim_mq2.h"
#include <string.h>

/* Private macros ----------------------------------------------------------*/

#define DAC_INST        DAC
#define DAC_CH          DAC_Channel_1
#define VREF_MV         3300
#define STACK_SIZE      256
#define TASK_PRIO       (tskIDLE_PRIORITY + 1)
#define TICK_MS         100u

/* Static objects ----------------------------------------------------------*/

static sim_mq2_t g_mq2_sim;

/* Private function prototypes ---------------------------------------------*/

static void prv_task(void *arg);

/* Public functions --------------------------------------------------------*/

void sim_mq2_init(void)
{
    memset(&g_mq2_sim, 0, sizeof(sim_mq2_t));

    /* 1. Logic reset */
    mq2_reset(&g_mq2_sim.logic);

    /* 2. RTOS objects */
    g_mq2_sim.mutex = xSemaphoreCreateMutex();

    /* 3. Driver setup */
    dac_setup(&g_mq2_sim.dac, DAC_INST, DAC_CH, DAC_Trigger_None, 
              DAC_OutputBuffer_Enable, DAC_ALIGN_12B_R, VREF_MV);
    dac_enable(&g_mq2_sim.dac);
    dac_write_millivolts(&g_mq2_sim.dac, g_mq2_sim.logic.current_mv);

    /* 4. Start task */
    xTaskCreate(prv_task, "Sim_MQ2", STACK_SIZE, &g_mq2_sim, TASK_PRIO, &g_mq2_sim.task_handle);
}

void sim_mq2_set_scene(mq2_scene_t scene)
{
    if (xSemaphoreTake(g_mq2_sim.mutex, portMAX_DELAY) == pdTRUE)
    {
        mq2_set_scene(&g_mq2_sim.logic, scene);
        xSemaphoreGive(g_mq2_sim.mutex);
    }
}

/* Private functions -------------------------------------------------------*/

static void prv_task(void *arg)
{
    sim_mq2_t *sim = (sim_mq2_t *)arg;
    for (;;)
    {
        if (xSemaphoreTake(sim->mutex, portMAX_DELAY) == pdTRUE)
        {
            mq2_tick(&sim->logic);
            dac_write_millivolts(&sim->dac, sim->logic.current_mv);
            xSemaphoreGive(sim->mutex);
        }
        vTaskDelay(pdMS_TO_TICKS(TICK_MS));
    }
}
