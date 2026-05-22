/**
 * @file    driver_dac.c
 * @brief   DAC driver implementation.
 * @details Implements DAC setup, control, and output APIs.
 */

#include "driver_dac.h"

/* Private macros ----------------------------------------------------------*/

#define DAC_12BIT_MAX_RAW       4095u
#define DAC_8BIT_MAX_RAW        255u

/* Private function prototypes ---------------------------------------------*/

static uint16_t prv_get_max_raw(dac_align_t align);
static uint16_t prv_limit_raw(dac_t *dev, uint16_t raw);
static uint16_t prv_limit_millivolts(dac_t *dev, uint16_t millivolts);

/* Public functions --------------------------------------------------------*/

driver_status_t dac_setup(dac_t *dev,
                          DAC_TypeDef *instance,
                          uint32_t channel,
                          uint32_t trigger,
                          uint32_t output_buffer,
                          dac_align_t align,
                          uint16_t vref_millivolts)
{
    if (dev == NULL || instance == NULL || vref_millivolts == 0)
    {
        return DRIVER_ERR_INVALID_ARG;
    }

    if (channel != DAC_Channel_1 && channel != DAC_Channel_2)
    {
        return DRIVER_ERR_INVALID_ARG;
    }

    dev->instance        = instance;
    dev->channel         = channel;
    dev->trigger         = trigger;
    dev->output_buffer   = output_buffer;
    dev->align           = align;
    dev->vref_millivolts = vref_millivolts;
    dev->max_raw         = prv_get_max_raw(align);

    dev->state           = DAC_STATE_IDLE;
    dev->last_raw        = 0;
    dev->last_millivolts = 0;

    return DRIVER_OK;
}

driver_status_t dac_enable(dac_t *dev)
{
    if (dev == NULL || dev->instance == NULL)
    {
        return DRIVER_ERR_INVALID_ARG;
    }

    DAC_InitTypeDef dac_init;

    DAC_StructInit(&dac_init);

    dac_init.DAC_Trigger      = dev->trigger;
    dac_init.DAC_OutputBuffer = dev->output_buffer;

    DAC_Init(dev->channel, &dac_init);
    DAC_Cmd(dev->channel, ENABLE);

    dev->state = DAC_STATE_RUNNING;

    return DRIVER_OK;
}

driver_status_t dac_disable(dac_t *dev)
{
    if (dev == NULL || dev->instance == NULL)
    {
        return DRIVER_ERR_INVALID_ARG;
    }

    DAC_Cmd(dev->channel, DISABLE);

    dev->state = DAC_STATE_READY;

    return DRIVER_OK;
}

driver_status_t dac_write_raw(dac_t *dev, uint16_t raw)
{
    if (dev == NULL || dev->instance == NULL)
    {
        return DRIVER_ERR_INVALID_ARG;
    }

    if (dev->state != DAC_STATE_RUNNING)
    {
        return DRIVER_ERR_BUSY;
    }

    raw = prv_limit_raw(dev, raw);

    if (dev->channel == DAC_Channel_1)
    {
        DAC_SetChannel1Data(dev->align, raw);
    }
    else
    {
        DAC_SetChannel2Data(dev->align, raw);
    }

    dev->last_raw        = raw;
    dev->last_millivolts = dac_raw_to_millivolts(dev, raw);

    return DRIVER_OK;
}

driver_status_t dac_write_millivolts(dac_t *dev, uint16_t millivolts)
{
    uint16_t raw;

    if (dev == NULL)
    {
        return DRIVER_ERR_INVALID_ARG;
    }

    millivolts = prv_limit_millivolts(dev, millivolts);
    raw        = dac_millivolts_to_raw(dev, millivolts);

    return dac_write_raw(dev, raw);
}

driver_status_t dac_write_percent(dac_t *dev, uint8_t percent)
{
    uint16_t raw;

    if (dev == NULL)
    {
        return DRIVER_ERR_INVALID_ARG;
    }

    if (percent > 100u)
    {
        percent = 100u;
    }

    raw = (uint16_t)(((uint32_t)percent * dev->max_raw) / 100u);

    return dac_write_raw(dev, raw);
}

uint16_t dac_millivolts_to_raw(dac_t *dev, uint16_t millivolts)
{
    if (dev == NULL || dev->vref_millivolts == 0u)
    {
        return 0u;
    }

    millivolts = prv_limit_millivolts(dev, millivolts);

    return (uint16_t)(((uint32_t)millivolts * dev->max_raw) /
                      dev->vref_millivolts);
}

uint16_t dac_raw_to_millivolts(dac_t *dev, uint16_t raw)
{
    if (dev == NULL || dev->max_raw == 0u)
    {
        return 0u;
    }

    raw = prv_limit_raw(dev, raw);

    return (uint16_t)(((uint32_t)raw * dev->vref_millivolts) /
                      dev->max_raw);
}

uint16_t dac_get_last_raw(dac_t *dev)
{
    if (dev == NULL)
    {
        return 0u;
    }

    return dev->last_raw;
}

uint16_t dac_get_last_millivolts(dac_t *dev)
{
    if (dev == NULL)
    {
        return 0u;
    }

    return dev->last_millivolts;
}

/* Private functions -------------------------------------------------------*/

static uint16_t prv_get_max_raw(dac_align_t align)
{
    if (align == DAC_ALIGN_8B_R)
    {
        return DAC_8BIT_MAX_RAW;
    }

    return DAC_12BIT_MAX_RAW;
}

static uint16_t prv_limit_raw(dac_t *dev, uint16_t raw)
{
    if (raw > dev->max_raw)
    {
        raw = dev->max_raw;
    }

    return raw;
}

static uint16_t prv_limit_millivolts(dac_t *dev, uint16_t millivolts)
{
    if (millivolts > dev->vref_millivolts)
    {
        millivolts = dev->vref_millivolts;
    }

    return millivolts;
}
