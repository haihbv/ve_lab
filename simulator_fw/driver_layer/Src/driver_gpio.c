/**
 * @file    driver_gpio.c
 * @brief   GPIO driver implementation.
 */

#include "driver_gpio.h"

/* Public functions --------------------------------------------------------*/

driver_status_t gpio_setup(gpio_t *dev,
                           GPIO_TypeDef *instance,
                           uint16_t pin,
                           GPIOMode_TypeDef mode,
                           GPIOSpeed_TypeDef speed)
{
    if ((dev == NULL) || (instance == NULL)) return DRIVER_ERR_INVALID_ARG;

    dev->instance = instance;
    dev->pin      = pin;

    GPIO_InitTypeDef gpio_init;
    gpio_init.GPIO_Pin   = pin;
    gpio_init.GPIO_Mode  = mode;
    gpio_init.GPIO_Speed = speed;

    GPIO_Init(instance, &gpio_init);

    return DRIVER_OK;
}

driver_status_t gpio_enable(gpio_t *dev)
{
    if (dev == NULL) return DRIVER_ERR_INVALID_ARG;

    /* GPIO doesn't need explicit enable, keep for layer consistency */
    return DRIVER_OK;
}

driver_status_t gpio_disable(gpio_t *dev)
{
    if (dev == NULL) return DRIVER_ERR_INVALID_ARG;
    return DRIVER_OK;
}

driver_status_t gpio_write_high(gpio_t *dev)
{
    if (dev == NULL) return DRIVER_ERR_INVALID_ARG;
    GPIO_SetBits(dev->instance, dev->pin);
    return DRIVER_OK;
}

driver_status_t gpio_write_low(gpio_t *dev)
{
    if (dev == NULL) return DRIVER_ERR_INVALID_ARG;
    GPIO_ResetBits(dev->instance, dev->pin);
    return DRIVER_OK;
}

driver_status_t gpio_toggle(gpio_t *dev)
{
    if (dev == NULL) return DRIVER_ERR_INVALID_ARG;
    
    /* Direct ODR access for performance */
    dev->instance->ODR ^= dev->pin;
    
    return DRIVER_OK;
}

driver_status_t gpio_read(gpio_t *dev, uint8_t *val)
{
    if ((dev == NULL) || (val == NULL)) return DRIVER_ERR_INVALID_ARG;
    *val = (uint8_t)GPIO_ReadInputDataBit(dev->instance, dev->pin);
    return DRIVER_OK;
}
