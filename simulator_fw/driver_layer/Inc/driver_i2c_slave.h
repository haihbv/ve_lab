/**
 * @file    driver_i2c_slave.h
 * @brief   I2C slave driver interface.
 * @details Provides APIs to setup, enable, disable, and handle interrupt-based
 *          I2C slave communication.
 */

#ifndef DRIVER_I2C_SLAVE_H
#define DRIVER_I2C_SLAVE_H

#ifdef __cplusplus
extern "C"
{
#endif

/* Includes ----------------------------------------------------------------*/
#include <stdint.h>
#include <stddef.h>

#include "stm32f10x.h"
#include "driver_common.h"

/* Exported macros ---------------------------------------------------------*/

/* Exported types ----------------------------------------------------------*/

/**
 * @brief I2C slave driver state.
 */
typedef enum
{
    I2C_SLAVE_STATE_IDLE = 0, /**< Slave is idle */
    I2C_SLAVE_STATE_RX,       /**< Slave is receiving data */
    I2C_SLAVE_STATE_TX        /**< Slave is transmitting data */
} i2c_slave_state_t;

/**
 * @brief I2C slave event type.
 */
typedef enum
{
    I2C_SLAVE_EVT_WRITE_START = 0, /**< Master starts writing to slave */
    I2C_SLAVE_EVT_READ_START,      /**< Master starts reading from slave */
    I2C_SLAVE_EVT_BYTE_RECEIVED,   /**< One byte received from master */
    I2C_SLAVE_EVT_BYTE_SENT,       /**< One byte sent to master */
    I2C_SLAVE_EVT_WRITE_DONE,      /**< Master write transaction completed */
    I2C_SLAVE_EVT_READ_DONE,       /**< Master read transaction completed */
    I2C_SLAVE_EVT_ERROR            /**< I2C error occurred */
} i2c_slave_event_t;

/**
 * @brief I2C slave event object.
 */
typedef struct
{
    i2c_slave_event_t type;  /**< Event type */
    uint8_t           data;  /**< Event data byte */
    uint32_t          error; /**< Error flags */
} i2c_slave_evt_t;

typedef struct i2c_slave i2c_slave_t;

/**
 * @brief I2C slave event callback function type.
 *
 * @param ctx User context pointer.
 * @param evt Pointer to I2C slave event object.
 */
typedef void (*i2c_slave_event_cb)(void *ctx, const i2c_slave_evt_t *evt);

/**
 * @brief I2C slave TX byte callback function type.
 *
 * @param ctx User context pointer.
 * @return Byte to transmit to I2C master.
 */
typedef uint8_t (*i2c_slave_tx_cb)(void *ctx);

/**
 * @brief I2C slave driver object.
 */
struct i2c_slave
{
    I2C_TypeDef *instance;    /**< I2C peripheral instance */

    uint32_t     clock_speed; /**< I2C peripheral clock speed configuration */
    uint16_t     own_address; /**< 7-bit slave address */
    uint16_t     duty_cycle;  /**< I2C duty cycle */

    volatile i2c_slave_state_t state; /**< Driver state */

    i2c_slave_event_cb event_cb;  /**< Event callback function */
    void              *event_ctx; /**< Event callback user context */

    i2c_slave_tx_cb    tx_cb;      /**< TX byte callback function */
    void              *tx_ctx;     /**< TX byte callback user context */
};

/* Public API --------------------------------------------------------------*/

/**
 * @brief Setup I2C slave driver object.
 *
 * @param dev Pointer to I2C slave driver object.
 * @param instance I2C peripheral instance.
 * @param own_address 7-bit slave address.
 * @param clock_speed I2C clock speed configuration.
 * @param duty_cycle I2C duty cycle.
 *
 * @return DRIVER_OK if successful, otherwise an error code.
 */
driver_status_t i2c_slave_setup(i2c_slave_t *dev,
                                I2C_TypeDef *instance,
                                uint16_t own_address,
                                uint32_t clock_speed,
                                uint16_t duty_cycle);

/**
 * @brief Enable I2C slave peripheral.
 *
 * @param dev Pointer to I2C slave driver object.
 *
 * @return DRIVER_OK if successful, otherwise an error code.
 */
driver_status_t i2c_slave_enable(i2c_slave_t *dev);

/**
 * @brief Disable I2C slave peripheral.
 *
 * @param dev Pointer to I2C slave driver object.
 *
 * @return DRIVER_OK if successful, otherwise an error code.
 */
driver_status_t i2c_slave_disable(i2c_slave_t *dev);

/**
 * @brief Register I2C slave event callback.
 *
 * @param dev Pointer to I2C slave driver object.
 * @param cb Event callback function.
 * @param ctx User callback context.
 *
 * @return DRIVER_OK if successful, otherwise an error code.
 */
driver_status_t i2c_slave_register_event(i2c_slave_t *dev,
                                         i2c_slave_event_cb cb,
                                         void *ctx);

/**
 * @brief Register I2C slave TX byte callback.
 *
 * @param dev Pointer to I2C slave driver object.
 * @param cb TX byte callback function.
 * @param ctx User callback context.
 *
 * @return DRIVER_OK if successful, otherwise an error code.
 */
driver_status_t i2c_slave_register_tx(i2c_slave_t *dev,
                                      i2c_slave_tx_cb cb,
                                      void *ctx);

/**
 * @brief Handle I2C slave event interrupt.
 *
 * @note Must be called from I2C event IRQ handler.
 *
 * @param dev Pointer to I2C slave driver object.
 */
void i2c_slave_handle_ev(i2c_slave_t *dev);

/**
 * @brief Handle I2C slave error interrupt.
 *
 * @note Must be called from I2C error IRQ handler.
 *
 * @param dev Pointer to I2C slave driver object.
 */
void i2c_slave_handle_er(i2c_slave_t *dev);

#ifdef __cplusplus
}
#endif

#endif /* DRIVER_I2C_SLAVE_H */
