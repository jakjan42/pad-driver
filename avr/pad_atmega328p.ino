#include <Wire.h>
#ifndef u8
typedef uint8_t  u8;
#endif // !u8

#define PAD_DEBUG 0
#define PAD_DEBUG_LEDPORT 0

#define I2C_ADDR 0x67
#define I2C_SDA A4
#define I2C_SCL A5

#define PIN_JOYS1X  A1
#define PIN_JOYS1Y  A2
#define PIN_BUTTON1 5

enum status {
  STATUS_OK,
  STATUS_RESERVED,
  STATUS_REQ_JOYS_COUNT,
  STATUS_REQ_BTN_COUNT,
  STATUS_REQ_JOYS_START,
  STATUS_REQ_BTN_START,
  STATUS_REQ_CALIBRATE,
  STATUS_CALIBRATING,
};

#define JOYS_SIZE 2

/*
 triplets (register_name, register_number, register_size),
 sorted by register_number in ascending order */
#define I2C_REGS_COUNT 5
#define I2C_REGS \
  X(REG_STATUS,    1, 1) \
  X(REG_REQ_DATA,  2, JOYS_SIZE) \
  X(REG_JOYS1X,    4, JOYS_SIZE) \
  X(REG_JOYS1Y,    6, JOYS_SIZE) \
  X(REG_BUTTONS1,  8, 1)


#if PAD_DEBUG == 1

const char *i2c_reg_names[] = {
  #define X(reg, num, size) #reg ,
    I2C_REGS
  #undef X
};

u8 i2c_reg_nums_by_index[] = {
  #define X(reg, num, size) num ,
    I2C_REGS
  #undef X
};

#endif // PAD_DEBUG == 1

enum i2c_regs {
  #define X(reg, num, size) reg = num ,
    I2C_REGS
  #undef X
  REG_MAX,
};

#define BTN_START  REG_BUTTONS1
#define JOYS_START REG_JOYS1X
#define BTN_COUNT  1
#define JOYS_COUNT 1

u8 i2c_reg_ptr = REG_STATUS;

u8 reg_status_prev = STATUS_OK;
u8 reg_status = STATUS_OK;
u16 reg_req_data = 0;

#if PAD_DEBUG_LEDPORT == 1
void ledport_init(void)
{
  DDRB |= 0x1f;
  DDRC |= 0x7;
}

void ledport_display(uint8_t data)
{
  PORTB = (PORTB & 0xe0) | (data & 0x1f);
  PORTC = (PORTC & 0xf8) | (data >> 5);
}
#else // PAD_DEBUG_LEDPORT == 1
void ledport_init(void) {}
void ledport_display(uint8_t data) {}
#endif // PAD_DEBUG_LEDPORT == 1


void status_handle_val(u8 val)
{
  switch (val) {
  case STATUS_OK:
      val = STATUS_OK;
      break;
  case STATUS_REQ_JOYS_SIZE:
      reg_req_data = JOYS_SIZE;
      break;
  case STATUS_REQ_JOYS_COUNT:
      reg_req_data = JOYS_COUNT;
      break;
  case STATUS_REQ_BTN_COUNT:
      reg_req_data = BTN_COUNT;
      break;
  case STATUS_REQ_JOYS_START:
      reg_req_data = JOYS_START;
      break;
  case STATUS_REQ_BTN_START:
      reg_req_data = BTN_START;
      break;
  case STATUS_REQ_CALIBRATE:
  case STATUS_CALIBRATING:
    break;
  }
}

void reg_set_val(u8 reg_no, u16 val)
{
  switch (reg_no) {
  case REG_STATUS:
    reg_status = (u8)val;
    status_handle_val(val);
    break;
  default:
    break;
  }
}

u16 reg_get_val(u8 reg_no)
{
  switch (reg_no) {
    case REG_STATUS:
      return (u16)reg_status;
    case REG_JOYS1X:
      return analogRead(PIN_JOYS1X);
    case REG_JOYS1Y:
      return analogRead(PIN_JOYS1Y);
    case REG_BUTTONS1:
      return digitalRead(PIN_BUTTON1);
    case REG_REQ_DATA:
      return reg_req_data;
    default:
      return 0xffff;
  }
}

void i2c_on_recieve(int how_many)
{
  if (how_many == 1 && Wire.available()) {
    i2c_reg_ptr = Wire.read();
    return;
  }

  u16 val;
  if (Wire.available())
    i2c_reg_ptr = Wire.read();
  if (Wire.available())
    val = Wire.read();
  if (Wire.available())
    val = (val << 8) | (u8)Wire.read();
  reg_set_val(i2c_reg_ptr, val);

  while (Wire.available())
    Wire.read();
}

void i2c_on_request(void)
{
  if (i2c_reg_ptr == REG_REQ_DATA |
      i2c_reg_ptr == REG_JOYS1X   |
      i2c_reg_ptr == REG_JOYS1Y) {
    u16 val = reg_get_val(i2c_reg_ptr);
    Wire.write((u8)(val >> 8));
    Wire.write((u8)(val & 0xff));
  } else {
    Wire.write((u8)(reg_get_val(i2c_reg_ptr) & 0xff));
  }
}

#if PAD_DEBUG == 1
void serial_print_regs(void)
{
  for (u8 i = 0; i < I2C_REGS_COUNT; i++) {
    Serial.print(i2c_reg_names[i]);
    Serial.print(": ");
    Serial.println(reg_get_val(i2c_reg_nums_by_index[i]));
  }

  Serial.println();
}
#else // PAD_DEBUG == 1
void serial_print_regs(void) {}
#endif // PAD_DEBUG == 1

void setup(void)
{
  if (PAD_DEBUG)
    Serial.begin(115200);
  if (PAD_DEBUG_LEDPORT)
    ledport_init();

  Wire.begin(I2C_ADDR);

  // disable internal pullups 
  pinMode(I2C_SDA, INPUT);
  pinMode(I2C_SCL, INPUT);

  Wire.onReceive(i2c_on_recieve);
  Wire.onRequest(i2c_on_request);

  pinMode(PIN_BUTTON1, INPUT_PULLUP);
}



void loop(void)
{
  if (PAD_DEBUG) {
    serial_print_regs();
    delay(100);
  }

}
