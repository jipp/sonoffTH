#ifndef DHTXX_H
#define DHTXX_H

#include "Arduino.h"
#include <DHT.h>

class DHTXX {
public:
  DHTXX();
  void begin();
  void measure();
  float getTemperature();
  float getHumidity();

private:
  float temperature;
  float humidity;
};

#endif
