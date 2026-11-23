#pragma once
#include <Arduino.h>

// Photoresistor in a voltage divider (photoresistor to 3V3, 10k to GND), read
// on an ADC1 pin. Smooths the reading and reports it as 0-100 %.
class LightSensor {
public:
  void begin(int pin);
  void update(uint32_t nowMs);

  uint8_t percent() const { return percent_; }   // 0 = dark, 100 = direct light
  int     raw() const     { return raw_; }       // last ADC reading, 0-4095
  bool    dark() const    { return dark_; }      // below LIGHT_DARK_ON_PCT (with hysteresis)

private:
  int      pin_ = -1;
  int      raw_ = 0;
  float    filtered_ = 0.0f;
  uint8_t  percent_ = 0;
  bool     dark_ = false;
  uint32_t lastSampleMs_ = 0;
};
