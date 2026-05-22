/**
 * @file    74hc595.h
 * @brief   74HC595 Pure Logic Simulation (No RTOS, No Driver).
 */

#ifndef HC595_H
#define HC595_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ----------------------------------------------------------------*/
#include <stdint.h>

/* Exported types ----------------------------------------------------------*/

/**
 * @brief 74HC595 logic state.
 */
typedef struct {
    uint8_t  shift_reg;   /**< 8-bit internal shift register */
    uint8_t  storage_reg; /**< 8-bit storage/output register */
    uint32_t bit_count;   /**< Total bits shifted since reset */
} hc595_t;

/* Public API --------------------------------------------------------------*/

/**
 * @brief Initialize 74HC595 logic with default values.
 */
void hc595_reset(hc595_t *dev);

/**
 * @brief Shift in one bit.
 * @param ds_bit The bit value (0 or 1) at the SER/DS pin.
 */
void hc595_shift(hc595_t *dev, uint8_t ds_bit);

/**
 * @brief Latch the shift register into the storage register.
 */
void hc595_latch(hc595_t *dev);

/**
 * @brief Get the current output value.
 */
uint8_t hc595_get(hc595_t *dev);

#ifdef __cplusplus
}
#endif

#endif /* HC595_H */
