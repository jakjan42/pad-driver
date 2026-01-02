#!/usr/bin/env python3

import smbus, time


I2C_REGS = [
    ('REG_STATUS',    1),
    ('REG_JOYS1XMSB', 2),
    ('REG_JOYS1XLSB', 3),
    ('REG_JOYS1YMSB', 4),
    ('REG_JOYS1YLSB', 5),
    ('REG_BUTTONS1', 10)
]

DEVICE_BUS = 3
DEVICE_ADDR = 0x67

if __name__ == '__main__':
    bus = smbus.SMBus(DEVICE_BUS)
    while True:
        for regname, regnum in I2C_REGS:
            regval = bus.read_byte_data(DEVICE_ADDR, regnum)
            print(f'{regname:<16}: {regval}')
        print(f'\033[{len(I2C_REGS) + 1}A')
        time.sleep(0.1)
