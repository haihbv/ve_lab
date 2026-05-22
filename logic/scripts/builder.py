#!/usr/bin/env python3
import os
import subprocess
from datetime import datetime

# ─── Config ───────────────────────────────────────────────────────────────────
BASE_DIR        = "/home/pi/ve_lab"
TEMPLATE_DIR    = f"{BASE_DIR}/stm32_temp_stu"
SIM_BUILD_DIR   = f"{BASE_DIR}/stm32_temp_sim"
SIM_SRC_DIR     = f"{BASE_DIR}/simulator_fw"

def log(msg):
    print(f"[{datetime.now().strftime('%H:%M:%S')}][BUILDER] {msg}", flush=True)

def build_student(lab_folder, repo_dir):
    """Biên dịch mã nguồn sinh viên."""
    workspace = os.environ.get("GITHUB_WORKSPACE", repo_dir)
    src = os.path.join(workspace, lab_folder, "main.c")

    if not os.path.exists(src):
        log(f"LOI: Khong tim thay file main.c tai {src}")
        return False

    dst = os.path.join(TEMPLATE_DIR, "Src", "main.c")
    log(f"Copy {src} -> {dst}")
    subprocess.run(["cp", src, dst], check=True)

    log(f"Dang build Student firmware cho {lab_folder}...")
    subprocess.run(["make", "clean"], cwd=TEMPLATE_DIR)
    result = subprocess.run(["make"], cwd=TEMPLATE_DIR)
    
    if result.returncode != 0:
        log("LOI: Build Student FAILED")
        return False

    log("OK: Build Student thanh cong")
    return True

def build_simulator(lab_folder):
    """Bien dich simulator firmware bang cach truyen tham so vao Makefile."""
    sim_build_dir = f"{BASE_DIR}/stm32_temp_sim"

    # Xac dinh flag can enable
    extra_defs = ""
    if "ds1307" in lab_folder.lower():
        extra_defs = 'EXTRA_DEFS="-DENABLE_DS1307=1"'
    elif "dht11" in lab_folder.lower():
        extra_defs = 'EXTRA_DEFS="-DENABLE_DHT11=1"'
    
    log(f"Building Simulator voi cau hinh: {extra_defs if extra_defs else 'DEFAULT'}")
    
    subprocess.run(["make", "clean"], cwd=sim_build_dir)
    # Goi make kem theo tham so EXTRA_DEFS
    cmd = ["make"]
    if extra_defs:
        cmd.append(extra_defs)
        
    result = subprocess.run(cmd, cwd=sim_build_dir)

    if result.returncode != 0:
        log("LOI: Build Simulator FAILED")
        return False
    
    log("OK: Build Simulator thanh cong")
    return True
