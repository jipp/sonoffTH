#ifndef SHT3X_H
#define SHT3X_H

#include "Arduino.h"
#include <Wire.h>

#define DEFAULT_ADDR  0x45
#define SOFTRESET     0x30A2
#define MEAS_HIGHREP  0x2400

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
