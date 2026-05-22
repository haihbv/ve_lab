# Sensor Layer Standard (Tầng Logic Cảm Biến)

Lớp Sensor mô phỏng chính xác logic hoạt động của sensor thực tế.

## 1. Nguyên tắc "Pure C"

Lớp Sensor phải hoàn toàn độc lập với môi trường chạy. Điều này giúp code logic có thể chạy trên Simulator MCU hoặc thậm chí là Unit Test trên PC (Visual Studio, GCC).

*   **KHÔNG** include: `stm32...h`, `FreeRTOS.h`, `driver_...h`.
*   **KHÔNG** sử dụng: GPIO, UART, Timer trực tiếp.
*   **CHỈ** sử dụng: `<stdint.h>`, `<stdbool.h>`, `<string.h>`.

## 2. State Machine

Mọi sensor được coi là một state machine nhận đầu vào và sinh đầu ra.

*   **Inputs**: Dữ liệu từ môi trường (Nhiệt độ, độ ẩm giả lập) hoặc tín hiệu từ chân bus (START signal, I2C address).
*   **Outputs**: Luồng bit truyền đi (Dữ liệu DHT11), Trạng thái thanh ghi (DS1307).

## 3. Bộ API chuẩn

| Hàm | Mục đích |
| :--- | :--- |
| `prefix_reset()` | Đưa logic về trạng thái mặc định (mô phỏng lúc vừa cấp nguồn). |
| `prefix_tick()` | Cập nhật logic theo thời gian (VD: giảm áp suất, tăng nhiệt độ từ từ). |
| `prefix_rx()` | Nhận dữ liệu đầu vào (mô phỏng việc Host ghi vào cảm biến). |
| `prefix_tx()` | Lấy dữ liệu đầu ra (mô phỏng việc Host đọc từ cảm biến). |
| `prefix_set()` | Ghi dữ liệu giả lập (VD: `dht11_set_temp`). |
| `prefix_get()` | Đọc dữ liệu hiện tại (VD: `dht11_get_temp`). |

## 4. Quy tắc phát triển

1.  **Raw Data**: Mô phỏng phải chính xác đến mức Byte/Bit. Nếu cảm biến thật gửi 40 bit, Sensor Layer phải tạo ra mảng 5 byte đúng format đó.
2.  **Timing**: Không sử dụng `vTaskDelay` bên trong Sensor. Thời gian được quản lý bởi Sim Layer thông qua việc gọi `tick()` hoặc dựa trên tần số xung nhịp do Sim Layer cung cấp.
3.  **Encapsulation**: Trạng thái nội bộ của cảm biến phải nằm hoàn toàn trong struct `prefix_t`.

---
**Ví dụ**:
```c
/* ds1307.h */
typedef struct {
    uint8_t registers[64];
    uint8_t current_addr;
} ds1307_t;

void ds1307_rx(ds1307_t *dev, uint8_t data);
uint8_t ds1307_tx(ds1307_t *dev);
```
