#include "Buzzer.h"

void Buzzer::begin(int pin) {
  pin_ = pin;
  pinMode(pin_, OUTPUT);
  drive(false);
}

void Buzzer::drive(bool on) {
  on_ = on;
  if (pin_ >= 0) digitalWrite(pin_, on ? HIGH : LOW);
}

void Buzzer::play(uint8_t beeps, uint16_t onMs, uint16_t offMs, uint32_t nowMs) {
  if (pin_ < 0 || beeps == 0 || onMs == 0) return;
  beepsLeft_ = beeps;
  onMs_ = onMs;
  offMs_ = offMs;
  phaseStartMs_ = nowMs;
  drive(true);
}

void Buzzer::stop() {
  beepsLeft_ = 0;
  drive(false);
}

void Buzzer::update(uint32_t nowMs) {
  if (beepsLeft_ == 0) return;
  const uint32_t elapsed = nowMs - phaseStartMs_;
  if (on_) {
    if (elapsed < onMs_) return;
    drive(false);
    beepsLeft_--;
    phaseStartMs_ = nowMs;
  } else if (elapsed >= offMs_) {
    drive(true);
    phaseStartMs_ = nowMs;
  }
}
