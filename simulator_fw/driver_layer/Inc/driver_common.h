/**
 * @file    driver_common.h
 * @brief   Common definitions for driver layer.
 * @details Provides standard return codes and common types used across all drivers.
 */

#ifndef DRIVER_COMMON_H
#define DRIVER_COMMON_H

#ifdef __cplusplus
extern "C"
{
#endif

/* Exported types ----------------------------------------------------------*/

/**
 * @brief Standard driver status return codes.
 */
typedef enum
{
    DRIVER_OK = 0,                 /**< Operation successful */
    DRIVER_ERR_INVALID_ARG = -1,   /**< Invalid argument provided */
    DRIVER_ERR_TIMEOUT = -2,       /**< Operation timed out */
    DRIVER_ERR_BUSY = -3,          /**< Peripheral is busy */
    DRIVER_ERR_NOT_SUPPORTED = -4, /**< Feature not supported */
    DRIVER_ERR_MEM = -5,           /**< Memory allocation/access error */
    DRIVER_ERR_EMPTY = -6          /**< Resource/Buffer is empty */
} driver_status_t;

#ifdef __cplusplus
}
#endif

#endif /* DRIVER_COMMON_H */
