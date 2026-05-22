# VE-Lab Hardware Autograder System

Hệ thống chấm điểm tự động Embedded Hardware dùng MCU STM32. Hệ thống thực hiện quy trình khép kín từ khâu biên dịch, nạp mã nguồn, cho đến capture tín hiệu thực tế và chấm điểm logic dựa trên dữ liệu thực tế từ thiết bị ngoại vi.

## 1. Cấu trúc thư mục (Directory Structure)

```text
ve_lab/
├── assignment/          # Ngân hàng đề bài (Chỉ chứa file JSON cấu hình)
├── logic/               # Logic Capture & Assessment (Nơi chạy Runner)
│   ├── scripts/
│   │   ├── main.py      # Script điều phối chính
│   │   ├── builder.py   # Module biên dịch Firmware
│   │   ├── sigrok.py    # Module điều khiển Logic Analyzer
│   │   ├── check_logic.py # Module chấm điểm Logic
│   │   └── check_reg.py   # Module kiểm tra Register qua SWD
│   ├── docs/
│   └── result.csv       # Kết quả capture tạm thời
├── simulator_fw/        # Mã nguồn Simulator Firmware (Driver, Sensor, Sim layers)
├── stm32_temp_sim/      # Template dự án Simulator (Makefile/Linker)
├── stm32_temp_stu/      # Template dự án Student (Makefile/Linker)
├── toolchain/           # ARM GNU Toolchain (Compiler)
├── Documentation/       # Tài liệu kiến trúc & Hướng dẫn cài đặt hệ thống
├── reference/           # Các tiêu chuẩn lập trình Firmware (Coding, Driver, Commit)
└── README.md            # File mô tả tổng quan dự án
```

---

## 2. Tài liệu hướng dẫn (Documentation)

### A. Kiến trúc & Vận hành hệ thống
Vui lòng tham khảo các tài liệu chi tiết trong thư mục `Documentation/`:
*   **[Kiến trúc hệ thống (System Architecture)](./Documentation/architecture.md)**: Chi tiết về sơ đồ khối, đấu nối Matrix, UART Backchannel và quy trình 8 bước vận hành.
*   **[Hướng dẫn vận hành (Run Guide)](./Documentation/run_guide.t)**: Cách chạy hệ thống chấm điểm từ command line.
*   **Hướng dẫn cài đặt (Setup Guides)**:
    *   [Cấu hình GitHub Action Runner](./Documentation/setup/action_runner.t)
    *   [Cài đặt Toolchain & ST-Link](./Documentation/setup/arm_gcc_stlink.t)
    *   [Cấu hình Sigrok & Logic Analyzer](./Documentation/setup/sigrok.t)
    *   [Hướng dẫn sử dụng pyOCD](./Documentation/setup/pyocd.t)

### B. Tiêu chuẩn phát triển Firmware
Dành cho người phát triển Simulator hoặc hướng dẫn cho sinh viên:
*   **[Tổng quan tiêu chuẩn (Standards Overview)](./reference/README.md)**: Bảng tra cứu các quy tắc lập trình.
*   **[Quy tắc Coding (Coding Standard)](./reference/coding_standard.md)**: Naming convention, cấu trúc project và Doxygen.
*   **[Quy tắc Driver (Driver Standard)](./reference/layer_driver_standard.md)**: Chuẩn API cho tầng Driver.
*   **[Quy chuẩn Commit (Commit Standard)](./reference/commit_standard.md)**: Cách viết commit message theo Conventional Commits.

---

## 3. Yêu cầu hệ thống (Requirements)

### Phần cứng (Hardware)
- **Controller**: Raspberry Pi 4 Model B.
- **Target MCU**: 2x STM32F103RCT6 (Black Pill).
- **Interconnect**: 3x CH446Q (Analog Crosspoint Switch).
- **Tools**: 1x 8-Channel Logic Analyzer, 2x ST-Link V2.

### Phần mềm (Software)
- `arm-none-eabi-gcc`: Bộ biên dịch cho ARM.
- `sigrok-cli`: Capture và giải mã tín hiệu.
- `pyOCD`: Truy cập và kiểm tra Register qua SWD.
- `st-flash`: Nạp Firmware xuống STM32.
- `libgpiod v2`: Điều khiển chân Reset của MCU.

---
