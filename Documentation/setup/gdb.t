openocd -f interface/stlink.cfg -f target/stm32f1x.cfg

arm-none-eabi-gdb /home/pi/stm32_template/build/template_make.elf

target extended-remote :3333

monitor reset halt
info registers
continue

# đọc GPIOC ODR — xem PC13 đang HIGH hay LOW
(gdb) x/wx 0x4001100C
# output| 0x4001100c:	0x00000000

# đọc GPIOA IDR — xem trạng thái input PA0
(gdb) x/wx 0x40010808



