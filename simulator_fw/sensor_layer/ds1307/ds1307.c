/**
 * @file    ds1307.c
 * @brief   DS1307 Pure Logic Simulation implementation.
 */

#include "ds1307.h"
#include <string.h>

/* Private macros ----------------------------------------------------------*/

#define DS1307_REG_SECONDS   0x00u
#define DS1307_REG_MINUTES   0x01u
#define DS1307_REG_HOURS     0x02u
#define DS1307_REG_DAY       0x03u
#define DS1307_REG_DATE      0x04u
#define DS1307_REG_MONTH     0x05u
#define DS1307_REG_YEAR      0x06u

#define DS1307_BIT_CH        (1u << 7)
#define DS1307_BIT_12H       (1u << 6)
#define DS1307_BIT_PM        (1u << 5)

#define DS1307_MASK_SECONDS  0x7Fu
#define DS1307_MASK_MINUTES  0x7Fu
#define DS1307_MASK_HOURS_12 0x1Fu
#define DS1307_MASK_HOURS_24 0x3Fu
#define DS1307_MASK_DAY      0x07u
#define DS1307_MASK_DATE     0x3Fu
#define DS1307_MASK_MONTH    0x1Fu

/* Private function prototypes ---------------------------------------------*/

static uint8_t prv_bin_to_bcd(uint8_t val);
static uint8_t prv_bcd_to_bin(uint8_t val);
static uint8_t prv_days_in_month(uint8_t month, uint16_t year);
static void prv_sync_regs_from_time(ds1307_t *dev);
static void prv_sync_time_from_regs(ds1307_t *dev);
static void prv_publish_regs(ds1307_t *dev);

/* Public functions --------------------------------------------------------*/

/**
 * @brief Initialize DS1307 logic with default values.
 */
void ds1307_reset(ds1307_t *dev)
{
    if (dev == NULL) return;

    memset(dev, 0, sizeof(ds1307_t));

    /* Default time: 2026-05-13 14:00:00 */
    dev->current_time.year    = 2026;
    dev->current_time.month   = 5;
    dev->current_time.date    = 13;
    dev->current_time.day     = 3;
    dev->current_time.hour    = 14;
    dev->current_time.minute  = 0;
    dev->current_time.second  = 0;
    dev->current_time.is_12h  = 0;
    
    prv_sync_regs_from_time(dev);
    prv_publish_regs(dev);
}

/**
 * @brief Advance internal clock by 1 second.
 */
void ds1307_tick(ds1307_t *dev)
{
    if (dev == NULL) return;

    /* If CH bit is set, clock is halted */
    if (dev->regs[DS1307_REG_SECONDS] & DS1307_BIT_CH) return;

    ds1307_time_t *t = &dev->current_time;

    if (++t->second > 59)
    {
        t->second = 0;
        if (++t->minute > 59)
        {
            t->minute = 0;
            if (++t->hour > 23)
            {
                t->hour = 0;
                if (++t->day > 7) t->day = 1;
                if (++t->date > prv_days_in_month(t->month, t->year))
                {
                    t->date = 1;
                    if (++t->month > 12)
                    {
                        t->month = 1;
                        if (++t->year > 2099) t->year = 2000;
                    }
                }
            }
        }
    }

    prv_sync_regs_from_time(dev);
    prv_publish_regs(dev);
}

/**
 * @brief DS1307 receives a byte from I2C Master.
 */
void ds1307_rx(ds1307_t *dev, uint8_t data, uint8_t is_first_byte)
{
    if (dev == NULL) return;

    if (is_first_byte)
    {
        dev->current_reg_ptr = data % DS1307_REG_COUNT;
        dev->write_start_reg = dev->current_reg_ptr;
        dev->write_pending = 1;
        dev->write_len = 0;
    }
    else
    {
        if (dev->write_len < DS1307_REG_COUNT)
        {
            dev->write_buf[dev->write_len++] = data;
        }
    }
}

/**
 * @brief DS1307 prepares a byte to transmit to I2C Master.
 */
uint8_t ds1307_tx(ds1307_t *dev)
{
    if (dev == NULL) return 0xFF;

    uint8_t bank = (dev->tx_active) ? dev->tx_bank : dev->active_bank;
    uint8_t data = dev->reg_bank[bank][dev->current_reg_ptr];
    
    /* Auto-increment pointer on read */
    dev->current_reg_ptr = (dev->current_reg_ptr + 1) % DS1307_REG_COUNT;
    
    return data;
}

/**
 * @brief Manually set the time.
 */
void ds1307_set_time(ds1307_t *dev, const ds1307_time_t *time)
{
    if (dev == NULL || time == NULL) return;
    dev->current_time = *time;
    prv_sync_regs_from_time(dev);
    prv_publish_regs(dev);
}

/**
 * @brief Apply pending I2C writes to the logic.
 */
void ds1307_apply_writes(ds1307_t *dev)
{
    if (dev == NULL) return;

    uint8_t reg = dev->write_start_reg;
    for (uint8_t i = 0; i < dev->write_len; i++)
    {
        dev->regs[reg] = dev->write_buf[i];
        reg = (uint8_t)((reg + 1) % DS1307_REG_COUNT);
    }

    if (dev->write_start_reg <= DS1307_REG_YEAR)
    {
        prv_sync_time_from_regs(dev);
    }

    dev->write_pending = 0;
    dev->write_len = 0;
    prv_publish_regs(dev);
}

/**
 * @brief Signal start of an I2C read transaction.
 */
void ds1307_read_start(ds1307_t *dev)
{
    if (dev == NULL) return;
    dev->tx_bank = dev->active_bank;
    dev->tx_active = 1;
}

/**
 * @brief Signal end of an I2C read transaction.
 */
void ds1307_read_done(ds1307_t *dev)
{
    if (dev == NULL) return;
    dev->tx_active = 0;
}

/* Private functions -------------------------------------------------------*/

static uint8_t prv_bin_to_bcd(uint8_t val) { return (uint8_t)(((val / 10) << 4) | (val % 10)); }
static uint8_t prv_bcd_to_bin(uint8_t val) { return (uint8_t)(((val >> 4) * 10) + (val & 0x0F)); }

static uint8_t prv_days_in_month(uint8_t month, uint16_t year)
{
    static const uint8_t days[] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
    if (month == 2 && ((year % 4 == 0 && year % 100 != 0) || (year % 400 == 0))) return 29;
    if (month >= 1 && month <= 12) return days[month - 1];
    return 30;
}

static void prv_sync_regs_from_time(ds1307_t *dev)
{
    uint8_t ch = dev->regs[DS1307_REG_SECONDS] & DS1307_BIT_CH;
    dev->regs[DS1307_REG_SECONDS] = prv_bin_to_bcd(dev->current_time.second) | ch;
    dev->regs[DS1307_REG_MINUTES] = prv_bin_to_bcd(dev->current_time.minute);
    
    if (dev->current_time.is_12h)
    {
        uint8_t h = dev->current_time.hour;
        uint8_t pm = (h >= 12) ? DS1307_BIT_PM : 0;
        if (h > 12) h -= 12;
        if (h == 0) h = 12;
        dev->regs[DS1307_REG_HOURS] = prv_bin_to_bcd(h) | DS1307_BIT_12H | pm;
    }
    else
    {
        dev->regs[DS1307_REG_HOURS] = prv_bin_to_bcd(dev->current_time.hour);
    }

    dev->regs[DS1307_REG_DAY]   = prv_bin_to_bcd(dev->current_time.day);
    dev->regs[DS1307_REG_DATE]  = prv_bin_to_bcd(dev->current_time.date);
    dev->regs[DS1307_REG_MONTH] = prv_bin_to_bcd(dev->current_time.month);
    dev->regs[DS1307_REG_YEAR]  = prv_bin_to_bcd((uint8_t)(dev->current_time.year - 2000));
}

static void prv_sync_time_from_regs(ds1307_t *dev)
{
    dev->current_time.second = prv_bcd_to_bin(dev->regs[DS1307_REG_SECONDS] & DS1307_MASK_SECONDS);
    dev->current_time.minute = prv_bcd_to_bin(dev->regs[DS1307_REG_MINUTES] & DS1307_MASK_MINUTES);
    
    if (dev->regs[DS1307_REG_HOURS] & DS1307_BIT_12H)
    {
        dev->current_time.is_12h = 1;
        dev->current_time.hour = prv_bcd_to_bin(dev->regs[DS1307_REG_HOURS] & DS1307_MASK_HOURS_12);
        if (dev->regs[DS1307_REG_HOURS] & DS1307_BIT_PM) dev->current_time.hour += 12;
    }
    else
    {
        dev->current_time.is_12h = 0;
        dev->current_time.hour = prv_bcd_to_bin(dev->regs[DS1307_REG_HOURS] & DS1307_MASK_HOURS_24);
    }

    dev->current_time.day   = prv_bcd_to_bin(dev->regs[DS1307_REG_DAY] & DS1307_MASK_DAY);
    dev->current_time.date  = prv_bcd_to_bin(dev->regs[DS1307_REG_DATE] & DS1307_MASK_DATE);
    dev->current_time.month = prv_bcd_to_bin(dev->regs[DS1307_REG_MONTH] & DS1307_MASK_MONTH);
    dev->current_time.year  = 2000 + prv_bcd_to_bin(dev->regs[DS1307_REG_YEAR]);
}

static void prv_publish_regs(ds1307_t *dev)
{
    uint8_t next_bank = (dev->active_bank + 1) % DS1307_BANK_COUNT;
    if (next_bank == dev->tx_bank && dev->tx_active)
    {
        next_bank = (next_bank + 1) % DS1307_BANK_COUNT;
    }
    memcpy(dev->reg_bank[next_bank], dev->regs, DS1307_REG_COUNT);
    dev->active_bank = next_bank;
}
