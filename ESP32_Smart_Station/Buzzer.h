#pragma once
#include <Arduino.h>

// Active buzzer behind an NPN transistor: HIGH on the pin sounds it. Plays
// short beep patterns without blocking the main loop.
class Buzzer {
public:
  void begin(int pin);
  void play(uint8_t beeps, uint16_t onMs, uint16_t offMs, uint32_t nowMs);   // replaces any pattern in progress
  void stop();
  void update(uint32_t nowMs);
  bool sounding() const { return on_; }

private:
  void drive(bool on);

  int      pin_ = -1;
  bool     on_ = false;
  uint8_t  beepsLeft_ = 0;
  uint16_t onMs_ = 0;
  uint16_t offMs_ = 0;
  uint32_t phaseStartMs_ = 0;
};
