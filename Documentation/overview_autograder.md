# Embedded Hardware Autograder - Technical Overview

## 1. Kiến trúc Hệ thống
Hệ thống được thiết kế theo dạng module hóa để tách biệt các trách nhiệm:

- **`main.py`**: Điều phối (Orchestrator). Quản lý luồng chạy tổng thể.
- **`builder.py`**: Quản lý biên dịch. Xử lý việc nạp code sinh viên và cấu hình Simulator.
- **`sigrok.py`**: Quản lý Capture. Giao tiếp với PulseView/sigrok để lấy dữ liệu logic.
- **`check_logic.py` & `check_reg.py`**: Quản lý chấm điểm.

## 2. Cơ chế Dynamic Simulator Configuration
Thay vì sử dụng các file hex cố định, Simulator hiện được build trực tiếp từ mã nguồn mỗi khi chấm bài:

1. **Preprocessor Flags**: File `app_config.h` sử dụng `#ifndef` để cho phép ghi đè cấu hình.
2. **Makefile Integration**: Biến `EXTRA_DEFS` được thêm vào Makefile để nhận các Define từ command line.
3. **Automated Build**: `builder.py` tự động xác định linh kiện cần giả lập (DS1307, DHT11) dựa trên tên lab và gọi:
   ```bash
   make EXTRA_DEFS="-DENABLE_DS1307=1"
   ```

## 3. Quy trình thực thi (Pipeline)
1. **Setup Hardware**: Khởi tạo GPIO NRST để điều khiển Reset MCUs.
2. **Build Student**: Copy `main.c` sinh viên vào template và build ra `stu.hex`.
3. **Build Simulator**: Cấu hình module tương ứng và build ra `sim.hex`.
4. **Flash**: Nạp firmware xuống cả 2 chip STM32.
5. **Register Check**: Cho chip chạy 1.5s, sau đó dùng pyOCD để đọc thanh ghi.
6. **Logic Capture**: Giữ Reset -> Bật Sigrok -> Nhả Reset -> Lưu `result.csv`.
7. **Grading**: So khớp dữ liệu trong `result.csv` với yêu cầu trong file `.json` của bài lab.

## 4. Hướng dẫn Chạy tay (Manual Testing)
Bạn có thể sử dụng `main.py` với các tham số sau để test từng phần:
- `--build`: Chỉ thực hiện biên dịch firmware.
- `--flash`: Chỉ thực hiện nạp firmware (yêu cầu đã có file hex).
- `--capture`: Chỉ thực hiện bắt tín hiệu logic.
- `--grade`: Chỉ thực hiện chấm điểm dựa trên file CSV có sẵn.
- `--lab <ten_lab>`: (Bắt buộc) Xác định bài lab cần thao tác.