sudo apt update
sudo apt install sigrok-cli -y
sudo apt install sigrok-firmware-fx2lafw -y

# D0 = I2C SCL    D4 = SPI MOSI
# D1 = I2C SDA    D5 = SPI CLK
# D2 = UART TX    D6 = SPI CS
# D3 = DHT11      D7 = PWM

sigrok-cli --scan
# pi@lab:~ $ sigrok-cli --scan
# The following devices were found:
# demo - Demo device with 13 channels: D0 D1 D2 D3 D4 D5 D6 D7 A0 A1 A2 A3 A4
# fx2lafw - Saleae Logic with 8 channels: D0 D1 D2 D3 D4 D5 D6 D7

# Capture raw I2C, SDA=D1, SCL=D0, 2MHz, 5 giây
sigrok-cli -d fx2lafw \
  --config samplerate=1M \
  --time 10s \
  --channels D0,D1 \
  -P i2c:scl=D0:sda=D1 \
  -A i2c=address-write:address-read:data-write:data-read

# Capture UART TX=D2, baudrate 115200
sigrok-cli -d fx2lafw \
  --config samplerate=2M \
  --time 30s \
  --channels D0,D1,D2,D3,D4,D5,D6,D7 \
  -P uart:baudrate=115200:tx=D2:format=hex \
  -A uart=tx-data

# test tool sigrok.py
python3 /home/pi/ve_lab/logic/sigrok.py --lab lab5_i2c --time 10

# show all annotation classes of i2c
sigrok-cli --protocol-decoders i2c --show
ID: i2c
Name: I²C
Long name: Inter-Integrated Circuit
Description: Two-wire, multi-master, serial bus.
License: gplv2+
Possible decoder input IDs:
- logic
Possible decoder output IDs:
- i2c
Decoder tags:
Annotation classes:
- start: Start condition
- repeat-start: Repeat start condition
- stop: Stop condition
- ack: ACK
- nack: NACK
- bit: Data/address bit
- address-read: Address read
- address-write: Address write
- data-read: Data read
- data-write: Data write
- warnings: Human-readable warnings
Annotation rows:
- bits (Bits): bit
- addr-data (Address/Data): start, repeat-start, stop, ack, nack, address-read, address-write, data-read, data-write
- warnings (Warnings): warnings
Binary classes:
- address-read: Address read
- address-write: Address write
- data-read: Data read
- data-write: Data write
Required channels:
- scl (SCL): Serial clock line
- sda (SDA): Serial data line
Optional channels:
None.
Options:
- address_format: Displayed slave address format ('shifted', 'unshifted', default 'shifted')
Documentation:
I²C (Inter-Integrated Circuit) is a bidirectional, multi-master
bus using two signals (SCL = serial clock line, SDA = serial data line).
