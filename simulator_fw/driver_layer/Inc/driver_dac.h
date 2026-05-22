/**
 * @file    driver_dac.h
 * @brief   DAC driver interface.
 * @details Provides APIs to setup, enable, disable, and write DAC output
 *          using raw value, millivolts, or percentage.
 */

#ifndef DRIVER_DAC_H
#define DRIVER_DAC_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ----------------------------------------------------------------*/
#include <stdint.h>
#include <stddef.h>

#include "stm32f10x.h"
#include "driver_common.h"

/* Exported types ----------------------------------------------------------*/

/**
 * @brief DAC data alignment mode.
 */
typedef enum
{
    DAC_ALIGN_12B_R = DAC_Align_12b_R, /**< 12-bit right alignment */
    DAC_ALIGN_12B_L = DAC_Align_12b_L, /**< 12-bit left alignment */
    DAC_ALIGN_8B_R  = DAC_Align_8b_R   /**< 8-bit right alignment */
} dac_align_t;

/**
 * @brief DAC driver state.
 */
typedef enum
{
    DAC_STATE_IDLE = 0, /**< Driver is initialized but not started */
    DAC_STATE_READY,    /**< Driver is stopped and ready to start */
    DAC_STATE_RUNNING   /**< DAC output is enabled */
} dac_state_t;

typedef struct dac dac_t;

/**
 * @brief DAC driver object.
 */
struct dac
{
    DAC_TypeDef *instance;        /**< DAC peripheral instance */
    uint32_t     channel;         /**< DAC channel */
    uint32_t     trigger;         /**< DAC trigger source */
    uint32_t     output_buffer;   /**< DAC output buffer configuration */

    dac_align_t  align;           /**< DAC data alignment mode */

    uint16_t     vref_millivolts; /**< Reference voltage in millivolts */
    uint16_t     max_raw;         /**< Maximum raw value based on alignment */

    uint16_t     last_raw;        /**< Last written raw value */
    uint16_t     last_millivolts; /**< Last written voltage in millivolts */

    dac_state_t  state;           /**< Current driver state */
};

/* Public API --------------------------------------------------------------*/

/**
 * @brief Setup DAC driver object.
 *
 * @param dev Pointer to DAC driver object.
 * @param instance DAC peripheral instance.
 * @param channel DAC channel.
 * @param trigger DAC trigger source.
 * @param output_buffer DAC output buffer configuration.
 * @param align DAC data alignment mode.
 * @param vref_millivolts Reference voltage in millivolts.
 *
 * @return DRIVER_OK if successful, otherwise an error code.
 */
driver_status_t dac_setup(dac_t *dev,
                          DAC_TypeDef *instance,
                          uint32_t channel,
                          uint32_t trigger,
                          uint32_t output_buffer,
                          dac_align_t align,
                          uint16_t vref_millivolts);

/**
 * @brief Enable DAC output.
 *
 * @param dev Pointer to DAC driver object.
 *
 * @return DRIVER_OK if successful, otherwise an error code.
 */
driver_status_t dac_enable(dac_t *dev);

/**
 * @brief Disable DAC output.
 *
 * @param dev Pointer to DAC driver object.
 *
 * @return DRIVER_OK if successful, otherwise an error code.
 */
driver_status_t dac_disable(dac_t *dev);

/**
 * @brief Write raw value to DAC output.
 *
 * @param dev Pointer to DAC driver object.
 * @param raw Raw DAC value.
 *
 * @return DRIVER_OK if successful, otherwise an error code.
 */
driver_status_t dac_write_raw(dac_t *dev, uint16_t raw);

/**
 * @brief Write voltage to DAC output.
 *
 * @param dev Pointer to DAC driver object.
 * @param millivolts Output voltage in millivolts.
 *
 * @return DRIVER_OK if successful, otherwise an error code.
 */
driver_status_t dac_write_millivolts(dac_t *dev, uint16_t millivolts);

/**
 * @brief Write DAC output using percentage.
 *
 * @param dev Pointer to DAC driver object.
 * @param percent Output percentage from 0 to 100.
 *
 * @return DRIVER_OK if successful, otherwise an error code.
 */
driver_status_t dac_write_percent(dac_t *dev, uint8_t percent);

/**
 * @brief Convert millivolts to DAC raw value.
 *
 * @param dev Pointer to DAC driver object.
 * @param millivolts Voltage in millivolts.
 *
 * @return Converted raw value.
 */
uint16_t dac_millivolts_to_raw(dac_t *dev, uint16_t millivolts);

/**
 * @brief Convert DAC raw value to millivolts.
 *
 * @param dev Pointer to DAC driver object.
 * @param raw Raw DAC value.
 *
 * @return Converted voltage in millivolts.
 */
uint16_t dac_raw_to_millivolts(dac_t *dev, uint16_t raw);

/**
 * @brief Get last written raw value.
 *
 * @param dev Pointer to DAC driver object.
 *
 * @return Last written raw value.
 */
uint16_t dac_get_last_raw(dac_t *dev);

/**
 * @brief Get last written voltage in millivolts.
 *
 * @param dev Pointer to DAC driver object.
 *
 * @return Last written voltage in millivolts.
 */
uint16_t dac_get_last_millivolts(dac_t *dev);

#ifdef __cplusplus
}
#endif

#endif /* DRIVER_DAC_H */