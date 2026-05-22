# Simulation Layer Standard (Tầng Tích Hợp)

Lớp Simulation kết nối Driver và Sensor để tạo thành một module chạy.

## 1. Nguyên tắc "Self-contained"

Mỗi module mô phỏng phải là một thực thể độc lập. Người dùng chỉ cần gọi một hàm `init` duy nhất để toàn bộ module tự khởi động và chạy ngầm.

*   **Tự khởi tạo**: Tự gọi `setup`, `enable` cho các Driver cần thiết bên trong hàm `init`.
*   **Tự quản lý Task**: Tự tạo RTOS Task để xử lý logic định kỳ hoặc đợi sự kiện.
*   **Tự xử lý ngắt**: Các hàm ISR của MCU nên được định nghĩa hoặc gọi trực tiếp trong file `.c` của Sim Layer.

## 2. Cấu trúc đối tượng

Struct của Sim Layer gồm các thành phần cần thiết.

```c
typedef struct {
    sensor_logic_t logic;       /* Lớp Sensor */
    driver_t       hw_driver;   /* Lớp Driver */
    TaskHandle_t   task;        /* RTOS Task */
    SemaphoreHandle_t mutex;    /* Bảo vệ dữ liệu */
} sim_object_t;
```

## 3. Luồng hoạt động

1.  **Event Driven**: Driver nhận tín hiệu phần cứng -> Gọi Callback của Sim -> Sim cập nhật trạng thái vào Sensor Logic.
2.  **Polling/Tick**: Sim Task chạy định kỳ -> Gọi hàm `tick()` của Sensor -> Cập nhật đầu ra Driver (DAC/GPIO).

## 4. Quy tắc phát triển

1.  **Encapsulation**: Toàn bộ cấu hình phần cứng (Pins, Timer Instance, DMA Channel) phải được `define` cứng bên trong file `.c` của lớp Sim.
2.  **Thread Safety**: Mọi truy cập vào Sensor Logic từ Task hoặc từ API bên ngoài phải được bảo vệ bởi Mutex.
3.  **Hàm Init duy nhất**: Chỉ cung cấp hàm `sim_prefix_init(void)`.
4.  **Logging**: Lớp Sim chịu trách nhiệm in các log debug ra UART thông qua `printf`.

---
**Ví dụ**:
```c
/* sim_dht11.c */
#define DHT11_PIN  GPIO_Pin_0
#define DHT11_PORT GPIOA

void sim_dht11_init(void) {
    gpio_setup(&g_sim.pin, DHT11_PORT, DHT11_PIN, ...);
    dht11_reset(&g_sim.logic);
    xTaskCreate(prv_sim_task, ...);
}
```
