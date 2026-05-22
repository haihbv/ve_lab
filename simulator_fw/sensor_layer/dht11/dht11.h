/**
 * @file    dht11.h
 * @brief   DHT11 Sensor Logic (Pure C, No RTOS, No Driver).
 *
 * @details Implements the 40-bit single-wire DHT11 protocol frame
 *          as a pure state machine. All timing is delegated to the
 *          Sim Layer via OC callbacks.
 *
 *          Frame format (40 bits, MSB-first):
 *            [7:0]   Humidity integer
 *            [15:8]  Humidity decimal (always 0 for DHT11)
 *            [23:16] Temperature integer
 *            [31:24] Temperature decimal (always 0 for DHT11)
 *            [39:32] Checksum (sum of bytes 0-3)
 */

#ifndef DHT11_H
#define DHT11_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ----------------------------------------------------------------*/
#include <stdint.h>

/* Exported types ----------------------------------------------------------*/

/**
 * @brief DHT11 logic state.
 */
typedef struct {
    uint8_t humidity;    /**< Humidity (0-99 %RH). */
    uint8_t temperature; /**< Temperature (0-50 degC). */
    uint8_t frame[5];    /**< 40-bit data packet, MSB-first. */
    uint8_t current_bit; /**< Transmit bit index (0-39). */
} dht11_t;

/* Public API --------------------------------------------------------------*/

/**
 * @brief  Initialize DHT11 logic with default values (60%RH, 25C).
 * @param  dev  Pointer to DHT11 logic object.
 */
void dht11_reset(dht11_t *dev);

/**
 * @brief  Set simulated sensor values and rebuild the 40-bit frame.
 * @param  dev         Pointer to DHT11 logic object.
 * @param  humidity    Humidity value (0-99 %RH).
 * @param  temperature Temperature value (0-50 degC).
 */
void dht11_set(dht11_t *dev, uint8_t humidity, uint8_t temperature);

/**
 * @brief  Get the next bit to transmit (MSB-first) and advance index.
 * @param  dev  Pointer to DHT11 logic object.
 * @return Bit value (0 or 1). Returns 0 if all 40 bits have been sent.
 */
uint8_t dht11_tx(dht11_t *dev);

/**
 * @brief  Reset communication state for next transaction.
 * @param  dev  Pointer to DHT11 logic object.
 */
void dht11_reset_comm(dht11_t *dev);

#ifdef __cplusplus
}
#endif

#endif /* DHT11_H */
