#!/usr/bin/env python3
"""
EMBEDDED HARDWARE AUTOGRADER - MAIN ORCHESTRATOR
------------------------------------------------
Tac gia: Antigravity AI & Team
Mo ta: Dieu phoi toan bo quy trinh tu Build, Flash den Capture va Cham diem.
Luong thuc thi:
  1. Setup He thong (GPIO, Paths)
  2. Build Firmware (Student & Simulator)
  3. Flash Firmware xuong MCUs
  4. Khoi chay va Kiem tra Register (Init phase)
  5. Capture tin hieu Logic (Execution phase)
  6. Tong hop ket qua và Cham diem
"""

import os
import re
import sys
import time
import json
import argparse
import subprocess
from datetime import datetime

# Import cac module chuc nang
import builder
import sigrok
import check_logic
import check_reg
import gpiod

# ==============================================================================
# SECTION 1: CONFIGURATION & PATHS
# ==============================================================================
BASE_DIR        = "/home/pi/ve_lab"
TEMPLATE_DIR    = f"{BASE_DIR}/stm32_temp_stu"
ASSIGNMENTS_DIR = f"{BASE_DIR}/assignment"
LOGIC_DIR       = f"{BASE_DIR}/logic"

# ST-Link Serials cho tung thiet bi
STLINK_STUDENT  = "37FF71064E573436DB011543"
STLINK_SIM      = "10002D00182D343632525544"

# GPIO Lines (BCM numbering) - Dung cho gpiod
GPIO_STUDENT_LINE = 17   # NRST STM32 Student (tuong ung 529 sysfs)
GPIO_SIM_LINE     = 27   # NRST STM32 Simulator (tuong ung 539 sysfs)

# Thong so mac dinh
CAPTURE_TIME    = 20
REPO_DIR        = os.environ.get("GITHUB_WORKSPACE", BASE_DIR)
CSV_PATH        = f"{LOGIC_DIR}/result.csv"

# Global GPIO Lines
gpio_lines = None

# ==============================================================================
# SECTION 2: UTILITIES & SYSTEM CONTROL
# ==============================================================================

def log(msg):
    """Ghi log kem thoi gian thuc."""
    print(f"[{datetime.now().strftime('%H:%M:%S')}] {msg}", flush=True)

def setup_hardware():
    """Khoi tao cac chan GPIO bang gpiod v2 API."""
    global gpio_lines
    if gpio_lines is not None:
        return

    log("Dang khoi tao phan cung (libgpiod v2 NRST)...")
    try:
        # Request lines theo kieu v2: Dinh nghia settings cho tung line
        settings = gpiod.LineSettings(
            direction=gpiod.line.Direction.OUTPUT,
            output_value=gpiod.line.Value.INACTIVE # Mac dinh la LOW (0) - Giu RESET de dam bao an toan/sync
        )

        # Luu doi tuong request vao bien global
        gpio_lines = gpiod.request_lines(
            "/dev/gpiochip0",
            consumer="autograder",
            config={
                GPIO_STUDENT_LINE: settings,
                GPIO_SIM_LINE: settings
            }
        )
        log("OK: Khoi tao GPIO v2 thanh cong")
    except Exception as e:
        log(f"LOI: Khong the request GPIO v2: {e}")
        sys.exit(1)

def gpio_control(action):
    """Dieu khien trang thai Reset cua ca 2 MCU qua gpiod v2 API."""
    global gpio_lines
    if gpio_lines is None:
        return

    # v2 dung Value.INACTIVE (0) và Value.ACTIVE (1)
    val = gpiod.line.Value.INACTIVE if action == "HOLD" else gpiod.line.Value.ACTIVE

    gpio_lines.set_values({
        GPIO_STUDENT_LINE: val,
        GPIO_SIM_LINE: val
    })

    status = "dang bi giu RESET" if action == "HOLD" else "da duoc NHA (dang chay)"
    log(f"He thong {status}")

# ==============================================================================
# SECTION 3: DEPLOYMENT LOGIC (FLASHING)
# ==============================================================================

def flash_firmware(hex_path, serial, label):
    """Nap file hex xuong MCU qua ST-Link."""
    log(f"Dang nap firmware cho {label}...")
    try:
        result = subprocess.run([
            "st-flash", "--serial", serial,
            "--format=ihex", "write", hex_path
        ], stdout=subprocess.DEVNULL, stderr=subprocess.PIPE)

        if result.returncode == 0:
            log(f"OK: Nap {label} thanh cong")
            return True
        else:
            error_msg = result.stderr.decode()
            log(f"LOI: Nap {label} that bai. Chi tiet: {error_msg}")
            return False
    except Exception as e:
        log(f"LOI: He thong nap gap su co: {e}")
        return False

# ==============================================================================
# SECTION 4: MAIN PIPELINE
# ==============================================================================

def run_autograder(args):
    lab_name = args.lab
    cap_time = args.time

    # Kiem tra xem co dang chay che do manual hay khong
    manual_mode = any([args.build, args.flash, args.reg, args.capture, args.grade])

    # 1. Resolve Lab Config
    # Thu hẹp logic: Tim truc tiep file {lab_name}.json trong ASSIGNMENTS_DIR
    json_path = os.path.join(ASSIGNMENTS_DIR, f"{lab_name}.json")

    if not os.path.exists(json_path):
        log(f"LOI: File cau hinh khong ton tai: {json_path}")
        return

    log(f"Su dung cau hinh: {os.path.basename(json_path)}")

    # 2. Build Phase
    if not manual_mode or args.build:
        log("--- PHASE 1: BUILDING ---")
        if not builder.build_student(lab_name, REPO_DIR): return
        if not builder.build_simulator(lab_name): return

    # 3. Flash Phase
    if not manual_mode or args.flash:
        log("--- PHASE 2: FLASHING ---")
        setup_hardware()
        gpio_control("RELEASE") # Tha reset de ST-Link co the nap code

        sim_hex = "/home/pi/ve_lab/stm32_temp_sim/build/sim.hex"
        success_sim = flash_firmware(sim_hex, STLINK_SIM, "SIMULATOR")

        stu_hex = "/home/pi/ve_lab/stm32_temp_stu/build/stu.hex"
        success_stu = flash_firmware(stu_hex, STLINK_STUDENT, "STUDENT")

        gpio_control("HOLD") # Nap xong thi giu Reset de cho cac phase sau
        if not success_sim or not success_stu: return

    # 4. Init & Register Check Phase
    if not manual_mode or args.reg:
        log("--- PHASE 3: REGISTER CHECK ---")
        setup_hardware()
        with open(json_path, encoding="utf-8") as f:
            cfg = json.load(f)

        gpio_control("RELEASE")
        time.sleep(1.5)
        if cfg.get("register_checks"):
            check_reg.run(cfg["register_checks"], serial=STLINK_STUDENT)

    # 5. Capture Logic Phase
    if not manual_mode or args.capture:
        log("--- PHASE 4: LOGIC CAPTURE ---")
        # Khoi tao runner theo kieu huong doi tuong
        runner = sigrok.SigrokRunner(lab_name=lab_name)

        # Kiem tra thiet bi (tu dong retry neu Pi vua boot)
        if not runner.check_device():
            log("LOI: Khong the tiep tuc vi thiet bi Logic Analyzer chua san sang.")
            return

        setup_hardware()
        gpio_control("HOLD")

        # Xoa ket qua cu
        if os.path.exists(CSV_PATH): os.remove(CSV_PATH)

        # Cau hinh va bat dau Capture
        runner.load_config()
        if not runner.start(capture_time=cap_time):
            log("LOI: Khong the khoi chay sigrok-cli. Vui long kiem tra lai thiet bi.")
            return

        time.sleep(1.5) # Cho phep sigrok on dinh truoc khi chay MCU
        gpio_control("RELEASE")

        # Doi den khi capture xong
        runner.wait()

    # 6. Grading Phase
    if not manual_mode or args.grade:
        log("--- PHASE 5: GRADING ---")
        check_logic.grade(lab_name, json_path, csv_path=CSV_PATH)
        log("Hoan tat qua trinh cham diem.")

# ==============================================================================
# SECTION 5: ENTRY POINT
# ==============================================================================

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Autograder Main Entry")
    parser.add_argument("--lab",     required=True, help="Ten thu muc lab")
    parser.add_argument("--time",    type=int, default=CAPTURE_TIME, help="Thoi gian capture")

    # Manual Testing Flags
    parser.add_argument("--build",   action="store_true", help="Chi thuc hien Build")
    parser.add_argument("--flash",   action="store_true", help="Chi thuc hien Flash")
    parser.add_argument("--reg",     action="store_true", help="Chi thuc hien Register Check")
    parser.add_argument("--capture", action="store_true", help="Chi thuc hien Capture")
    parser.add_argument("--grade",   action="store_true", help="Chi thuc hien Grade")

    args = parser.parse_args()

    run_autograder(args)
