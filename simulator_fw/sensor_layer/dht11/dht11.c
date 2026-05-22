/**
 * @file    dht11.c
 * @brief   DHT11 Sensor Logic implementation.
 *
 * @details Pure C — no RTOS, no hardware dependency.
 *          The Sim Layer drives all timing and GPIO toggling.
 */

#include "dht11.h"
#include <string.h>

/* Public functions --------------------------------------------------------*/

void dht11_reset(dht11_t *dev)
{
    if (dev == NULL) {
        return;
    }

    dht11_set(dev, 60u, 25u); /* Default: 60%RH, 25C */
    dev->current_bit = 0u;
}

void dht11_set(dht11_t *dev, uint8_t humidity, uint8_t temperature)
{
    if (dev == NULL) {
        return;
    }

    dev->humidity    = humidity;
    dev->temperature = temperature;

    /*
     * Build 40-bit frame (MSB-first per byte):
     *   Byte 0: Humidity integer
     *   Byte 1: Humidity decimal  (always 0 for DHT11)
     *   Byte 2: Temperature integer
     *   Byte 3: Temperature decimal (always 0 for DHT11)
     *   Byte 4: Checksum = (B0 + B1 + B2 + B3) & 0xFF
     */
    dev->frame[0] = humidity;
    dev->frame[1] = 0u;
    dev->frame[2] = temperature;
    dev->frame[3] = 0u;
    dev->frame[4] = (uint8_t)(dev->frame[0] + dev->frame[1] +
                               dev->frame[2] + dev->frame[3]);
}

uint8_t dht11_tx(dht11_t *dev)
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

void dht11_reset_comm(dht11_t *dev)
{
    if (dev == NULL) {
        return;
    }

    dev->current_bit = 0u;
}
