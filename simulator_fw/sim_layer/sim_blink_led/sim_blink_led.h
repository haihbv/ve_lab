/**
 * @file    sim_blink_led.h
 * @brief   Blink LED timing checker simulation module.
 */

#ifndef SIM_BLINK_LED_H
#define SIM_BLINK_LED_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ----------------------------------------------------------------*/
#include <stdint.h>

/**
 * @brief Last measured blink timing.
 */
typedef struct
{
    uint32_t high_ms; /**< Last HIGH pulse width in milliseconds. */
    uint32_t low_ms;  /**< Last LOW pulse width in milliseconds. */
    uint8_t valid;    /**< 1 when both HIGH and LOW are within tolerance. */
} sim_blink_led_last_t;

/**
 * @brief  Initialize and start blink LED timing checker.
 * @note   Self-contained: configures PA5, EXTI5, TIM4 timebase, NVIC,
 *         and its background validation task.
 */
void sim_blink_led_init(void);

/**
 * @brief  Read the latest measured blink timing.
 * @param  last Pointer to destination object.
 * @return 1 if a measurement was available, otherwise 0.
 */
uint8_t sim_blink_led_get_last(sim_blink_led_last_t* last);

#ifdef __cplusplus
}
#endif

#endif /* SIM_BLINK_LED_H */
