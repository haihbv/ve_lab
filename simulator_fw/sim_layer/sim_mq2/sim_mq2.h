/**
 * @file    sim_mq2.h
 * @brief   MQ-2 Simulation Module (Self-contained).
 */

#ifndef SIM_MQ2_H
#define SIM_MQ2_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ----------------------------------------------------------------*/
#include "mq2.h"
#include "driver_dac.h"
#include "FreeRTOS.h"
#include "semphr.h"
#include "task.h"

/* Exported types ----------------------------------------------------------*/

/**
 * @brief MQ-2 Simulation object.
 */
typedef struct {
    mq2_t             logic;       /**< Pure logic instance */
    dac_t             dac;         /**< DAC driver for voltage output */
    
    SemaphoreHandle_t mutex;       /**< Mutex for thread-safe access */
    TaskHandle_t      task_handle; /**< Handle for simulation task */
} sim_mq2_t;

/* Public API --------------------------------------------------------------*/

/**
 * @brief Initialize and start MQ-2 simulation.
 * @note  Self-contained module.
 */
void sim_mq2_init(void);

/**
 * @brief Set the simulated gas scene.
 * @param scene The environmental scene to simulate.
 */
void sim_mq2_set_scene(mq2_scene_t scene);

#ifdef __cplusplus
}
#endif

#endif /* SIM_MQ2_H */
