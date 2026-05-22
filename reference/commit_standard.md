# Project Commit Standard

Dự án tuân theo chuẩn **Conventional Commits** để đảm bảo lịch sử Git tường minh và chuyên nghiệp.

## 1. Cấu trúc Commit Message

```text
<type>(<scope>): <description>

[Optional body]
[Optional footer]
```

### Type (Bắt buộc)
- `feat`: Thêm tính năng mới (New feature).
- `fix`: Sửa lỗi (Bug fix).
- `docs`: Cập nhật tài liệu (Documentation).
- `refactor`: Tái cấu trúc code (không đổi tính năng).
- `perf`: Tối ưu hiệu năng (Performance improvement).
- `chore`: Thay đổi bộ máy build, tool, bảo trì (Maintenance).
- `test`: Thêm hoặc sửa test case.

### Scope (Tùy chọn nhưng khuyến khích)
Xác định khu vực bị ảnh hưởng:
- `driver`: Tầng Driver (prefix_setup, enable...).
- `sim`: Tầng Simulation (sim_xxx_init, Task, IRQ).
- `sensor`: Tầng Logic cảm biến (Pure logic).
- `app`: Tầng Ứng dụng (main.c, app_cfg).
- `sys`: Hệ thống (Linker script, startup, core).

### Description (Bắt buộc)
- Viết ngắn gọn, sử dụng động từ ở thì hiện tại (VD: "add", "fix", "change").
- Không viết hoa chữ cái đầu, không kết thúc bằng dấu chấm.

## 2. Ví dụ minh họa

- `feat(driver): add DMA support for UART transmit`
- `fix(sim): fix timing jitter in DHT11 signal response`
- `docs(reference): update coding standard for periph_init naming`
- `refactor(driver): convert SPI setup to direct parameters`
- `chore: update .gitignore to exclude build artifacts`

## 3. Quy tắc bổ sung
- Nếu thay đổi ảnh hưởng đến nhiều scope, có thể để trống scope: `refactor: global renaming of init structs`.
- Nếu có thay đổi gây lỗi tương thích (Breaking Change), thêm dấu `!` sau type: `feat(driver)!: change setup API to direct parameters`.
- **Viết ngắn gọn, súc tích (Concise & Readable):** Phần Body của commit không nên liệt kê chi tiết vụn vặt từng file hay từng thay đổi nhỏ. Chỉ tóm tắt ý chính sao cho người đọc lướt qua là hiểu ngay bản chất và mục đích của nhóm thay đổi này.
