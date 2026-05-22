/**
 * @file    driver_uart.h
 * @brief   UART driver interface.
 * @details Provides APIs to setup, enable, disable, and handle interrupt-based
 *          UART communication.
 */

#ifndef DRIVER_UART_H
#define DRIVER_UART_H

#ifdef __cplusplus
extern "C"
{
#endif

/* Includes ----------------------------------------------------------------*/
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#include "stm32f10x.h"
#include "driver_common.h"

/* Exported types ----------------------------------------------------------*/

typedef struct uart uart_t;

/**
 * @brief UART driver object.
 */
struct uart
{
    USART_TypeDef *instance;  /**< USART peripheral instance */
    uint32_t       baud_rate; /**< Baud rate configuration */
    void          *rx_rb;     /**< Pointer to internal RX ring buffer (private) */
};

/* Public API --------------------------------------------------------------*/

/**
 * @brief Setup UART driver object.
 *
 * @param dev Pointer to UART driver object.
 * @param instance USART peripheral instance.
 * @param baud_rate Baud rate configuration.
 *
 * @return DRIVER_OK if successful, otherwise an error code.
 */
driver_status_t uart_setup(uart_t *dev, USART_TypeDef *instance, uint32_t baud_rate);

/**
 * @brief Enable UART peripheral.
 *
 * @param dev Pointer to UART driver object.
 *
 * @return DRIVER_OK if successful, otherwise an error code.
 */
driver_status_t uart_enable(uart_t *dev);

/**
 * @brief Disable UART peripheral.
 *
 * @param dev Pointer to UART driver object.
 *
 * @return DRIVER_OK if successful, otherwise an error code.
 */
driver_status_t uart_disable(uart_t *dev);

/**
 * @brief Write a single character via UART.
 *
 * @param dev Pointer to UART driver object.
 * @param c Character to send.
 *
 * @return DRIVER_OK if successful, otherwise an error code.
 */
driver_status_t uart_write_char(uart_t *dev, char c);

/**
 * @brief Write a string via UART.
 *
 * @param dev Pointer to UART driver object.
 * @param str String to send.
 *
 * @return DRIVER_OK if successful, otherwise an error code.
 */
driver_status_t uart_write_string(uart_t *dev, const char *str);

/**
 * @brief Check if RX data is available.
 *
 * @param dev Pointer to UART driver object.
 * @return true if data is available, false otherwise.
 */
bool uart_read_available(uart_t *dev);

/**
 * @brief Read a single character from RX buffer.
 *
 * @param dev Pointer to UART driver object.
 * @param c Pointer to store the read character.
 *
 * @return DRIVER_OK if successful, otherwise an error code.
 */
driver_status_t uart_read_char(uart_t *dev, char *c);

/**
 * @brief Set the UART instance for printf output.
 * 
 * @param dev Pointer to UART driver object.
 */
void uart_set_debug_output(uart_t *dev);

/**
 * @brief Handle UART interrupt.
 *
 * @note Must be called from USARTx_IRQHandler.
 *
 * @param dev Pointer to UART driver object.
 */
void uart_handle_irq(uart_t *dev);

/* Default Instances -------------------------------------------------------*/

extern uart_t g_uart1; /**< Default UART1 instance */
extern uart_t g_uart2; /**< Default UART2 instance */
extern uart_t g_uart3; /**< Default UART3 instance */

#ifdef __cplusplus
}
#endif

#endif /* DRIVER_UART_H */
