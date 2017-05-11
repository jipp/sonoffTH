#include "SHT3X.h"

SHT3X::SHT3X() {
}

void SHT3X::begin(uint8_t i2cAddr) {
  this->i2cAddr = i2cAddr;
  Wire.begin();
}

void SHT3X::measure() {
  unsigned int data[6];

  writeCommand(SHT31_MEAS_HIGHREP);
  delay(500);
  Wire.requestFrom(this-i2CAddr, 6);
  if (Wire.available() == 6) {
    for (uint8_t i=0; i<6; i++) {
      data[i] = Wire.read();
    }
  }
  float temperature = ((((data[0] * 256.0) + data[1]) * 175) / 65535.0) - 45;
  float humidity = ((((data[3] * 256.0) + data[4]) * 100) / 65535.0);

}

float SHT3X::getTemperature() {
  return this->temperature;
}

float SHT3X::getHumidity() {
  return humidity;
}

void SHT3X::writeCommand(uint16_t command) {
  Wire.beginTransmission(this->i2caddr);
  Wire.write(command >> 8);
  Wire.write(command & 0xFF);
  Wire.endTransmission();
}

void SHT3X::reset() {
  writeCommand(SOFTRESET);
  delay(10);
}
