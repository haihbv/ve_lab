# VE-Lab Hardware Autograder System Design Document

Tài liệu thiết kế chi tiết hệ thống chấm điểm tự động Embedded Hardware sử dụng MCU STM32. Hệ thống thực hiện quy trình khép kín từ khâu biên dịch, nạp mã nguồn, cho đến capture tín hiệu thực tế và chấm điểm logic dựa trên dữ liệu thực tế từ thiết bị ngoại vi.

---

## 1. System Architecture (Kiến trúc hệ thống)

Hệ thống được thiết kế theo mô hình điều khiển tập trung, sử dụng một Central Controller để quản lý việc nạp chương trình, định cấu hình phần cứng chuyển mạch tín hiệu (Switching Fabric), giả lập môi trường ngoại vi và thu thập dữ liệu kiểm thử.

### 1.1. Sơ đồ kiến trúc tổng thể (Overall System Flow Diagram)

```mermaid
graph LR
    subgraph "Client Side"
        SV[Sinh viên] -- "1. Git Push main.c" --> GH[GitHub Repository]
    end

    subgraph "Cloud / GitHub Platform"
        GH -- "2. Webhook Trigger" --> GHA[GitHub Actions Engine]
    end

    subgraph "Local Autograder Station"
        GHA -- "3. Dispatch Job" --> Runner[RPi 4B - Self-hosted Runner]

        subgraph "Software Controller"
            Runner -- "4. Execute Orchestrator" --> PY[Python Engine: main.py]
            PY -- "Compile & Flash" --> ST[ST-Link V2]
            PY -- "Configure" --> Matrix[CH446Q Switching Matrix]
            PY -- "Measure" --> LA[Logic Analyzer]
        end

        subgraph "Physical Hardware Layer"
            ST -- "SWD" --> Stu[Student MCU]
            ST -- "SWD" --> Sim[Simulator MCU]
            Stu -- "Signal Flow (I2C/SPI)" --> Matrix
            Matrix -- "Routed Signals" --> Sim
            LA -. "Probe/Capture" .-> Matrix
        end
    end

    PY -- "5. Grade & Logs" --> GHA
    GHA -- "6. Feedback PR / Commit" --> SV
```

### 1.2. Sơ đồ khối kết nối phần cứng (Hardware Interconnection Block Diagram)

```mermaid
graph TB
    subgraph TOP["Control & Debug Layer"]
        direction LR
        ST1["ST-Link V2 #1"]
        Pi["RPi 4B\n(GitHub Runner)"]
        ST2["ST-Link V2 #2"]
        LA["Logic Analyzer\n(8-Channel USB)"]
    end

    subgraph BOT["Signal Layer"]
        direction LR
        StuMCU["Student MCU\nSTM32F103RCT6"]
        Matrix["CH446Q\nSwitching Matrix\n× 3 ICs"]
        SimMCU["Simulator MCU\nSTM32F103RCT6"]
    end

    %% USB Host connections (Blue)
    Pi -- "USB" --> ST1
    Pi -- "USB" --> ST2
    Pi -- "USB" --> LA

    %% SWD Programming Lines (Purple)
    ST1 -- "SWD" --> StuMCU
    ST2 -- "SWD" --> SimMCU

    %% RPi Control Lines
    Pi -- "3-Wire Serial" --> Matrix
    Pi -- "UART Backchannel" --> SimMCU

    %% Signal bus through Matrix (Gray)
    StuMCU -- "GPIO (Variable)" --> Matrix
    Matrix -- "Periph (Fixed)" --> SimMCU

    %% Logic Analyzer Probing (Dashed)
    LA -. "Probe Clamp" .-> Matrix
```

---

## 2. Hardware Architecture (Phần cứng sử dụng)

### 2.1. Danh sách thiết bị & Thông số kỹ thuật
*   **Central Controller (Raspberry Pi 4 Model B):**
    *   Nhiệm vụ: Chạy Self-hosted GitHub Actions Runner, điều phối toàn bộ quy trình chấm điểm.
    *   OS: Linux (Debian-based).
*   **Target MCUs:**
    *   **Student MCU (STM32F103RCT6 Black Pill):** Nạp và thực thi Firmware do sinh viên lập trình.
    *   **Simulator MCU (STM32F103RCT6 Black Pill):** Chạy Simulator Firmware để giả lập các linh kiện ngoại vi/sensor (như DS1307, DHT11, v.v.).
*   **Switching Fabric (3x CH446Q Analog Matrix ICs):**
    *   Cho phép Map chân tín hiệu linh hoạt giữa Student MCU và Simulator MCU mà không cần thay đổi dây cắm vật lý.
*   **Logic Analyzer (8-Channel USB Logic Analyzer):**
    *   Bắt và ghi lại dạng sóng (waveform) của các Bus tín hiệu (I2C, SPI, UART, PWM).
*   **Debuggers (2x ST-Link V2):**
    *   Nạp Firmware độc lập cho Student MCU và Simulator MCU qua giao thức SWD.

### 2.2. Chi tiết đấu nối & Interface

| Loại kết nối | Nguồn (Source) | Đích (Destination) | Mô tả giao tiếp |
| :--- | :--- | :--- | :--- |
| **Matrix Control** | RPi GPIO | 3x CH446Q | Giao tiếp 3-wire Serial (Shared CLK, Shared STB, 3x Independent DATA). |
| **UART Backchannel** | RPi UART | Simulator MCU | Link UART Full-duplex để truyền kịch bản test và đồng bộ hóa trạng thái. |
| **Signal Matrix (Flex)** | Student MCU | Matrix Side A | Các chân GPIO thay đổi tùy thuộc vào cấu hình bài nộp của sinh viên. |
| **Signal Matrix (Fixed)** | Matrix Side B | Simulator MCU | Các chân ngoại vi cố định của Simulator (I2C, SPI, UART, PWM). |
| **Logic Probes** | Logic Analyzer | Fixed Path | Kẹp trực tiếp vào đường truyền tín hiệu giữa Matrix và Simulator MCU. |
| **USB Host** | RPi 4B | Debuggers & LA | Kết nối dữ liệu và cấp nguồn cho 2 ST-Link V2 và Logic Analyzer. |

---

## 3. Software Architecture (Phần mềm sử dụng)

### 3.1. Host Toolchain & System Utilities
Các công cụ hệ thống được cài đặt trên Raspberry Pi dùng để tương tác với phần cứng:
*   **arm-none-eabi-gcc:** Compiler để dịch mã nguồn C của Student và Simulator thành file nhị phân.
*   **st-flash / pyOCD:** Thực hiện nạp chương trình (Flash) xuống MCU qua SWD. Ngoài ra, `pyOCD` được dùng để đọc các thanh ghi (Register) cấu hình trên Student MCU nhằm đánh giá việc cấu hình phần cứng.
*   **sigrok-cli:** Điều khiển Logic Analyzer thực hiện Capture tín hiệu logic trên Bus và xuất ra file dữ liệu `result.csv`.
*   **libgpiod (v2):** Điều khiển các chân GPIO của Raspberry Pi kết nối trực tiếp tới chân NRST (Reset) của các MCU.

### 3.2. Autograder Software Stack (Python Engine)
Toàn bộ logic chấm điểm và điều phối nằm trong thư mục [scripts/](../logic/scripts/):
*   **[main.py](../logic/scripts/main.py):** Bộ điều phối chính (Orchestrator). Quản lý toàn bộ vòng đời của một lượt chấm bài.
*   **[builder.py](../logic/scripts/builder.py):** Quản lý Compile. Tự động liên kết mã nguồn sinh viên với các template tương ứng và biên dịch ra file binary.
*   **[sigrok.py](../logic/scripts/sigrok.py):** Giao tiếp và điều khiển `sigrok-cli` để Capture tín hiệu với Sample Rate và số lượng mẫu cấu hình động.
*   **[check_reg.py](../logic/scripts/check_reg.py):** Kết nối SWD qua `pyOCD` để trực tiếp kiểm tra giá trị các Register ngoại vi của Student MCU.
*   **[check_logic.py](../logic/scripts/check_logic.py):** Parse dữ liệu tín hiệu từ `result.csv` và kiểm tra tính đúng đắn dựa trên các Test Case đã định nghĩa trong file cấu hình bài tập JSON.

### 3.3. Dynamic Simulator Firmware Configuration
Simulator được thiết kế linh hoạt thay vì sử dụng Firmware tĩnh:
*   Mã nguồn sử dụng Preprocessor Flags trong `app_config.h` để bật/tắt các module giả lập tương ứng.
*   [builder.py](../logic/scripts/builder.py) điều khiển quá trình Build thông qua việc truyền biến `EXTRA_DEFS` vào `Makefile`:
    ```bash
    make EXTRA_DEFS="-DENABLE_DS1307=1"
    ```

---

## 4. Flow Sequence (Quy trình vận hành)

Quy trình tự động hóa toàn bộ được thực hiện qua các Phase tuần tự sau:

### 4.1. Sơ đồ tuần tự thực thi (Sequence Diagram)
```mermaid
sequenceDiagram
    autonumber
    actor SV as Sinh viên
    participant RPi as Raspberry Pi (Runner)
    participant MTX as Matrix CH446Q
    participant ST as ST-Link V2
    participant StuMCU as Student MCU
    participant SimMCU as Simulator MCU
    participant LA as Logic Analyzer

    SV->>RPi: Push main.c lên GitHub Actions
    Note over RPi: Phase 1: Build Firmware
    RPi->>RPi: Compile Student & Simulator Firmware (arm-none-eabi-gcc)
    Note over RPi: Phase 2: Setup Matrix & Flash
    RPi->>MTX: Gửi dữ liệu cấu hình Route chân qua 3-wire Serial
    RPi->>ST: Gọi lệnh nạp (pyOCD / st-flash)
    ST->>StuMCU: Flash firmware sinh viên (stu.hex)
    ST->>SimMCU: Flash simulator firmware (sim.hex)
    Note over RPi: Phase 3: Register Check & Run
    RPi->>StuMCU: Delay 1.5s & Đọc Register qua pyOCD SWD
    RPi->>LA: Kích hoạt Capture tín hiệu logic (sigrok-cli)
    RPi->>StuMCU: Nhả Reset (NRST High)
    RPi->>SimMCU: Nhả Reset (NRST High) & Gửi kịch bản qua UART
    StuMCU->>SimMCU: Giao tiếp thực tế (I2C/SPI/UART/PWM)
    LA->>LA: Ghi lại dạng sóng trên Bus
    LA->>RPi: Lưu kết quả ra file result.csv
    Note over RPi: Phase 4: Grading & Feedback
    RPi->>RPi: Chạy check_logic.py so sánh result.csv với File JSON Lab
    RPi->>SV: Xuất kết quả chấm điểm lên GitHub Actions Dashboard
```

### 4.2. Chi tiết các bước vận hành
1.  **TRIGGER:** Sinh viên push bài làm (`main.c`) lên GitHub, kích hoạt GitHub Actions tự động chạy trên Self-hosted Runner (Raspberry Pi 4B).
2.  **BUILD:** Runner tự động gọi [builder.py](../logic/scripts/builder.py) để dịch file bài làm của sinh viên thành `stu.hex` và Simulator Firmware tương ứng thành `sim.hex`.
3.  **MATRIX SETUP:** Runner cấu hình IC chuyển mạch CH446Q để map các chân của Student MCU (ví dụ chân I2C SDA/SCL) nối thông vật lý tới các chân định sẵn trên Simulator MCU.
4.  **FLASH:** Sử dụng ST-Link nạp đồng thời code sinh viên và code giả lập xuống hai MCU riêng biệt.
5.  **INIT CHECK:** Sử dụng SWD thông qua [check_reg.py](../logic/scripts/check_reg.py) để kiểm tra xem sinh viên đã khởi tạo đúng các Register của ngoại vi cần thiết hay chưa.
6.  **SIMULATION & CAPTURE:** Nhả Reset cho cả hai MCU hoạt động đồng thời. Simulator MCU nhận lệnh từ UART để tương tác với Student MCU. Logic Analyzer thực hiện Capture toàn bộ khung truyền tín hiệu trên Bus vật lý và lưu thành `result.csv`.
7.  **GRADING:** [check_logic.py](../logic/scripts/check_logic.py) tiến hành chấm điểm bằng cách phân tích tập tin CSV để kiểm tra Timing, Frame dữ liệu truyền nhận, và định dạng giao thức so với cấu hình chuẩn trong file JSON của bài tập đó.
8.  **FEEDBACK:** Điểm số và logs chi tiết được tổng hợp trả về giao diện GitHub Actions để phản hồi trực quan cho sinh viên.
