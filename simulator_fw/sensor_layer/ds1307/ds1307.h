/**
 * @file    ds1307.h
 * @brief   DS1307 Pure Logic Simulation (No RTOS, No Driver).
 */

#ifndef DS1307_H
#define DS1307_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Exported macros ---------------------------------------------------------*/

#define DS1307_REG_COUNT 64u
#define DS1307_BANK_COUNT 3u

/* Exported types ----------------------------------------------------------*/

/**
 * @brief DS1307 time representation.
 */
typedef struct {
    uint16_t year;   /**< Year (e.g. 2026) */
    uint8_t  month;  /**< Month (1-12) */
    uint8_t  date;   /**< Date (1-31) */
    uint8_t  day;    /**< Day of week (1-7) */
    uint8_t  hour;   /**< Hour (0-23) */
    uint8_t  minute; /**< Minute (0-59) */
    uint8_t  second; /**< Second (0-59) */
    uint8_t  is_12h; /**< 12h format flag */
} ds1307_time_t;

/**
 * @brief DS1307 logic state.
 */
typedef struct {
    /* Core state */
    uint8_t regs[DS1307_REG_COUNT];
    ds1307_time_t current_time;

    /* Simulation State */
    uint8_t current_reg_ptr;
    uint8_t write_pending;
    uint8_t write_buf[DS1307_REG_COUNT];
    uint8_t write_len;
    uint8_t write_start_reg;

    /* Triple Buffering */
    uint8_t reg_bank[DS1307_BANK_COUNT][DS1307_REG_COUNT];
    volatile uint8_t active_bank;
    volatile uint8_t tx_bank;
    volatile uint8_t tx_active; 
} ds1307_t;

/* Public API --------------------------------------------------------------*/

/**
 * @brief Initialize DS1307 logic with default values.
 */
void ds1307_reset(ds1307_t *dev);

/**
 * @brief Advance internal clock by 1 second.
 * @details Call this from a timer or RTOS task every 1s.
 */
void ds1307_tick(ds1307_t *dev);

/**
 * @brief DS1307 receives a byte from I2C Master.
 * @param data The byte received.
 * @param is_first_byte True if this is the first byte (register address).
 */
void ds1307_rx(ds1307_t *dev, uint8_t data, uint8_t is_first_byte);

/**
 * @brief DS1307 prepares a byte to transmit to I2C Master.
 * @return The data byte to be sent.
 */
uint8_t ds1307_tx(ds1307_t *dev);

/**
 * @brief Manually set the time.
 */
void ds1307_set_time(ds1307_t *dev, const ds1307_time_t *time);

/**
 * @brief Apply pending I2C writes to the logic.
 */
void ds1307_apply_writes(ds1307_t *dev);

/**
 * @brief Signal start of an I2C read transaction.
 */
void ds1307_read_start(ds1307_t *dev);

/**
 * @brief Signal end of an I2C read transaction.
 */
void ds1307_read_done(ds1307_t *dev);

#ifdef __cplusplus
}
#endif

#endif /* DS1307_H */
