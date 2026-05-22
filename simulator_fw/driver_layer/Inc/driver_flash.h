/**
 * @file    driver_flash.h
 * @brief   Internal Flash driver interface.
 * @details Provides APIs to erase, write, and read from the internal flash memory.
 */

#ifndef DRIVER_FLASH_H
#define DRIVER_FLASH_H

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

/* Exported constants ------------------------------------------------------*/

#ifndef FLASH_BASE_ADDR
#define FLASH_BASE_ADDR  FLASH_BASE     /**< Flash base address */
#endif

#ifndef FLASH_TOTAL_SIZE
#define FLASH_TOTAL_SIZE (64u * 1024u)  /**< Total flash size (64KB for C8T6) */
#endif

#ifndef FLASH_PAGE_SIZE
#define FLASH_PAGE_SIZE  1024u          /**< Flash page size (1KB for C8T6) */
#endif

/* Public API --------------------------------------------------------------*/

/**
 * @brief Setup flash driver.
 *
 * @return DRIVER_OK if successful, otherwise an error code.
 */
driver_status_t flash_setup(void);

/**
 * @brief Enable flash driver (Unlock flash).
 *
 * @return DRIVER_OK if successful, otherwise an error code.
 */
driver_status_t flash_enable(void);

/**
 * @brief Disable flash driver (Lock flash).
 *
 * @return DRIVER_OK if successful, otherwise an error code.
 */
driver_status_t flash_disable(void);

/**
 * @brief Erase a range of flash memory.
 *
 * @param addr Starting address of the range to erase.
 * @param size Size in bytes to erase.
 *
 * @return DRIVER_OK if successful, otherwise an error code.
 */
driver_status_t flash_erase(uint32_t addr, uint32_t size);

/**
 * @brief Write data to flash memory.
 *
 * @param addr Destination flash address.
 * @param data Pointer to source data buffer.
 * @param len Number of bytes to write.
 *
 * @return DRIVER_OK if successful, otherwise an error code.
 */
driver_status_t flash_write(uint32_t addr, const uint8_t *data, uint32_t len);

/**
 * @brief Read data from flash memory.
 *
 * @param addr Source flash address.
 * @param buf Pointer to destination data buffer.
 * @param len Number of bytes to read.
 *
 * @return DRIVER_OK if successful, otherwise an error code.
 */
driver_status_t flash_read(uint32_t addr, uint8_t *buf, uint32_t len);

#ifdef __cplusplus
}
#endif

#endif /* DRIVER_FLASH_H */
