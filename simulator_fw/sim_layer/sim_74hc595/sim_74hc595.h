/**
 * @file    sim_74hc595.h
 * @brief   74HC595 Simulation Module (Self-contained).
 */

#ifndef SIM_74HC595_H
#define SIM_74HC595_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ----------------------------------------------------------------*/
#include "74hc595.h"
#include "driver_gpio.h"
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"

/* Exported types ----------------------------------------------------------*/

/**
 * @brief 74HC595 Simulation object.
 */
typedef struct {
    hc595_t           logic;       /**< Pure logic instance */
    gpio_t            ds;          /**< Data Serial pin */
    gpio_t            shcp;        /**< Shift Clock pin */
    gpio_t            stcp;        /**< Storage Clock (Latch) pin */
    
    SemaphoreHandle_t mutex;       /**< Mutex for thread-safe access */
    TaskHandle_t      task_handle; /**< Handle for simulation task */
} sim_74hc595_t;

/* Public API --------------------------------------------------------------*/

/**
 * @brief Initialize and start 74HC595 simulation.
 * @note  Self-contained module.
 */
void sim_74hc595_init(void);

#ifdef __cplusplus
}
#endif

#endif /* SIM_74HC595_H */
