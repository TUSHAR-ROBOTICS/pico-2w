#include <Wire.h>
#include <U8g2lib.h>
#include <math.h>

#define ICM_ADDR 0x68

U8G2_SH1106_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0);

int16_t ax, ay, az;
int16_t gx, gy, gz;

void writeReg(uint8_t reg, uint8_t value)
{
  Wire.beginTransmission(ICM_ADDR);
  Wire.write(reg);
  Wire.write(value);
  Wire.endTransmission();
}

uint8_t readReg(uint8_t reg)
{
  Wire.beginTransmission(ICM_ADDR);
  Wire.write(reg);
  Wire.endTransmission(false);

  Wire.requestFrom(ICM_ADDR, 1);

  if (Wire.available())
    return Wire.read();

  return 0;
}

void readIMU()
{
  Wire.beginTransmission(ICM_ADDR);
  Wire.write(0x2D);
  Wire.endTransmission(false);

  Wire.requestFrom(ICM_ADDR, 12);

  if (Wire.available() >= 12)
  {
    ax = (Wire.read() << 8) | Wire.read();
    ay = (Wire.read() << 8) | Wire.read();
    az = (Wire.read() << 8) | Wire.read();

    gx = (Wire.read() << 8) | Wire.read();
    gy = (Wire.read() << 8) | Wire.read();
    gz = (Wire.read() << 8) | Wire.read();
  }
}

void setup()
{
  Serial.begin(115200);
  delay(2000);

  Wire.setSDA(0);
  Wire.setSCL(1);
  Wire.begin();

  u8g2.begin();

  uint8_t whoami = readReg(0x00);

  if (whoami != 0xEA)
  {
    Serial.println("ICM20948 NOT FOUND");

    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_6x10_tf);
    u8g2.drawStr(0,20,"ICM20948 NOT FOUND");
    u8g2.sendBuffer();

    while(1);
  }

  writeReg(0x06, 0x01);

  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_6x10_tf);
  u8g2.drawStr(0,20,"ICM20948 READY");
  u8g2.sendBuffer();

  delay(1000);
}

void loop()
{
  readIMU();

  float roll =
      atan2((float)ay, (float)az) * 57.2958;

  float pitch =
      atan2(-(float)ax,
      sqrt((float)ay * ay + (float)az * az))
      * 57.2958;

  Serial.print("AX:");
  Serial.print(ax);
  Serial.print(" AY:");
  Serial.print(ay);
  Serial.print(" AZ:");
  Serial.print(az);

  Serial.print(" R:");
  Serial.print(roll,1);

  Serial.print(" P:");
  Serial.println(pitch,1);

  u8g2.clearBuffer();

  u8g2.setFont(u8g2_font_6x10_tf);

  u8g2.setCursor(0,10);
  u8g2.print("AX:");
  u8g2.print(ax);

  u8g2.setCursor(0,22);
  u8g2.print("AY:");
  u8g2.print(ay);

  u8g2.setCursor(0,34);
  u8g2.print("AZ:");
  u8g2.print(az);

  u8g2.setCursor(0,46);
  u8g2.print("Roll:");
  u8g2.print(roll,1);

  u8g2.setCursor(0,58);
  u8g2.print("Pitch:");
  u8g2.print(pitch,1);

  u8g2.sendBuffer();

  delay(100);
}