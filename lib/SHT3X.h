#ifndef SHT3X_H
#define SHT3X_H

#include "Arduino.h"
#include <Wire.h>

#define DEFAULT_ADDR                0x44
#define MEASUREMENT_HIGH            0x2C06
#define MEASUREMENT_MEDIUM          0x2C0D
#define MEASUREMENT_LOW             0x2C10
#define MEASUREMENT_STRETCH_HIGH    0x2400
#define MEASUREMENT_STRETCH_MEDIUM  0x240B
#define MEASUREMENT_STRETCH_LOW     0x2416
#define SOFTRESET                   0x30A2

class SHT3X {
public:
  SHT3X();
  void begin(uint8_t i2cAddr = DEFAULT_ADDR);
  void measure();
  float getTemperature();
  float getHumidity();

private:
  uint8_t i2caddr;
  float temperature;
  float humidity;

  void writeCommand(uint16_t command);
  void reset();
};

#endif
