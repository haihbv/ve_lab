/**
 * @file    sim_ds1307.h
 * @brief   DS1307 Simulation Module (Self-contained).
 * @details Integrates DS1307 logic with I2C Slave driver and RTOS.
 */

#ifndef SIM_DS1307_H
#define SIM_DS1307_H

#include "ds1307.h"
#include "driver_i2c_slave.h"
#include "FreeRTOS.h"
#include "semphr.h"
#include "task.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Exported macros ---------------------------------------------------------*/

/* Exported types ----------------------------------------------------------*/

/**
 * @brief DS1307 Simulation object handle.
 */
typedef struct {
    ds1307_t          logic;        /**< Pure logic instance */
    i2c_slave_t       i2c;          /**< Hardware I2C Slave driver */
    
    SemaphoreHandle_t mutex;        /**< Mutex for thread-safe access to logic */
    SemaphoreHandle_t write_sem;    /**< Semaphore to signal I2C write completion */
    TaskHandle_t      task_handle;  /**< Handle for the simulation task */
} sim_ds1307_t;

/* Public API --------------------------------------------------------------*/

/**
 * @brief Initialize and start the DS1307 simulation module.
 * @note  This is a self-contained module that initializes its own hardware (I2C Slave),
 *        RTOS tasks, and internal logic.
 */
void sim_ds1307_init(void);

#ifdef __cplusplus
}
#endif

#endif /* SIM_DS1307_H */
