# Simulator Firmware Development Standards

Để đảm bảo chất lượng code và tính đồng nhất, hãy tra cứu tài liệu theo bảng hướng dẫn dưới đây:

## 1. Reading Guide

| Khi bạn đang... | Hãy đọc tài liệu này |
| :--- | :--- |
| **Bắt đầu code mới** hoặc **Refactor** | [coding_standard.md](coding_standard.md) |
| **Đặt tên biến, hàm** hoặc **Viết comment** | [coding_standard.md](coding_standard.md) |
| **Phát triển Driver ngoại vi MCU** (I2C, SPI, UART...) | [layer_driver_standard.md](layer_driver_standard.md) |
| **Viết Logic mô phỏng Sensor** (Pure C logic) | [layer_sensor_standard.md](layer_sensor_standard.md) |
| **Tích hợp Driver + Sensor + Task** thành module | [layer_sim_standard.md](layer_sim_standard.md) |
| **Tạo Commit mới** hoặc **Push code** | [commit_standard.md](commit_standard.md) |
| **Sửa file `main.c`** hoặc cấu hình hệ thống | [coding_standard.md](coding_standard.md) |

## 2. Bộ quy tắc

1.  **[coding_standard.md](coding_standard.md)**: Quy tắc chung về Kiến trúc, Naming Convention (Clear Naming), Documentation (Technical English) và Startup Sequence.
2.  **[layer_driver_standard.md](layer_driver_standard.md)**: Các quy định về Object-Oriented Driver, bộ động từ API (`setup/enable/disable`) và cơ chế Callback.
3.  **[layer_sensor_standard.md](layer_sensor_standard.md)**: Các ràng buộc về "Pure C", cấm sử dụng thư viện phần cứng hay RTOS bên trong lớp Sensor.
4.  **[layer_sim_standard.md](layer_sim_standard.md)**: Hướng dẫn cách tạo module "Self-contained", tự quản lý Task và IRQ nội bộ.
5.  **[commit_standard.md](commit_standard.md)**: Quy chuẩn đặt tên commit message theo Conventional Commits.

---
