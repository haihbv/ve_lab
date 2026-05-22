# Cấu trúc Logic Mô phỏng DS1307 (Simulator Logic)

Tài liệu này giải thích chi tiết logic mô phỏng RTC DS1307 trong Firmware, nhằm khắc phục sự cố Race Condition và bất đồng bộ khi giao tiếp qua I2C.

## 1. Bài toán và Thách thức

Khi kết nối với I2C Master (STM32), việc mô phỏng đồng hồ DS1307 bằng phần mềm trên MCU phải đối mặt với 2 vấn đề lớn:

1.  **I2C Burst Write (Ghi liên tiếp):** Master thường cấu hình thời gian bằng cách ghi liên tiếp một chuỗi byte (Giây, Phút, Giờ...). Nếu ta cập nhật thẳng từng byte vào thanh ghi thời gian thực tế trong lúc đồng hồ dưới nền vẫn đang `tick`, thời gian sẽ bị nhiễu loạn cục bộ (VD: phút bị thay đổi nhưng giờ chưa kịp cập nhật, đúng lúc đồng hồ nhảy giây).
2.  **Đọc lệch thời gian (Tearing/Inconsistency):** Quá trình truyền tải I2C mất một khoảng thời gian hữu hình (vài ms). Giả sử lúc `10:59:59`, Master đọc xong byte "Giây" (59). Tích tắc sau đồng hồ nhảy lên `11:00:00`, Master đọc tiếp byte "Phút" (00) và "Giờ" (11). Kết quả Master ghép lại thành `11:00:59` - sai lệch hẳn 1 phút.

Để giải quyết triệt để, tầng Sensor Layer đã triển khai cơ chế **Write Buffering** và **Triple Buffering**.

---

## 2. Cơ chế Đệm Ghi (Write Buffering)

Cơ chế này đảm bảo I2C Master phải ghi xong toàn bộ chuỗi dữ liệu (transaction) thì Firmware mới áp dụng (commit) vào thanh ghi lõi.

### Các biến trạng thái liên quan:
*   `write_start_reg`: Lưu địa chỉ thanh ghi bắt đầu ghi (byte dữ liệu đầu tiên Master gửi).
*   `write_buf[64]`: Khay chứa tạm thời các byte dữ liệu thực tế tiếp theo.
*   `write_len`: Số lượng byte đã được đẩy vào khay chứa.
*   `write_pending`: Cờ báo hiệu đang có một tiến trình ghi dang dở chưa hoàn tất.

### Luồng hoạt động thực tế:
1.  Khi Master bắt đầu phiên ghi (`I2C_SLAVE_EVT_WRITE_START`), cờ `write_pending` và `write_len` được xóa về `0` để sẵn sàng đón luồng dữ liệu mới thông qua `ds1307_i2c_write_start()`.
2.  Khi nhận các byte (`I2C_SLAVE_EVT_BYTE_RECEIVED`), byte đầu tiên được gán cho `write_start_reg` và bật cờ `write_pending = 1`. Các byte theo sau được tuần tự đẩy vào mảng `write_buf` thông qua hàm `ds1307_rx()`.
3.  Khi Master kết thúc phiên ghi, nó không gửi một "byte kết thúc" nào mà tạo ra một **Điều kiện Dừng (STOP Condition)** trên đường dây vật lý. Phần cứng nhận diện và kích hoạt sự kiện `I2C_SLAVE_EVT_WRITE_DONE`. Lúc này, ngắt ISR (Sim Layer) sẽ `Give` (phát tín hiệu) Binary Semaphore `write_sem`. Việc này mang ý nghĩa "thả một lá cờ báo hiệu" chứ không phải là khóa hệ thống.
4.  **Xử lý ngắt trì hoãn (Deferred Interrupt Processing):** Việc copy mảng dữ liệu tốn nhiều chu kỳ máy nên không được làm trong ngắt. Thay vào đó, ngắt chỉ tốn 1µs để thả cờ rồi thoát. Task nền (`prv_sim_task`) đang ngủ ngóng cờ sẽ lập tức bừng tỉnh, gọi hàm `ds1307_apply_writes()` để chuyển an toàn dữ liệu từ `write_buf` sang thanh ghi lõi `regs` và đồng bộ thời gian.

---

## 3. Cơ chế 3 Bộ Đệm Đọc (Triple Buffering)

Lấy cảm hứng từ công nghệ xuất hình đồ họa (V-Sync/Triple Buffering), cơ chế này cung cấp một "khung hình" (snapshot) thời gian tĩnh và nhất quán tuyệt đối cho quá trình đọc của Master, bất chấp việc đồng hồ dưới nền vẫn liên tục trôi.

### Các biến trạng thái liên quan:
*   `reg_bank[3][64]`: 3 mảng thanh ghi độc lập lưu các bản snapshot thời gian.
*   `active_bank`: Trỏ tới mảng chứa bản snapshot thời gian mới nhất, hoàn chỉnh nhất vừa được đồng hồ cập nhật.
*   `tx_bank`: Mảng đã bị "khoá" (lock) riêng lại chỉ để dành cho việc truyền dữ liệu lên I2C.
*   `tx_active`: Cờ báo hiệu quá trình đọc I2C đang diễn ra.

### Luồng hoạt động thực tế:
1.  **Chạy ngầm (Background Tick):** Hàm `ds1307_tick()` chạy mỗi 1s. Tăng thời gian xong, nó gọi `prv_publish_regs()` để copy mảng `regs` hiện tại vào một `reg_bank` mới. Hệ thống sẽ tự động chọn bank tiếp theo, **nhưng bỏ qua bank đang bị đánh dấu là `tx_bank`** nếu cờ `tx_active` đang bật. Sau đó cập nhật con trỏ `active_bank`.
2.  **Bắt đầu Đọc (Read Start):** Khi Master gửi lệnh đọc (`I2C_SLAVE_EVT_READ_START`), hệ thống gọi `ds1307_read_start()`. Hàm này chốt hạ `tx_bank = active_bank` và bật cờ `tx_active = 1`.
3.  **Quá trình Truyền (TX):** Xuyên suốt quá trình Master đọc qua I2C (cho dù Master có delay vài giây giữa các byte), hàm `ds1307_tx()` chỉ trích xuất dữ liệu tĩnh từ `reg_bank[tx_bank]`. Nhờ đó, Master nhận được bộ thời gian đồng nhất của đúng khoảnh khắc nó bắt đầu đọc.
4.  **Kết thúc Đọc (Read Done):** Khi kết thúc truyền (`I2C_SLAVE_EVT_READ_DONE`), hàm `ds1307_read_done()` hạ cờ `tx_active` xuống, giải phóng `tx_bank` về lại vòng luân chuyển.

---

## 4. Phân luồng sự kiện I2C từ Phần cứng đến Logic (Event Routing)

Để đảm bảo kiến trúc đa tầng (Layered Architecture) không bị phá vỡ, tín hiệu I2C vật lý sẽ đi qua 3 lớp (Driver -> Sim -> Sensor) theo đúng chuẩn Real-time:

1.  **Ngắt phần cứng (Hardware IRQ):** Khi I2C1 phát hiện điều kiện Start/Stop hoặc nhận byte, vi điều khiển sinh ngắt `I2C1_EV_IRQHandler()`. Ngắt này chuyển điều khiển cho hàm `i2c_slave_handle_ev()` của tầng Driver.
2.  **Driver phân loại và Phát sự kiện (Emit Event):** 
    * Driver phân tích thanh ghi ngoại vi (VD: Cờ `ADDR` báo hiệu địa chỉ khớp, cờ `RXNE` báo hiệu nhận được byte, `STOPF` báo hiệu STOP). 
    * Driver đóng gói các thông tin này thành struct sự kiện và gọi hàm `prv_emit_event()`. 
    * Chữ **"Emit"** ở đây mang ý nghĩa "phát thanh". Driver không cần biết tầng trên là ai, nó chỉ "ném" dữ liệu qua một Con trỏ hàm (Function Pointer) `event_cb` đã được tầng Sim "đăng ký" (register) từ trước.
3.  **Sim Layer bắt Sự kiện và Định tuyến (Routing):** Tầng Sim (`sim_ds1307.c`) đóng vai trò người "nghe đài". CPU sẽ nhảy qua con trỏ hàm vào thẳng hàm `prv_i2c_event_cb` để xử lý sự kiện vừa được Emit, sau đó gọi vào hàm logic `ds1307_rx()` của tầng Sensor:
    ```c
    case I2C_SLAVE_EVT_BYTE_RECEIVED:
        ds1307_rx(&sim->logic, evt->data, (sim->logic.write_pending == 0 && sim->logic.write_len == 0));
        break;
    ```
4.  **Sensor Layer phân loại Byte:** Tầng Sensor (`ds1307.c`) đón nhận dữ liệu. Dựa vào biến cờ `is_first_byte` được truyền vào, nó quyết định byte này là địa chỉ thanh ghi (cần lưu vào `write_start_reg`) hay là dữ liệu thuần tuý (cần nạp vào khay `write_buf`).

Luồng thiết kế này tách biệt hoàn toàn giữa việc **Hứng dữ liệu (qua ngắt - ISR)** và **Xử lý dữ liệu (qua Task - Thread)**, giữ cho hệ thống không bao giờ bị nghẽn (blocking) hay trễ nhịp I2C.

---

Nhờ sự kết hợp chặt chẽ giữa các cơ chế đệm và phân luồng sự kiện đa tầng này, bộ giả lập DS1307 đạt được độ phản hồi "vật lý" chính xác, Master hoàn toàn không thể nhận ra những hạn chế của một hệ thống xử lý phần mềm phía dưới.

---

## 5. Mô phỏng chi tiết phần cứng (Hardware Register Mimicking)

Bên cạnh giao thức giao tiếp, Sensor Layer còn mô phỏng chính xác cấu trúc thanh ghi phần cứng của DS1307, tiêu biểu nhất là **Chế độ 12H/24H**.

### Vấn đề:
Theo Datasheet của DS1307, thanh ghi Giờ (`0x02`) quy định:
*   **Bit 6 = 0**: Chế độ 24 giờ.
*   **Bit 6 = 1**: Chế độ 12 giờ (lúc này Bit 5 là cờ AM/PM).

### Giải pháp trong Simulator:
Để thuận tiện cho tính toán (ngày, tháng, năm nhuận) trong hàm `tick()`, hệ thống ngầm đếm thời gian bằng biến `hour` hệ 24h (0-23). Việc hỗ trợ cấu hình 12H của Master được giải quyết thông qua cờ `is_12h`:

1.  **Khi Master Ghi thời gian:** Hàm `prv_sync_time_from_regs()` sẽ đọc Bit 6 của byte Giờ mà Master gửi. Nếu Bit 6 = 1, nó gán `is_12h = 1`. Sau đó nó bóc tách số giờ thực tế, chuyển ngược về thang 0-23 để lưu vào `hour`.
2.  **Khi Tick nền hoạt động:** Hàm `tick()` cứ thoải mái đếm giờ từ 0 lên 23.
3.  **Khi xuất lên thanh ghi (`prv_sync_regs_from_time`):** Hệ thống kiểm tra cờ `is_12h`. 
    * Nếu `1`, nó lấy biến `hour` đổi sang hệ 1-12, gài Bit 5 (AM/PM) và Bit 6 (12H) rồi lưu vào thanh ghi cho Master đọc. 
    * Nếu `0`, nó đẩy thẳng biến `hour` vào.

Với thiết kế này, bộ mô phỏng vừa giữ được luồng tính toán ngầm đơn giản, vừa thỏa mãn 100% định dạng dữ liệu khắt khe của phần cứng!
