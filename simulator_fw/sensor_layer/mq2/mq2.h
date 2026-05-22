/**
 * @file    mq2.h
 * @brief   MQ-2 Pure Logic Simulation (No RTOS, No Driver).
 */

#ifndef MQ2_H
#define MQ2_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ----------------------------------------------------------------*/
#include <stdint.h>
#include <stddef.h>

/* Exported types ----------------------------------------------------------*/

/**
 * @brief MQ-2 environmental scene.
 */
typedef enum {
    MQ2_SCENE_CLEAN_AIR = 0,
    MQ2_SCENE_SMOKE,
    MQ2_SCENE_LPG,
    MQ2_SCENE_METHANE,
    MQ2_SCENE_ALCOHOL
} mq2_scene_t;

/**
 * @brief MQ-2 logic state.
 */
typedef struct {
    uint16_t current_mv;            /**< Current sensor output voltage (mV) */
    uint16_t target_mv;             /**< Target sensor output voltage (mV) */
    uint16_t slew_rate_mv_per_tick; /**< Maximum voltage change per tick */
} mq2_t;

/* Public API --------------------------------------------------------------*/

/**
 * @brief Initialize MQ-2 logic with default values.
 */
void mq2_reset(mq2_t *dev);

/**
 * @brief Set the environmental scene (changes target voltage).
 */
void mq2_set_scene(mq2_t *dev, mq2_scene_t scene);

/**
 * @brief Update the internal voltage based on slew rate.
 * @details Call this periodically to simulate sensor response time.
 */
void mq2_tick(mq2_t *dev);

/**
 * @brief Get the current sensor voltage.
 */
uint16_t mq2_get(mq2_t *dev);

#ifdef __cplusplus
}
#endif

#endif /* MQ2_H */
