/**
 * @file    74hc595.c
 * @brief   74HC595 Pure Logic Simulation implementation.
 */

#include "74hc595.h"
#include <string.h>

/* Public functions --------------------------------------------------------*/

void hc595_reset(hc595_t *dev)
{
    if (dev == NULL) return;
    memset(dev, 0, sizeof(hc595_t));
}

void hc595_shift(hc595_t *dev, uint8_t ds_bit)
{
    if (dev == NULL) return;
    /* Shift left and add new bit at LSB */
    dev->shift_reg = (uint8_t)((dev->shift_reg << 1) | (ds_bit & 0x01u));
    dev->bit_count++;
}

void hc595_latch(hc595_t *dev)
{
    if (dev == NULL) return;
    dev->storage_reg = dev->shift_reg;
}

uint8_t hc595_get(hc595_t *dev)
{
    return (dev != NULL) ? dev->storage_reg : 0u;
}
