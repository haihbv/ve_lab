#!/usr/bin/env python3
"""
check_reg.py — Kiểm tra cấu hình thanh ghi MCU qua pyOCD (SWD)

Dùng độc lập:
  python3 check_reg.py --lab lab5-i2c
  python3 check_reg.py --lab lab5-i2c --serial 37FF71064E573436DB011543

Import từ checker.py:
  import check_reg
  results = check_reg.run(register_checks, target=..., serial=...)
"""

import json
import re
import os
import argparse

# ─── Config ───────────────────────────────────────────────────────────────────

BASE_DIR         = "/home/pi/ve_lab"
ASSIGNMENTS_DIR  = f"{BASE_DIR}/assignment"
STLINK_STUDENT   = "37FF71064E573436DB011543"
DEFAULT_TARGET   = "stm32f103c8"

# ─── Core ─────────────────────────────────────────────────────────────────────

def run(register_checks, target=DEFAULT_TARGET, serial=STLINK_STUDENT):

    results = []

    # --- STEP 1: System Readiness Check ---
    # Đảm bảo môi trường Python đã có thư viện giao tiếp pyOCD.
    try:
        from pyocd.core.helpers import ConnectHelper
    except ImportError:
        msg = "pyocd module not found. Please install via 'pip install pyocd'."
        print(f" [CRITICAL] {msg}")
        return _all_fail(register_checks, msg)

    if not register_checks:
        print(" [INFO] No registers to check.")
        return []

    print(f"\n{'='*60}")
    print(f" STEP-BY-STEP REGISTER VALIDATION")
    print(f" Target MCU: {target}")
    print(f" Debugger  : {serial}")
    print(f"{'='*60}")

    try:
        # --- STEP 2: Establish SWD Connection ---
        # Khởi tạo phiên làm việc với ST-Link cụ thể thông qua Serial ID.
        print(f" [1/5] Connecting to ST-Link probe...")
        with ConnectHelper.session_with_chosen_probe(
            unique_id=serial,
            options={"target_override": target}
        ) as session:
            t = session.board.target
            
            # --- STEP 3: Enter Debug State ---
            # Dừng CPU (Halt) là bắt buộc để việc đọc thanh ghi ngoại vi không bị nhiễu 
            # hoặc gây ra lỗi truy cập (Bus Fault) khi CPU đang xử lý tác vụ khác.
            print(f" [2/5] Halting CPU for stable register access...")
            t.halt()

            try:
                # --- STEP 4: Execute Register Validations ---
                # Duyệt qua danh sách các thanh ghi cần kiểm tra từ cấu hình JSON bài Lab.
                print(f" [3/5] Executing validation rules...")
                for rc in register_checks:
                    _check_one(t, rc, results)
            
            finally:
                # --- STEP 5: Release Hardware (Resume) ---
                # Luôn luôn phải Resume CPU để chip có thể tiếp tục chạy mã nguồn của sinh viên 
                # ngay sau khi quá trình kiểm tra kết thúc.
                print(f" [4/5] Resuming CPU execution and releasing hardware...")
                t.resume()

    except Exception as e:
        print(f" [CRITICAL] Connection/Communication Failure: {e}")
        # Đánh dấu FAIL cho tất cả các mục chưa kịp kiểm tra do lỗi phần cứng/kết nối.
        checked_ids = {r["id"] for r in results}
        for rc in register_checks:
            if rc["id"] not in checked_ids:
                results.append(_fail_entry(rc, f"Hardware Error: {e}"))

    # --- STEP 6: Generate Assessment Report ---
    # Tổng hợp điểm số dựa trên các kết quả so khớp thành công.
    earned = sum(r["score"] for r in results)
    total  = sum(r["max"]   for r in results)
    print(f"\n [5/5] VALIDATION SUMMARY")
    print(f" TOTAL EARNED: {earned}/{total}")
    print(f"{'='*60}\n")

    return results


def _check_one(target, rc, results):
    """
    Thực hiện kiểm tra cho một thanh ghi cụ thể (Atomic check).
    Quy trình: Đọc dữ liệu thô -> Áp dụng Mask -> So sánh với Expected.
    """
    try:
        addr     = int(rc["address"], 16)
        mask     = int(rc["mask"],    16)
        expected = int(rc["expected"],16)

        # Truy vấn giá trị thực tế từ phần cứng qua SWD
        raw    = target.read32(addr)
        # Loại bỏ các bit không cần quan tâm thông qua phép toán AND với Mask
        actual = raw & mask
        passed = (actual == expected)

        status = "PASS" if passed else "FAIL"
        
        # Tạo chuỗi chi tiết để phục vụ việc debug và lưu log
        detail = (
            f"Addr: {rc['address']} | "
            f"Raw: 0x{raw:08X} | "
            f"Masked: 0x{actual:08X} | "
            f"Expected: 0x{expected:08X}"
        )

        score = rc["points"] if passed else 0
        
        # In kết quả trực quan ra Terminal
        print(f" [{status}] {rc['id']}: {rc['name']}")
        print(f"        {detail}")
        if "comment" in rc:
            print(f"        Note: {rc['comment']}")

        results.append({
            "id":     rc["id"],
            "name":   rc["name"],
            "status": status,
            "score":  score,
            "max":    rc["points"],
            "detail": detail,
        })

    except Exception as e:
        err_msg = f"Read failed: {e}"
        print(f" [ERROR] {rc['id']} ({rc['name']}): {err_msg}")
        results.append(_fail_entry(rc, err_msg))


def _fail_entry(rc, reason):
    """Tạo nhanh một entry kết quả bị đánh trượt (FAIL)."""
    return {
        "id":     rc["id"],
        "name":   rc["name"],
        "status": "FAIL",
        "score":  0,
        "max":    rc["points"],
        "detail": reason,
    }


def _all_fail(register_checks, reason):
    """Đánh trượt toàn bộ danh sách khi có lỗi hệ thống (ví dụ: mất kết nối)."""
    return [_fail_entry(rc, reason) for rc in register_checks]


# ─── Standalone ───────────────────────────────────────────────────────────────

def main():
    parser = argparse.ArgumentParser(description="Register checker via pyOCD")
    parser.add_argument("--lab",    required=True, help="Tên lab (vd: lab5-i2c)")
    parser.add_argument("--serial", default=STLINK_STUDENT, help="STLink serial")
    parser.add_argument("--target", default=DEFAULT_TARGET,  help="pyOCD target")
    parser.add_argument("--json",   default=None, help="Đường dẫn JSON (tùy chọn)")
    args = parser.parse_args()

    # Resolve JSON path (Tim truc tiep file {lab_name}.json trong ASSIGNMENTS_DIR)
    json_path = args.json
    if not json_path:
        json_path = os.path.join(ASSIGNMENTS_DIR, f"{args.lab}.json")

    if not json_path or not os.path.exists(json_path):
        print(f"[ERROR] Không tìm thấy JSON trong thư mục: {args.lab}")
        return

    with open(json_path, encoding="utf-8") as f:
        cfg = json.load(f)

    register_checks = cfg.get("register_checks", [])
    if not register_checks:
        print("[WARN] JSON không có trường 'register_checks'")
        return

    target = cfg.get("target", args.target)
    run(register_checks, target=target, serial=args.serial)


if __name__ == "__main__":
    main()
