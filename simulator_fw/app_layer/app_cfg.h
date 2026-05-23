#ifndef APP_CFG_H
#define APP_CFG_H

/**
 * @brief Bài lab về cảm biến thời gian thực DS1307.
 */
#ifndef ENABLE_DS1307
#define ENABLE_DS1307 0
#endif

/**
 * @brief Bài lab về đọc cảm biến nhiệt độ, độ ẩm DHT11.
 */
#ifndef ENABLE_DHT11
#define ENABLE_DHT11 0
#endif

/**
 * @brief Bài lab về đọc cảm biến nhiệt độ, độ ẩm DHT22.
 */
#ifndef ENABLE_DHT22
#define ENABLE_DHT22 0
#endif

/**
 * @brief Bài lab về cách sử dụng SPI cho IC74HC595.
 */
#ifndef ENABLE_HC595
#define ENABLE_HC595 0
#endif

/**
 * @brief Bài lab về blink led.
 */
#ifndef ENABLE_BLINK_LED
#define ENABLE_BLINK_LED 1
#endif

#endif /* APP_CFG_H */
