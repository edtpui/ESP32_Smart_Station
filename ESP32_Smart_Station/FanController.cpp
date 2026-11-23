#include "FanController.h"
#include "Config.h"

// ESP32 Arduino core 3.x changed the LEDC (PWM) API; support both 2.x and 3.x.
#if defined(ESP_ARDUINO_VERSION_MAJOR) && ESP_ARDUINO_VERSION_MAJOR >= 3
#define FAN_LEDC_PIN_API 1
#else
#define FAN_LEDC_PIN_API 0
#endif

bool FanController::begin(int pin) {
  pin_ = pin;
#if FAN_LEDC_PIN_API
  if (!ledcAttach(pin_, FAN_PWM_FREQ_HZ, FAN_PWM_BITS)) {
    Serial.println(F("[FAN] PWM setup failed"));
    pin_ = -1;
    return false;
  }
#else
  ledcSetup(FAN_PWM_CHANNEL, FAN_PWM_FREQ_HZ, FAN_PWM_BITS);
  ledcAttachPin(pin_, FAN_PWM_CHANNEL);
#endif
  writeDuty(0);
  lastUpdateMs_ = millis();
  return true;
}

void FanController::setRunning(bool on) {
  running_ = on;
}

void FanController::setSpeedPercent(uint8_t pct) {
  speedPct_ = constrain(pct, 10, 100);
}

void FanController::update(uint32_t nowMs) {
  const uint32_t dt = nowMs - lastUpdateMs_;
  lastUpdateMs_ = nowMs;
  if (pin_ < 0) return;

  if (!running_) {
    rampDuty_ = 0.0f;
  } else {
    runtimeMs_ += dt;
    const float target = targetDuty();
    const float step = 255.0f * dt / FAN_SOFTSTART_MS;
    if (rampDuty_ < FAN_MIN_DUTY) {
      rampDuty_ = FAN_MIN_DUTY;   // jump to a duty that gets the motor turning, then ramp
    } else if (rampDuty_ < target) {
      rampDuty_ = (rampDuty_ + step > target) ? target : rampDuty_ + step;
    } else if (rampDuty_ > target) {
      rampDuty_ = (rampDuty_ - step < target) ? target : rampDuty_ - step;
    }
  }

  const uint8_t newDuty = static_cast<uint8_t>(rampDuty_ + 0.5f);
  if (newDuty != duty_) writeDuty(newDuty);
}

// 10 % = slowest reliable speed (FAN_MIN_DUTY), 100 % = full power (255).
uint8_t FanController::targetDuty() const {
  return FAN_MIN_DUTY + static_cast<uint32_t>(255 - FAN_MIN_DUTY) * (speedPct_ - 10) / 90;
}

void FanController::writeDuty(uint8_t duty) {
  duty_ = duty;
#if FAN_LEDC_PIN_API
  ledcWrite(pin_, duty);
#else
  ledcWrite(FAN_PWM_CHANNEL, duty);
#endif
}
