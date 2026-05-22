#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "driver_flash.h"
#include "driver_uart.h"
#include "stm32f10x.h"

#define FLASH_TEST_BAUDRATE	115200u

/*
 * Mặc định dùng page cuối flash để giảm khả năng đụng vùng code.
 * Có thể override bằng -DFLASH_TEST_ADDR=... khi build.
 */
#ifndef FLASH_TEST_ADDR
#define FLASH_TEST_ADDR \
	0x08008000
#endif

#define FLASH_TEST_PATTERN_LEN	16u
#define FLASH_TEST_ODD_LEN	16u

static uint32_t g_pass_count;
static uint32_t g_fail_count;

static void flash_test_log_case(const char *name, int pass)
{
	if (pass) {
		g_pass_count++;
		printf("[flash-test] PASS: %s\n", name);
	} else {
		g_fail_count++;
		printf("[flash-test] FAIL: %s\n", name);
	}
}

static void flash_test_expect_status(const char *name, driver_status_t actual,
				     driver_status_t expected)
{
	flash_test_log_case(name, actual == expected);

	if (actual != expected) {
		printf("[flash-test]   actual=%d expected=%d\n",
		       (int)actual, (int)expected);
	}
}

static void flash_test_expect_buffer(const char *name, const uint8_t *actual,
				     const uint8_t *expected, uint16_t len)
{
	uint16_t idx;

	for (idx = 0u; idx < len; idx++) {
		if (actual[idx] != expected[idx]) {
			flash_test_log_case(name, 0);
			printf("[flash-test]   idx=%u actual=0x%02X expected=0x%02X\n",
			       (unsigned int)idx, actual[idx], expected[idx]);
			return;
		}
	}

	flash_test_log_case(name, 1);
}

static void flash_test_run(void)
{
	driver_status_t status;
	uint8_t tx_buf[FLASH_TEST_PATTERN_LEN] = {
		0x10u, 0x21u, 0x32u, 0x43u,
		0x54u, 0x65u, 0x76u, 0x87u,
		0x98u, 0xA9u, 0xBAu, 0xCBu,
		0xDCu, 0xEDu, 0xFEu, 0x0Fu
	};
	uint8_t rx_buf[FLASH_TEST_PATTERN_LEN];
	uint8_t expect_buf[FLASH_TEST_PATTERN_LEN];

	printf("\n[flash-test] start\n");
	printf("[flash-test] test_addr=0x%08lX\n", (unsigned long)FLASH_TEST_ADDR);

	status = flash_init();
	flash_test_expect_status("flash_init", status, DRIVER_OK);

	status = flash_erase(FLASH_TEST_ADDR + 1u, FLASH_DRIVER_PAGE_SIZE);
	flash_test_expect_status("erase_unaligned_addr", status,
				 DRIVER_ERR_INVALID_ARG);

	status = flash_write(FLASH_TEST_ADDR + 1u, tx_buf, FLASH_TEST_PATTERN_LEN);
	flash_test_expect_status("write_unaligned_addr", status,
				 DRIVER_ERR_INVALID_ARG);

	status = flash_write(FLASH_TEST_ADDR, NULL, FLASH_TEST_PATTERN_LEN);
	flash_test_expect_status("write_null_ptr", status, DRIVER_ERR_INVALID_ARG);

	status = flash_erase(FLASH_TEST_ADDR, FLASH_DRIVER_PAGE_SIZE);
	flash_test_expect_status("erase_page", status, DRIVER_OK);

	memset(expect_buf, 0xFF, sizeof(expect_buf));
	memset(rx_buf, 0x00, sizeof(rx_buf));
	flash_read_buffer(FLASH_TEST_ADDR, rx_buf, sizeof(rx_buf));
	flash_test_expect_buffer("erase_verify_0xFF", rx_buf, expect_buf,
				 sizeof(rx_buf));

	status = flash_write(FLASH_TEST_ADDR, tx_buf, FLASH_TEST_PATTERN_LEN);
	flash_test_expect_status("write_pattern", status, DRIVER_OK);

	memset(rx_buf, 0x00, sizeof(rx_buf));
	flash_read_buffer(FLASH_TEST_ADDR, rx_buf, sizeof(rx_buf));
	flash_test_expect_buffer("readback_pattern", rx_buf, tx_buf,
				 sizeof(rx_buf));

	status = flash_erase(FLASH_TEST_ADDR, FLASH_DRIVER_PAGE_SIZE);
	flash_test_expect_status("erase_before_odd_write", status, DRIVER_OK);

	status = flash_write(FLASH_TEST_ADDR, tx_buf, FLASH_TEST_ODD_LEN);
	flash_test_expect_status("write_odd_len", status, DRIVER_OK);

	memset(expect_buf, 0xFF, sizeof(expect_buf));
	memcpy(expect_buf, tx_buf, FLASH_TEST_ODD_LEN);
	memset(rx_buf, 0x00, sizeof(rx_buf));
	flash_read_buffer(FLASH_TEST_ADDR, rx_buf, FLASH_TEST_ODD_LEN + 1u);
	flash_test_expect_buffer("odd_len_pad_ff", rx_buf, expect_buf,
				 FLASH_TEST_ODD_LEN + 1u);

	status = flash_deinit();
	flash_test_expect_status("flash_deinit", status, DRIVER_OK);

	printf("[flash-test] done pass=%lu fail=%lu\n",
	       (unsigned long)g_pass_count, (unsigned long)g_fail_count);
}

int main(void)
{
	SystemInit();
	SystemCoreClockUpdate();

	if (uart1.init(FLASH_TEST_BAUDRATE) != DRIVER_OK) {
		while (1) {
		}
	}

	flash_test_run();

	while (1) {
	}
}

