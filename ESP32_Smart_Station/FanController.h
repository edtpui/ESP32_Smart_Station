#pragma once
#include <Arduino.h>

// Drives the fan motor with PWM through a low-side MOSFET (or the EN / IN pin
// of an L293D or L9110 driver). A soft-start ramp keeps the motor's inrush
// current from browning out a USB-powered ESP32.
class FanController {
public:
  bool begin(int pin);
  void setRunning(bool on);
  void setSpeedPercent(uint8_t pct);   // 10-100 %, used while running
  void update(uint32_t nowMs);         // call every loop

  bool     running() const      { return running_; }
  uint8_t  duty() const         { return duty_; }       // what is on the pin right now (0-255)
  uint32_t runtimeMs() const    { return runtimeMs_; }  // total on-time since boot / reset
  void     resetRuntime()       { runtimeMs_ = 0; }

private:
  uint8_t targetDuty() const;
  void    writeDuty(uint8_t duty);

  int      pin_ = -1;
  bool     running_ = false;
  uint8_t  speedPct_ = 100;
  uint8_t  duty_ = 0;
  float    rampDuty_ = 0.0f;   // fractional duty so the ramp is smooth
  uint32_t lastUpdateMs_ = 0;
  uint32_t runtimeMs_ = 0;
};
