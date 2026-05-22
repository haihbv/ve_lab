/**
 * @file    dht22.h
 * @brief   DHT22/AM2302 Sensor Logic (Pure C, No RTOS, No Driver).
 *
 * @details Implements the 40-bit single-wire DHT22 protocol frame
 *          as a pure state machine. All timing is delegated to the
 *          Sim Layer via OC callbacks.
 *
 *          Frame format (40 bits, MSB-first):
 *            [7:0]   Humidity high byte
 *            [15:8]  Humidity low byte   => humidity_x10
 *            [23:16] Temperature high byte (bit 7 = sign for negative)
 *            [31:24] Temperature low byte => abs(temp_x10)
 *            [39:32] Checksum (sum of bytes 0-3, lower 8 bits)
 *
 *          Encoding:
 *            humidity_x10 = 653  => 65.3 %RH
 *            temp_x10     = 278  => 27.8 degC
 *            temp_x10     = -55  => -5.5 degC  (bit 15 of raw word = 1)
 */

#ifndef DHT22_H
#define DHT22_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ----------------------------------------------------------------*/
#include <stdint.h>

/* Exported types ----------------------------------------------------------*/

/**
 * @brief DHT22 logic state.
 */
typedef struct {
    uint16_t humidity;    /**< Humidity x10 (e.g. 653 = 65.3%RH). Range: 0-1000. */
    int16_t  temperature; /**< Temperature x10 (e.g. 278 = 27.8C). Range: -400 to 800. */
    uint8_t  frame[5];    /**< 40-bit data packet, MSB-first. */
    uint8_t  current_bit; /**< Transmit bit index (0-39). */
} dht22_t;

/* Public API --------------------------------------------------------------*/

/**
 * @brief  Initialize DHT22 logic with default values (60.0%RH, 25.5C).
 * @param  dev  Pointer to DHT22 logic object.
 */
void dht22_reset(dht22_t *dev);

/**
 * @brief  Set simulated sensor values and rebuild the 40-bit frame.
 * @param  dev          Pointer to DHT22 logic object.
 * @param  humidity_x10 Humidity multiplied by 10 (0-1000).
 * @param  temp_x10     Temperature multiplied by 10 (-400 to 800).
 */
void dht22_set(dht22_t *dev, uint16_t humidity_x10, int16_t temp_x10);

/**
 * @brief  Get the next bit to transmit (MSB-first) and advance index.
 * @param  dev  Pointer to DHT22 logic object.
 * @return Bit value (0 or 1). Returns 0 if all 40 bits have been sent.
 */
uint8_t dht22_tx(dht22_t *dev);

/**
 * @brief  Reset communication state for next transaction.
 * @param  dev  Pointer to DHT22 logic object.
 */
void dht22_reset_comm(dht22_t *dev);

#ifdef __cplusplus
}
#endif

#endif /* DHT22_H */
