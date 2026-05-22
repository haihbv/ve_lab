/**
 * @file    driver_flash.c
 * @brief   Internal Flash driver implementation.
 */

#include "driver_flash.h"

/* Private function prototypes ---------------------------------------------*/

static bool prv_is_valid_address(uint32_t addr);
static bool prv_is_valid_range(uint32_t addr, uint32_t size);
static driver_status_t prv_to_driver_status(FLASH_Status status);

/* Public functions --------------------------------------------------------*/

driver_status_t flash_setup(void)
{
    return DRIVER_OK;
}

driver_status_t flash_enable(void)
{
    FLASH_Unlock();
    FLASH_ClearFlag(FLASH_FLAG_EOP | FLASH_FLAG_PGERR | FLASH_FLAG_WRPRTERR);
    return DRIVER_OK;
}

driver_status_t flash_disable(void)
{
    FLASH_Lock();
    return DRIVER_OK;
}

driver_status_t flash_erase(uint32_t addr, uint32_t size)
{
    if (size == 0u) return DRIVER_OK;
    if (!prv_is_valid_range(addr, size) || (addr % FLASH_PAGE_SIZE) != 0u) return DRIVER_ERR_INVALID_ARG;

    driver_status_t status = flash_enable();
    if (status != DRIVER_OK) return status;

    uint32_t end_addr = addr + size;
    for (uint32_t cur = addr; cur < end_addr; cur += FLASH_PAGE_SIZE)
    {
        status = prv_to_driver_status(FLASH_ErasePage(cur));
        if (status != DRIVER_OK)
        {
            flash_disable();
            return status;
        }
    }

    return flash_disable();
}

driver_status_t flash_write(uint32_t addr, const uint8_t *data, uint32_t len)
{
    if (len == 0u) return DRIVER_OK;
    if (data == NULL || (addr % 2u) != 0u || !prv_is_valid_range(addr, len)) return DRIVER_ERR_INVALID_ARG;

    driver_status_t status = flash_enable();
    if (status != DRIVER_OK) return status;

    for (uint32_t offset = 0u; offset < len; offset += 2u)
    {
        uint16_t halfword = (uint16_t)data[offset] | ((offset + 1u < len) ? ((uint16_t)data[offset + 1u] << 8) : 0xFF00);

        status = prv_to_driver_status(FLASH_ProgramHalfWord(addr + offset, halfword));
        if (status != DRIVER_OK)
        {
            flash_disable();
            return status;
        }

        if (*(volatile uint16_t *)(addr + offset) != halfword)
        {
            flash_disable();
            return DRIVER_ERR_MEM;
        }
    }

    return flash_disable();
}

driver_status_t flash_read(uint32_t addr, uint8_t *buf, uint32_t len)
{
    if (buf == NULL || len == 0u) return DRIVER_ERR_INVALID_ARG;
    if (!prv_is_valid_range(addr, len)) return DRIVER_ERR_INVALID_ARG;

    for (uint32_t offset = 0u; offset < len; offset++)
    {
        buf[offset] = *(volatile uint8_t *)(addr + offset);
    }
    return DRIVER_OK;
}

/* Private functions -------------------------------------------------------*/

static bool prv_is_valid_address(uint32_t addr)
{
    return (addr >= FLASH_BASE_ADDR) && (addr < (FLASH_BASE_ADDR + FLASH_TOTAL_SIZE));
}

static bool prv_is_valid_range(uint32_t addr, uint32_t size)
{
    if (size == 0u || !prv_is_valid_address(addr)) return false;
    uint32_t end_addr = addr + size;
    if (end_addr < addr) return false;
    return end_addr <= (FLASH_BASE_ADDR + FLASH_TOTAL_SIZE);
}

static driver_status_t prv_to_driver_status(FLASH_Status status)
{
    switch (status)
    {
        case FLASH_COMPLETE: return DRIVER_OK;
        case FLASH_BUSY:     return DRIVER_ERR_BUSY;
        case FLASH_TIMEOUT:  return DRIVER_ERR_TIMEOUT;
        default:             return DRIVER_ERR_MEM;
    }
}
