/**
 * @file    dht22.c
 * @brief   DHT22/AM2302 Sensor Logic implementation.
 *
 * @details Pure C — no RTOS, no hardware dependency.
 *          Negative temperature is encoded with bit 15 of the 16-bit
 *          raw word set (sign-magnitude, NOT two's complement).
 */

#include "dht22.h"
#include <string.h>

/* Public functions --------------------------------------------------------*/

void dht22_reset(dht22_t *dev)
{
    if (dev == NULL) {
        return;
    }

    dht22_set(dev, 600u, 255); /* Default: 60.0%RH, 25.5C */
    dev->current_bit = 0u;
}

void dht22_set(dht22_t *dev, uint16_t humidity_x10, int16_t temp_x10)
{
    uint16_t temp_raw;

    if (dev == NULL) {
        return;
    }

    dev->humidity    = humidity_x10;
    dev->temperature = temp_x10;

    /*
     * Build 40-bit frame (MSB-first per byte):
     *   Byte 0: Humidity high byte
     *   Byte 1: Humidity low byte
     *   Byte 2: Temperature high byte (bit7 = sign for negative)
     *   Byte 3: Temperature low byte
     *   Byte 4: Checksum = (B0 + B1 + B2 + B3) & 0xFF
     *
     * Negative temperature: set bit 15 of temp_raw, store abs value.
     * Example: -5.5C => temp_x10 = -55 => temp_raw = 0x8037 (55 | 0x8000)
     */
    temp_raw = (temp_x10 < 0) ?
               (uint16_t)((uint16_t)(-temp_x10) | 0x8000u) :
               (uint16_t)temp_x10;

    dev->frame[0] = (uint8_t)((humidity_x10 >> 8u) & 0xFFu);
    dev->frame[1] = (uint8_t)(humidity_x10 & 0xFFu);
    dev->frame[2] = (uint8_t)((temp_raw >> 8u) & 0xFFu);
    dev->frame[3] = (uint8_t)(temp_raw & 0xFFu);
    dev->frame[4] = (uint8_t)((dev->frame[0] + dev->frame[1] +
                                dev->frame[2] + dev->frame[3]) & 0xFFu);
}

uint8_t dht22_tx(dht22_t *dev)
{
    uint8_t byte_idx;
    uint8_t bit_pos;
    uint8_t bit_val;

    if (dev == NULL || dev->current_bit >= 40u) {
        return 0u;
    }

    byte_idx = dev->current_bit / 8u;
    bit_pos  = 7u - (dev->current_bit % 8u);
    bit_val  = (dev->frame[byte_idx] >> bit_pos) & 0x01u;

    dev->current_bit++;
    return bit_val;
}

void dht22_reset_comm(dht22_t *dev)
{
    if (dev == NULL) {
        return;
    }

    dev->current_bit = 0u;
}
