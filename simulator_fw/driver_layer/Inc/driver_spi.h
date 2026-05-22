/**
 * @file    driver_spi.h
 * @brief   SPI driver interface.
 */

#ifndef DRIVER_SPI_H
#define DRIVER_SPI_H

#ifdef __cplusplus
extern "C"
{
#endif

/* Includes ----------------------------------------------------------------*/
#include <stdint.h>
#include <stddef.h>

#include "stm32f10x.h"
#include "driver_common.h"

/* Exported types ----------------------------------------------------------*/

/**
 * @brief SPI event types.
 */
typedef enum
{
    SPI_EVT_TX_COMPLETE = 0, /**< Transmission complete */
    SPI_EVT_RX_COMPLETE,    /**< Reception complete */
    SPI_EVT_ERROR           /**< SPI error occurred */
} spi_event_t;

/**
 * @brief SPI event object.
 */
typedef struct
{
    spi_event_t type;  /**< Event type */
    uint32_t    error; /**< Error flags if any */
} spi_evt_t;

/**
 * @brief SPI event callback function type.
 */
typedef void (*spi_event_cb)(void *ctx, const spi_evt_t *evt);

typedef struct spi spi_t;

/**
 * @brief SPI driver object.
 */
struct spi
{
    SPI_TypeDef *instance; /**< SPI peripheral instance */
    spi_event_cb callback; /**< Event callback function */
    void        *ctx;      /**< Event callback user context */
};

/* Public API --------------------------------------------------------------*/

/**
 * @brief Setup SPI driver object.
 *
 * @param dev Pointer to SPI driver object.
 * @param instance SPI peripheral instance.
 * @param mode SPI_Mode_Master or SPI_Mode_Slave.
 * @param baudrate_prescaler SPI baud rate prescaler.
 * @param cpol SPI_CPOL_Low or SPI_CPOL_High.
 * @param cpha SPI_CPHA_1Edge or SPI_CPHA_2Edge.
 * @param first_bit SPI_FirstBit_MSB or SPI_FirstBit_LSB.
 *
 * @return DRIVER_OK if successful, otherwise an error code.
 */
driver_status_t spi_setup(spi_t *dev, 
                          SPI_TypeDef *instance, 
                          uint16_t mode,
                          uint16_t baudrate_prescaler,
                          uint16_t cpol,
                          uint16_t cpha,
                          uint16_t first_bit);

/**
 * @brief Enable SPI peripheral.
 *
 * @param dev Pointer to SPI driver object.
 *
 * @return DRIVER_OK if successful, otherwise an error code.
 */
driver_status_t spi_enable(spi_t *dev);

/**
 * @brief Disable SPI peripheral.
 *
 * @param dev Pointer to SPI driver object.
 *
 * @return DRIVER_OK if successful, otherwise an error code.
 */
driver_status_t spi_disable(spi_t *dev);

/**
 * @brief Blocking SPI transmission.
 *
 * @param dev Pointer to SPI driver object.
 * @param data Pointer to data buffer.
 * @param size Number of bytes to send.
 * @param timeout Timeout in milliseconds.
 *
 * @return DRIVER_OK if successful, otherwise an error code.
 */
driver_status_t spi_transmit(spi_t *dev, uint8_t *data, uint16_t size, uint32_t timeout);

/**
 * @brief Blocking SPI reception.
 *
 * @param dev Pointer to SPI driver object.
 * @param data Pointer to buffer to store received data.
 * @param size Number of bytes to receive.
 * @param timeout Timeout in milliseconds.
 *
 * @return DRIVER_OK if successful, otherwise an error code.
 */
driver_status_t spi_receive(spi_t *dev, uint8_t *data, uint16_t size, uint32_t timeout);

/**
 * @brief Blocking SPI full-duplex transmission and reception.
 *
 * @param dev Pointer to SPI driver object.
 * @param tx_data Pointer to data buffer to send.
 * @param rx_data Pointer to buffer to store received data.
 * @param size Number of bytes to transfer.
 * @param timeout Timeout in milliseconds.
 *
 * @return DRIVER_OK if successful, otherwise an error code.
 */
driver_status_t spi_transmit_receive(spi_t *dev, uint8_t *tx_data, uint8_t *rx_data, uint16_t size, uint32_t timeout);

/**
 * @brief Register SPI event callback.
 *
 * @param dev Pointer to SPI driver object.
 * @param cb Event callback function.
 * @param ctx User callback context.
 */
void spi_register_callback(spi_t *dev, spi_event_cb cb, void *ctx);

/**
 * @brief Handle SPI interrupt.
 *
 * @note Must be called from the corresponding SPI IRQ handler.
 *
 * @param dev Pointer to SPI driver object.
 */
void spi_handle_irq(spi_t *dev);

#ifdef __cplusplus
}
#endif

#endif /* DRIVER_SPI_H */
