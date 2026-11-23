#include "ClimateSensor.h"

ClimateSensor::ClimateSensor(int pin, uint8_t type) : dht_(pin, type) {}

void ClimateSensor::begin() {
  dht_.begin();
  // The first read happens one interval after boot, which also gives the
  // sensor the 1-2 s it needs after power-up.
  lastReadMs_ = millis();
  for (uint8_t i = 0; i < TREND_HISTORY_LEN; i++) {
    histTemp_[i] = NAN;
    histHumidity_[i] = NAN;
  }
}

void ClimateSensor::update(uint32_t nowMs) {
  if (nowMs - lastReadMs_ >= CLIMATE_READ_INTERVAL_MS) {
    lastReadMs_ = nowMs;
    const float t = dht_.readTemperature();   // one bus transaction...
    const float h = dht_.readHumidity();      // ...the library reuses it here
    if (isnan(t) || isnan(h) || t < -40.0f || t > 85.0f || h < 0.0f || h > 100.0f) {
      failedReads_++;
    } else {
      tempC_ = t + TEMP_OFFSET_C;
      humidity_ = constrain(h + HUMIDITY_OFFSET_PCT, 0.0f, 100.0f);
      haveReading_ = true;
      lastGoodMs_ = nowMs;
      goodReads_++;

      if (!statsValid_) {
        minTempC_ = maxTempC_ = tempC_;
        minHumidity_ = maxHumidity_ = humidity_;
        statsValid_ = true;
      } else {
        if (tempC_ < minTempC_) minTempC_ = tempC_;
        if (tempC_ > maxTempC_) maxTempC_ = tempC_;
        if (humidity_ < minHumidity_) minHumidity_ = humidity_;
        if (humidity_ > maxHumidity_) maxHumidity_ = humidity_;
      }
    }
  }

  valid_ = haveReading_ && (nowMs - lastGoodMs_ < CLIMATE_STALE_MS);

  // Trend: first sample as soon as there is data, then one every interval.
  if (haveReading_ && (historyCount_ == 0 || nowMs - lastHistoryMs_ >= TREND_SAMPLE_INTERVAL_MS)) {
    lastHistoryMs_ = nowMs;
    pushHistory(valid_ ? tempC_ : NAN, valid_ ? humidity_ : NAN);
  }
}

float ClimateSensor::historyTempC(uint8_t i) const {
  return i < historyCount_ ? histTemp_[historySlot(i)] : NAN;
}

float ClimateSensor::historyHumidity(uint8_t i) const {
  return i < historyCount_ ? histHumidity_[historySlot(i)] : NAN;
}

// The oldest sample sits historyCount_ slots behind the write head.
uint8_t ClimateSensor::historySlot(uint8_t i) const {
  return (historyHead_ + TREND_HISTORY_LEN - historyCount_ + i) % TREND_HISTORY_LEN;
}

void ClimateSensor::pushHistory(float tempC, float humidity) {
  histTemp_[historyHead_] = tempC;
  histHumidity_[historyHead_] = humidity;
  historyHead_ = (historyHead_ + 1) % TREND_HISTORY_LEN;
  if (historyCount_ < TREND_HISTORY_LEN) historyCount_++;
}
