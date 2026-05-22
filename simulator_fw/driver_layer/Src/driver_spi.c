/**
 * @file    driver_spi.c
 * @brief   SPI driver implementation.
 */

#include "driver_spi.h"
#include <string.h>

/* Private function prototypes ---------------------------------------------*/

static driver_status_t prv_wait_flag(SPI_TypeDef *instance,
                                     uint16_t flag,
                                     FlagStatus status,
                                     uint32_t timeout);

/* Public functions --------------------------------------------------------*/

driver_status_t spi_setup(spi_t *dev, 
                          SPI_TypeDef *instance, 
                          uint16_t mode,
                          uint16_t baudrate_prescaler,
                          uint16_t cpol,
                          uint16_t cpha,
                          uint16_t first_bit)
{
    if ((dev == NULL) || (instance == NULL))
    {
        return DRIVER_ERR_INVALID_ARG;
    }

    memset(dev, 0, sizeof(*dev));
    dev->instance = instance;

    SPI_InitTypeDef spi_init;
    spi_init.SPI_Direction         = SPI_Direction_2Lines_FullDuplex;
    spi_init.SPI_Mode              = mode;
    spi_init.SPI_DataSize          = SPI_DataSize_8b;
    spi_init.SPI_CPOL              = cpol;
    spi_init.SPI_CPHA              = cpha;
    spi_init.SPI_NSS               = SPI_NSS_Soft;
    spi_init.SPI_BaudRatePrescaler = baudrate_prescaler;
    spi_init.SPI_FirstBit          = first_bit;
    spi_init.SPI_CRCPolynomial     = 7;

    SPI_Init(dev->instance, &spi_init);

    return DRIVER_OK;
}

driver_status_t spi_enable(spi_t *dev)
{
    if ((dev == NULL) || (dev->instance == NULL))
    {
        return DRIVER_ERR_INVALID_ARG;
    }

    SPI_Cmd(dev->instance, ENABLE);

    return DRIVER_OK;
}

driver_status_t spi_disable(spi_t *dev)
{
    if ((dev == NULL) || (dev->instance == NULL))
    {
        return DRIVER_ERR_INVALID_ARG;
    }

    SPI_Cmd(dev->instance, DISABLE);

    return DRIVER_OK;
}

driver_status_t spi_transmit_receive(spi_t *dev,
                                     uint8_t *tx_data,
                                     uint8_t *rx_data,
                                     uint16_t size,
                                     uint32_t timeout)
{
    if ((dev == NULL) || (dev->instance == NULL) || ((tx_data == NULL) && (rx_data == NULL)))
    {
        return DRIVER_ERR_INVALID_ARG;
    }

    SPI_TypeDef     *hspi = dev->instance;
    driver_status_t  status;

    for (uint16_t i = 0; i < size; i++)
    {
        status = prv_wait_flag(hspi, SPI_I2S_FLAG_TXE, SET, timeout);
        if (status != DRIVER_OK) return status;

        SPI_I2S_SendData(hspi, tx_data ? tx_data[i] : 0xFF);

        status = prv_wait_flag(hspi, SPI_I2S_FLAG_RXNE, SET, timeout);
        if (status != DRIVER_OK) return status;

        uint8_t dummy = (uint8_t)SPI_I2S_ReceiveData(hspi);
        if (rx_data) rx_data[i] = dummy;
    }

    return prv_wait_flag(hspi, SPI_I2S_FLAG_BSY, RESET, timeout);
}

driver_status_t spi_transmit(spi_t *dev, uint8_t *data, uint16_t size, uint32_t timeout)
{
    return spi_transmit_receive(dev, data, NULL, size, timeout);
}

driver_status_t spi_receive(spi_t *dev, uint8_t *data, uint16_t size, uint32_t timeout)
{
    return spi_transmit_receive(dev, NULL, data, size, timeout);
}

void spi_register_callback(spi_t *dev, spi_event_cb cb, void *ctx)
{
    if (dev == NULL) return;
    dev->callback = cb;
    dev->ctx      = ctx;
}

void spi_handle_irq(spi_t *dev)
{
    if ((dev == NULL) || (dev->instance == NULL)) return;

    if (SPI_I2S_GetITStatus(dev->instance, SPI_I2S_IT_OVR) != RESET)
    {
        SPI_I2S_ClearITPendingBit(dev->instance, SPI_I2S_IT_OVR);

        if (dev->callback)
        {
            spi_evt_t evt = {.type = SPI_EVT_ERROR, .error = SPI_I2S_IT_OVR};
            dev->callback(dev->ctx, &evt);
        }
    }
}

/* Private functions -------------------------------------------------------*/

static driver_status_t prv_wait_flag(SPI_TypeDef *instance,
                                     uint16_t flag,
                                     FlagStatus status,
                                     uint32_t timeout)
{
    uint32_t tick = 0;
    while (SPI_I2S_GetFlagStatus(instance, flag) != status)
    {
        tick++;
        if (tick > timeout) return DRIVER_ERR_TIMEOUT;
    }
    return DRIVER_OK;
}