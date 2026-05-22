/**
 * @file    driver_i2c_slave.c
 * @brief   I2C slave driver implementation.
 * @details Implements interrupt-based I2C slave communication APIs.
 */

/* Includes ----------------------------------------------------------------*/
#include "driver_i2c_slave.h"

#include <string.h>

/* Private macros ----------------------------------------------------------*/

/* Private function prototypes ---------------------------------------------*/
static void    prv_emit_event(i2c_slave_t *dev, i2c_slave_event_t type, uint8_t data, uint32_t error);
static uint8_t prv_get_tx_byte(i2c_slave_t *dev);

/* Public functions --------------------------------------------------------*/

/**
 * @brief Setup I2C slave driver object.
 */
driver_status_t i2c_slave_setup(i2c_slave_t *dev,
                                I2C_TypeDef *instance,
                                uint16_t own_address,
                                uint32_t clock_speed,
                                uint16_t duty_cycle)
{
    if (dev == NULL)
    {
        return DRIVER_ERR_INVALID_ARG;
    }

    memset(dev, 0, sizeof(*dev));

    dev->instance = instance;
    dev->own_address = own_address;
    dev->clock_speed = clock_speed;
    dev->duty_cycle = duty_cycle;
    dev->state = I2C_SLAVE_STATE_IDLE;

    return DRIVER_OK;
}

/**
 * @brief Enable I2C slave peripheral.
 */
driver_status_t i2c_slave_enable(i2c_slave_t *dev)
{
    if ((dev == NULL) || (dev->instance == NULL))
    {
        return DRIVER_ERR_INVALID_ARG;
    }

    I2C_InitTypeDef i2c_init;

    i2c_init.I2C_Mode = I2C_Mode_I2C;
    i2c_init.I2C_ClockSpeed = dev->clock_speed;
    i2c_init.I2C_DutyCycle = dev->duty_cycle;
    i2c_init.I2C_OwnAddress1 = (uint16_t)(dev->own_address << 1);
    i2c_init.I2C_Ack = I2C_Ack_Enable;
    i2c_init.I2C_AcknowledgedAddress = I2C_AcknowledgedAddress_7bit;

    I2C_Init(dev->instance, &i2c_init);
    I2C_AcknowledgeConfig(dev->instance, ENABLE);

    /* Enable event, buffer, and error interrupts. */
    I2C_ITConfig(dev->instance, I2C_IT_EVT | I2C_IT_BUF | I2C_IT_ERR, ENABLE);

    I2C_Cmd(dev->instance, ENABLE);

    dev->state = I2C_SLAVE_STATE_IDLE;

    return DRIVER_OK;
}

/**
 * @brief Disable I2C slave peripheral.
 */
driver_status_t i2c_slave_disable(i2c_slave_t *dev)
{
    if ((dev == NULL) || (dev->instance == NULL))
    {
        return DRIVER_ERR_INVALID_ARG;
    }

    I2C_ITConfig(dev->instance, I2C_IT_EVT | I2C_IT_BUF | I2C_IT_ERR, DISABLE);
    I2C_Cmd(dev->instance, DISABLE);

    dev->state = I2C_SLAVE_STATE_IDLE;

    return DRIVER_OK;
}

/**
 * @brief Register I2C slave event callback.
 */
driver_status_t i2c_slave_register_event(i2c_slave_t *dev,
                                         i2c_slave_event_cb cb,
                                         void *ctx)
{
    if (dev == NULL)
    {
        return DRIVER_ERR_INVALID_ARG;
    }

    dev->event_cb = cb;
    dev->event_ctx = ctx;

    return DRIVER_OK;
}

/**
 * @brief Register I2C slave TX byte callback.
 */
driver_status_t i2c_slave_register_tx(i2c_slave_t *dev,
                                      i2c_slave_tx_cb cb,
                                      void *ctx)
{
    if (dev == NULL)
    {
        return DRIVER_ERR_INVALID_ARG;
    }

    dev->tx_cb = cb;
    dev->tx_ctx = ctx;

    return DRIVER_OK;
}

/**
 * @brief Handle I2C slave event interrupt.
 */
void i2c_slave_handle_ev(i2c_slave_t *dev)
{
    if ((dev == NULL) || (dev->instance == NULL))
    {
        return;
    }

    I2C_TypeDef *i2c = dev->instance;
    uint16_t sr1 = i2c->SR1;

    /* ADDR: address matched. Read SR1 then SR2 to clear ADDR flag. */
    if ((sr1 & I2C_SR1_ADDR) != 0u)
    {
        /* Workaround:
         * Phải ghi byte đầu tiên vào DR trước khi đọc SR2 (giải phóng SCL).
         * Nếu là Master Write, HW sẽ bỏ qua lệnh ghi này và pointer sẽ bị đè lại
         * khi nhận byte địa chỉ thanh ghi.
         */
        uint8_t tx_data = prv_get_tx_byte(dev);
        i2c->DR = tx_data;

        uint16_t sr2 = i2c->SR2; // ADDR bị xóa ở đây. SCL nhả ra. DR đã có data.

        if ((sr2 & I2C_SR2_TRA) != 0u)
        {
            dev->state = I2C_SLAVE_STATE_TX;
            prv_emit_event(dev, I2C_SLAVE_EVT_READ_START, 0u, 0u);
            prv_emit_event(dev, I2C_SLAVE_EVT_BYTE_SENT, tx_data, 0u);
        }
        else
        {
            dev->state = I2C_SLAVE_STATE_RX;
            prv_emit_event(dev, I2C_SLAVE_EVT_WRITE_START, 0u, 0u);
        }

        return;
    }

    /* STOPF: stop detected. Read SR1 then write CR1 to clear STOPF flag. */
    if ((sr1 & I2C_SR1_STOPF) != 0u)
    {
        (void)i2c->SR1;
        /* Write CR1 to clear STOPF */
        i2c->CR1 = (uint16_t)(I2C_CR1_PE | I2C_CR1_ACK);

        if (dev->state == I2C_SLAVE_STATE_RX)
        {
            prv_emit_event(dev, I2C_SLAVE_EVT_WRITE_DONE, 0u, 0u);
        }

        dev->state = I2C_SLAVE_STATE_IDLE;

        return;
    }

    /* RXNE: receive buffer not empty. */
    if ((sr1 & I2C_SR1_RXNE) != 0u)
    {
        uint8_t rx_data = (uint8_t)i2c->DR;

        if (dev->state != I2C_SLAVE_STATE_RX)
        {
            dev->state = I2C_SLAVE_STATE_RX;
        }

        prv_emit_event(dev, I2C_SLAVE_EVT_BYTE_RECEIVED, rx_data, 0u);

        return;
    }

    /* TXE: transmit buffer empty. */
    if ((sr1 & I2C_SR1_TXE) != 0u)
    {
        uint8_t tx_data;

        if (dev->state != I2C_SLAVE_STATE_TX)
        {
            dev->state = I2C_SLAVE_STATE_TX;
        }

        tx_data = prv_get_tx_byte(dev);
        i2c->DR = tx_data;

        prv_emit_event(dev, I2C_SLAVE_EVT_BYTE_SENT, tx_data, 0u);

        return;
    }

    /* BTF: byte transfer finished. */
    if ((sr1 & I2C_SR1_BTF) != 0u)
    {
        if (dev->state == I2C_SLAVE_STATE_TX)
        {
            /* BTF in TX: DR is empty and shift register just finished.
             * Clock is stretched — must load next byte immediately. */
            uint8_t tx_data = prv_get_tx_byte(dev);
            i2c->DR = tx_data;
            prv_emit_event(dev, I2C_SLAVE_EVT_BYTE_SENT, tx_data, 0u);
        }
        else if (dev->state == I2C_SLAVE_STATE_RX)
        {
            uint8_t rx_data = (uint8_t)i2c->DR;

            prv_emit_event(dev, I2C_SLAVE_EVT_BYTE_RECEIVED, rx_data, 0u);
        }

        return;
    }
}

/**
 * @brief Handle I2C slave error interrupt.
 */
void i2c_slave_handle_er(i2c_slave_t *dev)
{
    if ((dev == NULL) || (dev->instance == NULL))
    {
        return;
    }

    I2C_TypeDef *i2c = dev->instance;
    uint32_t error = 0u;

    /* AF: acknowledge failure. */
    if ((i2c->SR1 & I2C_SR1_AF) != 0u)
    {
        i2c->SR1 &= (uint16_t)~I2C_SR1_AF;

        /* Hardware clears ACK bit on AF, must re-enable */
        i2c->CR1 |= I2C_CR1_ACK;

        if (dev->state == I2C_SLAVE_STATE_TX)
        {
            /* Master sends NACK to indicate end of read transaction. */
            prv_emit_event(dev, I2C_SLAVE_EVT_READ_DONE, 0u, 0u);
            dev->state = I2C_SLAVE_STATE_IDLE;
        }
        else
        {
            error |= I2C_SR1_AF;
        }
    }

    /* BERR: bus error. */
    if ((i2c->SR1 & I2C_SR1_BERR) != 0u)
    {
        i2c->SR1 &= (uint16_t)~I2C_SR1_BERR;
        error |= I2C_SR1_BERR;
    }

    /* ARLO: arbitration lost. */
    if ((i2c->SR1 & I2C_SR1_ARLO) != 0u)
    {
        i2c->SR1 &= (uint16_t)~I2C_SR1_ARLO;
        error |= I2C_SR1_ARLO;
    }

    /* OVR: overrun or underrun. */
    if ((i2c->SR1 & I2C_SR1_OVR) != 0u)
    {
        i2c->SR1 &= (uint16_t)~I2C_SR1_OVR;
        error |= I2C_SR1_OVR;
    }

    if (error != 0u)
    {
        dev->state = I2C_SLAVE_STATE_IDLE;
        prv_emit_event(dev, I2C_SLAVE_EVT_ERROR, 0u, error);
    }
}

/* Private functions -------------------------------------------------------*/

/**
 * @brief Emit an I2C slave event to the upper layer.
 */
static void prv_emit_event(i2c_slave_t *dev,
                           i2c_slave_event_t type,
                           uint8_t data,
                           uint32_t error)
{
    if ((dev == NULL) || (dev->event_cb == NULL))
    {
        return;
    }

    i2c_slave_evt_t evt;

    evt.type = type;
    evt.data = data;
    evt.error = error;

    dev->event_cb(dev->event_ctx, &evt);
}

/**
 * @brief Get a byte to transmit from the upper layer.
 */
static uint8_t prv_get_tx_byte(i2c_slave_t *dev)
{
    if ((dev == NULL) || (dev->tx_cb == NULL))
    {
        return 0xFFu;
    }

    return dev->tx_cb(dev->tx_ctx);
}
