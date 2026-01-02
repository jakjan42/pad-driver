#include <Wire.h>

typedef uint8_t u8;

#define PAD_DEBUG 0
#define PAD_DEBUG_LEDPORT 0

#define I2C_ADDR 0x67
#define I2C_SDA A4
#define I2C_SCL A5

#define PIN_JOYS1X  A1
#define PIN_JOYS1Y  A2
#define PIN_BUTTON1 5


// pairs (register_name, register_number),
// must be sorted by register_number in ascending order
 
#define I2C_REGS_COUNT 6
#define I2C_REGS \
  X(REG_STATUS,    1) \
  X(REG_JOYS1XMSB, 2) \
  X(REG_JOYS1XLSB, 3) \
  X(REG_JOYS1YMSB, 4) \
  X(REG_JOYS1YLSB, 5) \
  X(REG_BUTTONS1, 10)


#if PAD_DEBUG == 1

const char *i2c_reg_names[] = {
  #define X(reg, num) #reg ,
    I2C_REGS
  #undef X
};

u8 i2c_reg_nums_by_index[] = {
  #define X(reg, num) num ,
    I2C_REGS
  #undef X
};

#endif // PAD_DEBUG == 1

enum pad_regs {
  #define X(reg, num) reg = num ,
    I2C_REGS
  #undef X
  REG_MAX,
};

u8 i2c_reg_ptr = REG_JOYS1XMSB;

u8 reg_status = 0x67;


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


void reg_set_val(u8 reg_no, u8 val)
{
  switch (reg_no) {
  case REG_STATUS:
    reg_status = val;
    break;
  default:
    break;
  }
}

u8 reg_get_val(u8 reg_no)
{
  int a;
  u8 val = 0xff;
  switch (reg_no) {
    case REG_STATUS:
      return reg_status;
    case REG_JOYS1XMSB:
      a = analogRead(PIN_JOYS1X);
      return *((u8 *)(&a) + 1);
    case REG_JOYS1XLSB:
      a = analogRead(PIN_JOYS1X);
      return *((u8 *)(&a) + 0);
    case REG_JOYS1YMSB:
      a = analogRead(PIN_JOYS1Y);
      return *((u8 *)(&a) + 1);
    case REG_JOYS1YLSB:
      a = analogRead(PIN_JOYS1Y);
      return *((u8 *)(&a) + 0);
    case REG_BUTTONS1:
      val = digitalRead(PIN_BUTTON1);
      return val;
    default:
      return val;
  }
}

void i2c_on_recieve(int how_many)
{
  if (how_many == 1 && Wire.available()) {
    i2c_reg_ptr = Wire.read();
    return;
  }

  if (Wire.available())
    i2c_reg_ptr = Wire.read();
  if (Wire.available()) {
    reg_set_val(i2c_reg_ptr, Wire.read());
    i2c_reg_ptr++;
  }

  while (Wire.available())
    Wire.read();
}

void i2c_on_request(void)
{
  Wire.write(reg_get_val(i2c_reg_ptr));
  i2c_reg_ptr++;
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
