#pragma once
#include <Arduino.h>
#include <DHT.h>
#include "Config.h"

// DHT11 / DHT22 temperature + humidity sensor with min/max tracking and a
// short history for the trend graph.
class ClimateSensor {
public:
  ClimateSensor(int pin, uint8_t type);
  void begin();
  void update(uint32_t nowMs);

  bool  valid() const        { return valid_; }      // a good reading within CLIMATE_STALE_MS
  float temperatureC() const { return tempC_; }
  float humidity() const     { return humidity_; }

  bool  hasStats() const     { return statsValid_; }
  float minTempC() const     { return minTempC_; }
  float maxTempC() const     { return maxTempC_; }
  float minHumidity() const  { return minHumidity_; }
  float maxHumidity() const  { return maxHumidity_; }
  void  resetStats()         { statsValid_ = false; }

  uint32_t goodReads() const   { return goodReads_; }
  uint32_t failedReads() const { return failedReads_; }

  // Trend history: index 0 is the oldest sample, missing samples are NAN.
  uint8_t historyCount() const { return historyCount_; }
  float   historyTempC(uint8_t i) const;
  float   historyHumidity(uint8_t i) const;

private:
  uint8_t historySlot(uint8_t i) const;
  void    pushHistory(float tempC, float humidity);

  DHT      dht_;
  bool     valid_ = false;
  bool     haveReading_ = false;
  float    tempC_ = NAN;
  float    humidity_ = NAN;
  bool     statsValid_ = false;
  float    minTempC_ = 0.0f;
  float    maxTempC_ = 0.0f;
  float    minHumidity_ = 0.0f;
  float    maxHumidity_ = 0.0f;
  uint32_t lastReadMs_ = 0;
  uint32_t lastGoodMs_ = 0;
  uint32_t lastHistoryMs_ = 0;
  uint32_t goodReads_ = 0;
  uint32_t failedReads_ = 0;

  float   histTemp_[TREND_HISTORY_LEN];
  float   histHumidity_[TREND_HISTORY_LEN];
  uint8_t historyHead_ = 0;    // next slot to write
  uint8_t historyCount_ = 0;
};
