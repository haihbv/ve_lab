/**
 * @file    sim_dht22.h
 * @brief   DHT22/AM2302 Simulation Module (Self-contained).
 *
 * @details Wraps DHT22 sensor logic with TIM IC/OC drivers and a
 *          FreeRTOS task. The module self-manages all GPIO switching,
 *          IRQ routing, and protocol timing.
 *
 *          Architecture:
 *            IC Timer  -> prv_ic_cb  -> detects host start pulse (>= 1ms LOW)
 *            Semaphore -> prv_task   -> rebuilds frame, starts OC
 *            OC Timer  -> prv_oc_cb  -> drives 40-bit response FSM
 */

#ifndef SIM_DHT22_H
#define SIM_DHT22_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ----------------------------------------------------------------*/
#include "dht22.h"
#include "driver_tim.h"
#include "driver_gpio.h"
#include "FreeRTOS.h"
#include "queue.h"
#include "semphr.h"
#include "task.h"

/* Exported types ----------------------------------------------------------*/

/**
 * @brief DHT22 data payload for runtime update.
 */
typedef struct {
    uint16_t humidity_x10; /**< Humidity x10 (0-1000, e.g. 653 = 65.3%RH). */
    int16_t  temp_x10;     /**< Temperature x10 (-400 to 800, e.g. 278 = 27.8C). */
} sim_dht22_data_t;

/**
 * @brief DHT22 Simulation object.
 */
typedef struct {
    dht22_t           logic;       /**< Pure logic instance (sensor layer). */
    timer_t           ic;          /**< IC timer: measures host start pulse width. */
    timer_t           oc;          /**< OC timer: drives protocol response timing. */
    gpio_t            pin;         /**< Single-wire data pin. */

    SemaphoreHandle_t start_sem;   /**< Signals task when host start pulse is valid. */
    QueueHandle_t     data_queue;  /**< Depth-1 queue for pending data updates. */
    TaskHandle_t      task_handle; /**< Handle for simulation task. */
} sim_dht22_t;

/* Public API --------------------------------------------------------------*/

/**
 * @brief  Initialize and start DHT22 simulation.
 * @note   Self-contained: configures GPIO, Timers, RTOS objects,
 *         and starts the background task internally.
 */
void sim_dht22_init(void);

/**
 * @brief  Update the humidity and temperature values returned by the simulator.
 * @param  data  Pointer to new sensor data.
 * @note   Safe to call from any task context. The new data takes effect
 *         on the next host-initiated transaction.
 */
void sim_dht22_set_data(const sim_dht22_data_t *data);

#ifdef __cplusplus
}
#endif

#endif /* SIM_DHT22_H */
