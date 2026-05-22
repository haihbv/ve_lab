#!/usr/bin/env python3
"""
EMBEDDED HARDWARE AUTOGRADER — LOGIC ANALYZER ENGINE (v2.0)
-----------------------------------------------------------
Mô tả: Công cụ capture và decode tín hiệu Logic tự động cho hệ thống Autograder.
Hỗ trợ: I2C, UART, SPI, PWM, DHT11.

CÁC BƯỚC THỰC HIỆN (PIPELINE):
    BƯỚC 1: Kiểm tra kết nối phần cứng (Device Scan)
    BƯỚC 2: Load cấu hình Lab và Mapping các kênh (Pinout)
    BƯỚC 3: Khởi tạo tiến trình sigrok-cli (Non-blocking)
    BƯỚC 4: Xử lý luồng dữ liệu Real-time (Regex Decoding)
    BƯỚC 5: Tổng hợp dữ liệu và xuất file kết quả (CSV)
"""

import subprocess
import re
import time
import sys
import threading
import csv
import json
import os
import argparse
from itertools import zip_longest
from datetime import datetime

# ==============================================================================
# SECTION 1: CẤU HÌNH HỆ THỐNG (SYSTEM CONFIG)
# ==============================================================================

class Config:
    # Định nghĩa mapping các chân vật lý (D0-D7) sang tên ngoại vi
    CHANNEL_MAP = {
        "i2c_scl":  "D0",
        "i2c_sda":  "D1",
        "uart_tx":  "D2",
        "dht11":    "D3",
        "spi_mosi": "D4",
        "spi_clk":  "D5",
        "spi_cs":   "D6",
        "pwm":      "D7",
    }

    # Đường dẫn và thông số kỹ thuật
    BASE_DIR        = "/home/pi/ve_lab"
    ASSIGNMENTS_DIR = f"{BASE_DIR}/assignment"
    CSV_PATH        = f"{BASE_DIR}/logic/result.csv"
    SAMPLERATE      = "2M"
    DEFAULT_TIME    = 30

    # Cấu hình Decoder mặc định nếu không có file JSON
    DEFAULT_DECODERS = [
        f"i2c:scl={CHANNEL_MAP['i2c_scl']}:sda={CHANNEL_MAP['i2c_sda']}",
        f"uart:baudrate=115200:tx={CHANNEL_MAP['uart_tx']}:format=hex",
        f"pwm:data={CHANNEL_MAP['pwm']}",
        f"spi:mosi={CHANNEL_MAP['spi_mosi']}:clk={CHANNEL_MAP['spi_clk']}:cs={CHANNEL_MAP['spi_cs']}:wordsize=8:cpol=0:cpha=0",
        f"timing:data={CHANNEL_MAP['i2c_scl']}",
        f"timing:data={CHANNEL_MAP['spi_clk']}",
        f"timing:data={CHANNEL_MAP['dht11']}",
    ]

# ==============================================================================
# SECTION 2: GIẢI MÃ GIAO THỨC ĐẶC THÙ (CUSTOM DECODERS)
# ==============================================================================

class ProtocolDecoders:
    @staticmethod
    def decode_dht11(pulses_us):
        """Giải mã chuỗi xung thời gian thành dữ liệu DHT11 (40 bits)."""
        if len(pulses_us) < 43: return None

        idx = 3 # Bỏ qua Start/Response bits
        bits = []
        while idx < len(pulses_us) - 1 and len(bits) < 40:
            low_p  = pulses_us[idx]
            high_p = pulses_us[idx + 1]
            if 35 < low_p < 75:
                bits.append(1 if high_p > 42 else 0)
                idx += 2
            else: idx += 1

        if len(bits) < 40: return None

        data = [0] * 5
        for i in range(40):
            if bits[i]: data[i // 8] |= (1 << (7 - (i % 8)))

        # Kiểm tra Checksum
        if (sum(data[:4]) & 0xFF) != data[4]: return None
        return f"{data[0]:02X} {data[1]:02X} {data[2]:02X} {data[3]:02X} {data[4]:02X}"

# ==============================================================================
# SECTION 3: CÔNG CỤ ĐIỀU KHIỂN SIGROK (SIGROK ENGINE)
# ==============================================================================

class SigrokRunner:
    def __init__(self, lab_name=None):
        self.lab_name = lab_name
        self.proc = None
        self.thread = None
        self.decoders = []
        self.timing_channels = []

        # Trạng thái capture (State management)
        self.base_time = None
        self.state = {
            "freq_i2c": "--", "freq_spi": "--",
            "uart_buf": "", "uart_last_t": 0,
            "pwm_last": "", "pwm_time": 0,
            "dht_pulses": [], "dht_last": "", "dht_last_t": 0,
            "cs_active": False, "spi_last_t": 0,
            "timing_buf": {}
        }

        # CSV Writer
        self.f_csv = None
        self.writer = None

    def check_device(self, retries=5, delay=3):
        """BƯỚC 1: Kiểm tra sự hiện diện của phần cứng (với cơ chế Retry và Wait)."""
        print("=" * 80)
        
        # Log version de debug neu can
        try:
            v = subprocess.run(["sigrok-cli", "--version"], capture_output=True, text=True)
            print(f" [DEBUG] {v.stdout.splitlines()[0] if v.stdout else 'Sigrok version unknown'}")
        except: pass

        for i in range(retries):
            print(f" [HE THONG] Dang quet Logic Analyzer (Lan {i+1}/{retries})...")
            try:
                # Lan dau tien sau boot, lenh nay se trigger upload firmware fx2lafw
                result = subprocess.run(
                    ["sigrok-cli", "-d", "fx2lafw", "--scan"],
                    capture_output=True, text=True, timeout=10
                )
                output = (result.stdout + result.stderr).lower()
                
                if "fx2lafw" in output:
                    print(" OK: Da tim thay thiet bi fx2lafw.")
                    
                    # Neu day la lan dau tim thay (co the vua upload firmware), 
                    # can cho 2-3s de device re-enumerate tren USB bus.
                    print(" [INFO] Dang doi thiet bi on dinh (USB re-enumeration)...")
                    time.sleep(3)
                    
                    # Kiem tra lai lan nua de chac chan device da san sang
                    check_again = subprocess.run(
                        ["sigrok-cli", "-d", "fx2lafw", "--scan"],
                        capture_output=True, text=True, timeout=10
                    )
                    if "fx2lafw" in (check_again.stdout + check_again.stderr).lower():
                        print(" OK: Thiet bi da san sang cho Capture!")
                        return True
                    else:
                        print(" [!] Thiet bi chua on dinh, dang thu lai...")
            except Exception as e:
                print(f" [DEBUG] Loi trong qua trinh quet: {e}")

            if i < retries - 1:
                print(f" [!] Chua thay thiet bi, thu lai sau {delay}s...")
                time.sleep(delay)

        print("\n [!] LOI: Khong the ket noi voi Logic Analyzer.")
        print(" [GOI Y]:")
        print("  1. Kiem tra cap USB noi voi Pi.")
        print("  2. Kiem tra quyen truy cap (udev rules).")
        print("  3. Thu rut ra cam lai Logic Analyzer.")
        return False

    def load_config(self):
        """BƯỚC 2: Load decoder tu file Lab JSON hoac dung mac dinh."""
        if not self.lab_name:
            self.decoders = Config.DEFAULT_DECODERS
            return

        # Tim file {lab_name}.json truc tiep trong ASSIGNMENTS_DIR
        json_path = os.path.join(Config.ASSIGNMENTS_DIR, f"{self.lab_name}.json")

        if not os.path.exists(json_path):
            print(f" [WARN] Khong tim thay {json_path}, dung default config.")
            self.decoders = Config.DEFAULT_DECODERS
            return

        try:
            with open(json_path, encoding="utf-8") as f:
                cfg = json.load(f)
                self.decoders = cfg.get("decoders", Config.DEFAULT_DECODERS)
                # Chuan hoa UART format sang hex de de regex
                self.decoders = [d.replace("format=ascii", "format=hex") if "uart" in d else d for d in self.decoders]
        except Exception as e:
            print(f" [ERR] Khong the load JSON: {e}")
            self.decoders = Config.DEFAULT_DECODERS

        # Tu dong them timing decoders neu can thiet
        self._inject_timing_decoders()
        print(f" [INFO] Da load {len(self.decoders)} decoders.")

    def _inject_timing_decoders(self):
        """Helper: Them cac kenh do thoi gian (timing) cho cac giao thuc tuong ung."""
        decoder_str = ",".join(self.decoders)
        mappings = [("i2c", "i2c_scl"), ("spi", "spi_clk"), ("dht11", "dht11")]
        for protocol, channel_key in mappings:
            if protocol in decoder_str:
                d = f"timing:data={Config.CHANNEL_MAP[channel_key]}"
                if d not in self.decoders: self.decoders.append(d)

    def start(self, capture_time=Config.DEFAULT_TIME):
        """BƯỚC 3: Khoi chay sigrok-cli va luong xu ly stdout."""
        # Chuan bi CMD
        annots = self._build_annotations()
        channels = self._get_used_channels()

        cmd = [
            "sigrok-cli", "-d", "fx2lafw",
            "--config", f"samplerate={Config.SAMPLERATE}",
            "--time", f"{capture_time}s",
            "--channels", channels,
            *[arg for d in self.decoders for arg in ("-P", d)],
            "-A", ",".join(annots),
        ]

        print(f"\n [EXEC] {' '.join(cmd)}\n")

        # Chuan bi file CSV
        os.makedirs(os.path.dirname(Config.CSV_PATH), exist_ok=True)
        self.f_csv = open(Config.CSV_PATH, "w", newline="", encoding="utf-8")
        self.writer = csv.writer(self.f_csv)
        self.writer.writerow(["Time(ms)", "I2C_CLK", "I2C", "UART", "PWM", "DHT11", "SPI_CLK", "SPI", "CS"])

        self._print_table_header()

        # Run process
        self.proc = subprocess.Popen(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True, bufsize=1)

        # Kiem tra nhanh xem process co bi crash ngay lap tuc khong (vd: thiet bi bien mat)
        time.sleep(0.5)
        if self.proc.poll() is not None:
            _, stderr = self.proc.communicate()
            print(f" [!] LOI: sigrok-cli thoat som voi ma loi {self.proc.returncode}")
            print(f" [!] Chi tiet: {stderr.strip()}")
            return False

        # Start monitor threads
        threading.Thread(target=self._drain_stderr, daemon=True).start()
        self.thread = threading.Thread(target=self._process_stdout, daemon=False)
        self.thread.start()
        return True

    def wait(self):
        """Doi capture hoan tat."""
        if self.proc: self.proc.wait()
        if self.thread: self.thread.join()
        if self.f_csv: self.f_csv.close()

    # --- INTERNAL METHODS ---

    def _get_used_channels(self):
        used = set()
        for d in self.decoders:
            for part in d.split(":"):
                val = part.split("=")[-1]
                if re.match(r'^D\d+$', val): used.add(val)
        return ",".join(sorted(used, key=lambda x: int(x[1:])))

    def _build_annotations(self):
        decoder_str = ",".join(self.decoders)
        annots = []
        mapping = {
            "i2c": "i2c=address-write:address-read:data-write:data-read:ack:nack",
            "uart": "uart=tx-data",
            "pwm": "pwm=duty-cycle:period",
            "spi": "spi=mosi-data",
            "timing": "timing=time"
        }
        for k, v in mapping.items():
            if k in decoder_str: annots.append(v)
        return annots

    def _drain_stderr(self):
        for line in iter(self.proc.stderr.readline, ""):
            line = line.strip()
            if not line: continue
            # Khong loc libusb nua de de debug tren Pi
            if "warning" in line.lower() or "note" in line.lower():
                continue
            print(f" [SIGROK-ERR] {line}", flush=True)

    def _process_stdout(self):
        """BƯỚC 4: Logic xu ly tung dong du lieu tu sigrok-cli."""
        # Lay danh sach timing channels de map index
        timing_list = []
        for d in self.decoders:
            m = re.search(r'timing:data=(D\d+)', d)
            if m: timing_list.append(m.group(1))

        try:
            for raw in iter(self.proc.stdout.readline, ""):
                raw = raw.strip()
                if not raw or any(x in raw.lower() for x in ["warning", "libusb", "note"]): continue

                now = time.perf_counter_ns()
                if self.base_time is None: self.base_time = now
                ts = f"{(now - self.base_time) / 1e6:.3f}"

                self._handle_raw_line(raw, ts, now, timing_list)
                self._handle_timeouts(now, ts)

        except Exception as e:
            print(f" [ERR] Output processing: {e}")
        finally:
            self._final_flush()

    def _handle_raw_line(self, raw, ts, now_ns, timing_list):
        low = raw.lower()

        # 1. I2C
        if "i2c" in low:
            # 1. Neu la dia chi hoac du lieu
            m = re.search(r'(?:address|data)[- ](?:write|read):\s*([0-9a-fA-F]+)', raw, re.I)
            if m:
                type_str = "Addr" if "address" in low else "Data"
                dir_str = "W" if "write" in low else "R"
                val = f"{dir_str}-{type_str}: {m.group(1).upper()}"
                
                # Ghi tam vao state de doi ACK/NACK neu co
                self.state["last_i2c_val"] = val
                self.state["last_i2c_ts"] = ts
                return

            # 2. Neu la ACK/NACK (thuong xuat hien ngay sau line address/data)
            if "nack" in low or "ack" in low:
                val = self.state.get("last_i2c_val", "Unknown")
                ts = self.state.get("last_i2c_ts", ts)
                
                if "nack" in low:
                    val += " (NACK)"
                
                self._write_row(ts, i2c_clk=self.state["freq_i2c"], i2c=val)
                self.state["last_i2c_val"] = None
                return

        # 2. UART
        elif "uart" in low:
            m = re.search(r'uart-\d+:\s*([0-9a-fA-F]{2})\s*$', raw, re.I)
            if m:
                ch = chr(int(m.group(1), 16))
                self.state["uart_last_t"] = now_ns
                if ch in ('\n', '\r'):
                    if self.state["uart_buf"]:
                        self._write_row(ts, uart=self.state["uart_buf"].strip())
                        self.state["uart_buf"] = ""
                elif ch.isprintable(): self.state["uart_buf"] += ch

        # 3. SPI
        elif "spi" in low:
            m = re.search(r'spi-\d+:\s*([0-9a-fA-F]{2})\s*$', raw, re.I)
            if m:
                self.state["spi_last_t"] = now_ns
                if not self.state["cs_active"]:
                    self._write_row(ts, cs="LOW")
                    self.state["cs_active"] = True
                self._write_row(ts, spi_clk=self.state["freq_spi"], spi="0x" + m.group(1).upper())

        # 4. PWM
        elif "pwm" in low:
            f_m = re.search(r'([0-9.]+)\s*([kK]?)Hz', raw, re.I)
            d_m = re.search(r'([0-9.]+)\s*%', raw)
            if f_m and d_m:
                f_val = float(f_m.group(1)) * (1000 if f_m.group(2).lower() == 'k' else 1)
                pwm_str = f"{f_val/1000:.1f}kHz {round(float(d_m.group(1)), 1)}%"
                if pwm_str != self.state["pwm_last"] or (now_ns - self.state["pwm_time"]) > 1e9:
                    self.state["pwm_last"], self.state["pwm_time"] = pwm_str, now_ns
                    self._write_row(ts, pwm=pwm_str)

        # 5. TIMING (I2C/SPI Freq & DHT11)
        elif "timing" in low:
            ch_id_m = re.search(r'timing-(\d+)', raw)
            if not ch_id_m: return
            ch_idx = int(ch_id_m.group(1)) - 1
            ch_name = timing_list[ch_idx] if ch_idx < len(timing_list) else None

            f_m = re.search(r'\(([0-9.]+)\s*([kKmM]?)Hz\)', raw)
            if f_m:
                f_val = float(f_m.group(1)) * {"m": 1e6, "k": 1e3, "": 1.0}.get(f_m.group(2).lower(), 1.0)
                if ch_name == Config.CHANNEL_MAP['i2c_scl'] and f_val > 50000:
                    self.state["freq_i2c"] = f"{f_val/1000:.1f}k"
                elif ch_name == Config.CHANNEL_MAP['spi_clk'] and f_val > 50000:
                    self.state["freq_spi"] = f"{f_val/1000:.1f}k"

            elif ch_name == Config.CHANNEL_MAP['dht11']:
                t_m = re.search(r'([0-9.]+)\s*([μµum]s|ms|ns|s)\b', raw, re.I)
                if t_m:
                    us = float(t_m.group(1)) * {"s": 1e6, "ms": 1e3, "us": 1.0, "ns": 1e-3}.get(t_m.group(2).lower().replace('μ','u').replace('µ','u'), 1.0)
                    buf = self.state["timing_buf"].setdefault(ch_idx, [])
                    if 12000 < us < 30000: # DHT11 Start bit detected
                        if len(buf) >= 40:
                            res = ProtocolDecoders.decode_dht11(buf)
                            if res and (res != self.state["dht_last"] or (now_ns - self.state["dht_last_t"]) > 1e9):
                                self.state["dht_last"], self.state["dht_last_t"] = res, now_ns
                                self._write_row(ts, dht11=res)
                        self.state["timing_buf"][ch_idx] = []
                    elif us < 500000: buf.append(us)

    def _handle_timeouts(self, now_ns, ts):
        # SPI CS High timeout
        if self.state["cs_active"] and (now_ns - self.state["spi_last_t"]) > 50e6:
            self._write_row(ts, cs="HIGH")
            self.state["cs_active"] = False
        # UART buffer flush timeout
        if self.state["uart_buf"] and (now_ns - self.state["uart_last_t"]) > 500e6:
            self._write_row(ts, uart=self.state["uart_buf"].strip())
            self.state["uart_buf"] = ""

    def _write_row(self, ts, **kwargs):
        """BƯỚC 5: Ghi dữ liệu đồng thời ra Console và CSV."""
        cols = {"i2c_clk": "", "i2c": "", "uart": "", "pwm": "", "dht11": "", "spi_clk": "", "spi": "", "cs": ""}
        cols.update(kwargs)
        # Console output
        print(f"[{ts:>9}] | {cols['i2c_clk']:<9} | {cols['i2c']:<14} | {cols['uart']:<24} | {cols['pwm']:<14} | {cols['dht11']:<19} | {cols['spi_clk']:<9} | {cols['spi']:<12} | {cols['cs']:<8}")
        # CSV output
        if self.writer:
            self.writer.writerow([ts, cols["i2c_clk"], cols["i2c"], cols["uart"], cols["pwm"], cols["dht11"], cols["spi_clk"], cols["spi"], cols["cs"]])

    def _print_table_header(self):
        h = f"{'THOI GIAN':<11} | {'I2C CLK':<9} | {'I2C':<14} | {'UART':<24} | {'PWM':<14} | {'DHT11':<19} | {'SPI CLK':<9} | {'SPI':<12} | {'CS':<8}"
        print("=" * len(h))
        print(h)
        print("-" * len(h))

    def _final_flush(self):
        if self.state["uart_buf"]:
            self._write_row("END", uart=self.state["uart_buf"].strip())
        print(f"\n [XONG] Da luu ket qua capture tai: {Config.CSV_PATH}")

# ==============================================================================
# SECTION 4: ENTRY POINT
# ==============================================================================

def main():
    parser = argparse.ArgumentParser(description="Autograder Logic Analyzer Tool v2.0")
    parser.add_argument("--lab", type=str, help="Ten lab (vd: lab7_dht11)")
    parser.add_argument("--time", type=int, default=Config.DEFAULT_TIME, help="Thoi gian capture (s)")
    parser.add_argument("--no-check", action="store_true", help="Bo qua kiem tra thiet bi")
    args = parser.parse_args()

    # Khoi tao Runner
    runner = SigrokRunner(lab_name=args.lab)

    # BUOC 1: Check hardware
    if not args.no_check:
        if not runner.check_device():
            sys.exit(1)

    # BUOC 2: Load Lab config
    runner.load_config()

    # BUOC 3, 4 & 5: Capture, Process & Save
    runner.start(capture_time=args.time)
    runner.wait()

if __name__ == "__main__":
    main()
