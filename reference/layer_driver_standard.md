# Driver Layer Standard (Tầng Ngoại Vi MCU)

Lớp Driver cung cấp interface để Simulation Layer tương tác với phần cứng MCU

## 1. Thiết kế Hướng đối tượng (Object-Oriented)

Mọi driver (trừ singleton đặc thù) phải sử dụng cấu trúc Handle-based.

```c
/* Forward declaration trong file .h */
typedef struct uart uart_t;

/* Định nghĩa struct trong file .h */
struct uart {
    USART_TypeDef *instance;
    uart_event_cb callback;
    void          *ctx;
};
```

## 2. Bộ API chuẩn

Tất cả Driver phải triển khai bộ hàm theo quy tắc:

| Hàm | Mục đích |
| :--- | :--- |
| `prefix_setup()` | Khởi tạo cấu hình (Cấp clock, cấu hình thanh ghi, tham số cấu hình). Chưa cho chạy. |
| `prefix_enable()` | Bắt đầu cho phép ngoại vi hoạt động (CMD ENABLE, ngắt). |
| `prefix_disable()` | Dừng ngoại vi (CMD DISABLE). |
| `prefix_handle_irq()` | Hàm xử lý ngắt, được gọi từ ISR thực tế trong file Sim hoặc Startup. |

## 3. Cơ chế Callback & Event

Driver không xử lý logic mà chỉ phát sự kiện thông qua Callback.

*   **Callback Type**: `typedef void (*prefix_event_cb)(void *ctx, const prefix_evt_t *evt);`
*   **Event Payload**: Struct chứa loại sự kiện (`enum`) và dữ liệu đi kèm.

## 4. Quy tắc phát triển (Rules)

1.  **Wrapper**: Driver chỉ nên bọc thư viện ngoại vi (SPL). Không chứa logic mô phỏng cảm biến.
2.  **Trạng thái**: Driver phải quản lý trạng thái của ngoại vi để tránh khởi tạo lặp lại hoặc lỗi xung đột.
3.  **Return Type**: Mọi hàm setup/write/read phải trả về `driver_status_t` (`DRIVER_OK`, `DRIVER_ERR_...`).
4.  **IRQ Isolation**: Driver cung cấp hàm `handle_irq()`, nhưng không được tự ý khai báo tên ISR của hệ thống (như `USART1_IRQHandler`) để tránh trùng lặp khi nhiều module dùng chung một loại ngoại vi.

---
**Ví dụ Naming**:
*   `uart_setup(uart_t *dev, USART_TypeDef *instance, uint32_t baud_rate);`
*   `spi_tx_rx(spi_t *dev, uint8_t *tx_data, uint8_t *rx_data, uint16_t len);`
