#include "LightSensor.h"
#include "Config.h"

namespace {
constexpr float ADC_FULL_SCALE = 4095.0f;
constexpr float SMOOTHING      = 0.2f;   // share of each new sample in the average

uint8_t toPercent(float adc) {
  return static_cast<uint8_t>(constrain(lroundf(adc * 100.0f / ADC_FULL_SCALE), 0L, 100L));
}
}  // namespace

void LightSensor::begin(int pin) {
  pin_ = pin;
  pinMode(pin_, INPUT);
  raw_ = analogRead(pin_);
  filtered_ = raw_;
  percent_ = toPercent(filtered_);
  dark_ = percent_ < LIGHT_DARK_ON_PCT;
  lastSampleMs_ = millis();
}

void LightSensor::update(uint32_t nowMs) {
  if (pin_ < 0 || nowMs - lastSampleMs_ < LIGHT_SAMPLE_MS) return;
  lastSampleMs_ = nowMs;

  raw_ = analogRead(pin_);
  filtered_ += (raw_ - filtered_) * SMOOTHING;
  percent_ = toPercent(filtered_);

  // Two thresholds, so a reading that sits on the edge doesn't flicker.
  if (dark_ && percent_ >= LIGHT_DARK_OFF_PCT)     dark_ = false;
  else if (!dark_ && percent_ < LIGHT_DARK_ON_PCT) dark_ = true;
}
