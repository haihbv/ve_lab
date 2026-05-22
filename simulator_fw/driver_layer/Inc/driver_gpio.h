/**
 * @file    driver_gpio.h
 * @brief   GPIO driver interface.
 */

#ifndef DRIVER_GPIO_H
#define DRIVER_GPIO_H

#ifdef __cplusplus
extern "C"
{
#endif

/* Includes ----------------------------------------------------------------*/
#include <stddef.h>

#include "driver_common.h"
#include "stm32f10x.h"

/* Exported types ----------------------------------------------------------*/

typedef struct gpio gpio_t;

/**
 * @brief GPIO driver object.
 */
struct gpio
{
    GPIO_TypeDef *instance; /**< Port (GPIOA, GPIOB, ...) */
    uint16_t      pin;      /**< Pin mask (GPIO_Pin_X) */
};

/* Public API --------------------------------------------------------------*/

/**
 * @brief  Setup GPIO pin mode and speed.
 * @return DRIVER_OK on success.
 */
driver_status_t gpio_setup(gpio_t *dev,
                           GPIO_TypeDef *instance,
                           uint16_t pin,
                           GPIOMode_TypeDef mode,
                           GPIOSpeed_TypeDef speed);

/**
 * @brief  Enable GPIO (Clock gating).
 */
driver_status_t gpio_enable(gpio_t *dev);

/**
 * @brief  Disable GPIO.
 */
driver_status_t gpio_disable(gpio_t *dev);

/**
 * @brief  Write high level (1) to pin.
 */
driver_status_t gpio_write_high(gpio_t *dev);

/**
 * @brief  Write low level (0) to pin.
 */
driver_status_t gpio_write_low(gpio_t *dev);

/**
 * @brief  Toggle pin state.
 */
driver_status_t gpio_toggle(gpio_t *dev);

/**
 * @brief  Read pin input level.
 * @param  val Pointer to store result (0 or 1).
 */
driver_status_t gpio_read(gpio_t *dev, uint8_t *val);

#ifdef __cplusplus
}
#endif

#endif /* DRIVER_GPIO_H */
