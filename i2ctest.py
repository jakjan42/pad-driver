#!/usr/bin/env python3

import smbus, time

STATUS_OK = 0
STATUS_REQ_JOYS_SIZE = 1
STATUS_REQ_JOYS_COUNT = 2
STATUS_REQ_BTN_COUNT = 3
STATUS_REQ_JOYS_START = 4
STATUS_REQ_BTN_START = 5
STATUS_REQ_CALIBRATE = 6
STATUS_CALIBRATING = 7


I2C_REGS = [
  ('REG_STATUS',    1, 1), 
  ('REG_REQ_DATA',  2, 2), 
  ('REG_JOYS1X',    4, 2), 
  ('REG_JOYS1Y',    6, 2), 
  ('REG_BUTTONS1',  8, 1)
]

DEVICE_BUS = 3
DEVICE_ADDR = 0x67

if __name__ == '__main__':
    bus = smbus.SMBus(DEVICE_BUS)

    while True:
        for regname, regnum, regsize in I2C_REGS:
            if regsize == 1:
                regval = bus.read_byte_data(DEVICE_ADDR, regnum)
            else:
                regval = bus.read_word_data(DEVICE_ADDR, regnum)
            print(f'{regname:<16}: {regval:04X}')
        print(f'\033[{len(I2C_REGS) + 1}A')
        time.sleep(0.1)
