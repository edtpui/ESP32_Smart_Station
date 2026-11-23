#include "MotionSensor.h"
#include "Config.h"

void MotionSensor::begin(int pin, uint32_t warmupMs) {
  pin_ = pin;
  warmupMs_ = warmupMs;
  pinMode(pin_, INPUT_PULLDOWN);   // reads "no motion" if the sensor is unplugged
  startMs_ = millis();
  raw_ = digitalRead(pin_) == HIGH;
  rawChangedMs_ = startMs_;
}

void MotionSensor::update(uint32_t nowMs) {
  if (pin_ < 0) return;

  const bool level = digitalRead(pin_) == HIGH;
  if (level != raw_) {
    raw_ = level;
    rawChangedMs_ = nowMs;
  }

  if (!ready_) {
    if (nowMs - startMs_ < warmupMs_) return;   // the PIR is still settling
    ready_ = true;
  }

  if (raw_ != active_ && nowMs - rawChangedMs_ >= PIR_DEBOUNCE_MS) {
    active_ = raw_;
    if (active_) {
      events_++;
      newEvent_ = true;
      everDetected_ = true;
    }
  }
  if (active_) lastMotionMs_ = nowMs;
}

uint32_t MotionSensor::warmupRemainingMs(uint32_t nowMs) const {
  if (ready_) return 0;
  const uint32_t elapsed = nowMs - startMs_;
  return elapsed >= warmupMs_ ? 0 : warmupMs_ - elapsed;
}

bool MotionSensor::takeNewEvent() {
  const bool event = newEvent_;
  newEvent_ = false;
  return event;
}
