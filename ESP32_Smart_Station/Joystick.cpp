#include "Joystick.h"
#include "Config.h"

const char* joyEventName(JoyEvent event) {
  switch (event) {
    case JoyEvent::None:      return "none";
    case JoyEvent::Up:        return "up";
    case JoyEvent::Down:      return "down";
    case JoyEvent::Left:      return "left";
    case JoyEvent::Right:     return "right";
    case JoyEvent::Press:     return "press";
    case JoyEvent::LongPress: return "long press";
  }
  return "?";
}

void Joystick::begin(int pinX, int pinY, int pinButton) {
  pinX_ = pinX;
  pinY_ = pinY;
  pinButton_ = pinButton;
  pinMode(pinButton_, INPUT_PULLUP);   // SW shorts to GND when pressed
  analogReadResolution(12);            // 0-4095
  calibrate();
}

bool Joystick::calibrate() {
  const int samples = 32;
  long sumX = 0, sumY = 0;
  int minX = 4095, maxX = 0, minY = 4095, maxY = 0;
  for (int i = 0; i < samples; i++) {
    const int x = analogRead(pinX_);
    const int y = analogRead(pinY_);
    sumX += x;
    sumY += y;
    minX = min(minX, x);
    maxX = max(maxX, x);
    minY = min(minY, y);
    maxY = max(maxY, y);
    delay(2);
  }
  centerX_ = sumX / samples;
  centerY_ = sumY / samples;

  // A resting stick sits near mid-scale and reads steadily. Readings stuck at
  // one end, or jumping around, mean it is unplugged (floating pins) or held.
  const bool steady  = (maxX - minX) < 250 && (maxY - minY) < 250;
  const bool centred = centerX_ > 800 && centerX_ < 3300 && centerY_ > 800 && centerY_ < 3300;
  stickOk_ = steady && centred;
  dir_ = Dir::None;
  return stickOk_;
}

void Joystick::update(uint32_t nowMs) {
  if (pinButton_ < 0) return;

  // ----- Button: debounce, then Press on release or LongPress while held -----
  const bool pressed = digitalRead(pinButton_) == LOW;
  if (pressed != buttonRaw_) {
    buttonRaw_ = pressed;
    buttonChangedMs_ = nowMs;
  }
  if (buttonRaw_ != buttonStable_ && nowMs - buttonChangedMs_ >= BUTTON_DEBOUNCE_MS) {
    buttonStable_ = buttonRaw_;
    if (buttonStable_) {
      buttonDownMs_ = nowMs;
      longFired_ = false;
    } else if (!longFired_) {
      push(JoyEvent::Press);
    }
  }
  if (buttonStable_ && !longFired_ && nowMs - buttonDownMs_ >= BUTTON_LONG_PRESS_MS) {
    longFired_ = true;
    push(JoyEvent::LongPress);
  }

  // ----- Stick: sample every JOY_SAMPLE_MS -----
  if (nowMs - lastSampleMs_ < JOY_SAMPLE_MS) return;
  lastSampleMs_ = nowMs;
  rawX_ = readAxis(pinX_);
  rawY_ = readAxis(pinY_);
  if (!stickOk_) return;

  int dx = rawX_ - centerX_;
  int dy = rawY_ - centerY_;
  if (JOY_SWAP_XY) {
    const int t = dx;
    dx = dy;
    dy = t;
  }
  if (JOY_INVERT_X) dx = -dx;
  if (JOY_INVERT_Y) dy = -dy;

  Dir next = dir_;
  if (dir_ == Dir::None) {
    if (abs(dx) >= JOY_THRESHOLD_ON || abs(dy) >= JOY_THRESHOLD_ON) {
      if (abs(dx) > abs(dy)) next = dx > 0 ? Dir::Right : Dir::Left;
      else                   next = dy > 0 ? Dir::Down : Dir::Up;
    }
  } else {
    // Release (with hysteresis) once the active axis is back near the centre.
    const bool horizontal = (dir_ == Dir::Left || dir_ == Dir::Right);
    if (abs(horizontal ? dx : dy) < JOY_THRESHOLD_OFF) next = Dir::None;
  }

  if (next != dir_) {
    dir_ = next;
    if (dir_ != Dir::None) {
      push(toEvent(dir_));
      nextRepeatMs_ = nowMs + JOY_REPEAT_DELAY_MS;
    }
  } else if (dir_ != Dir::None && static_cast<int32_t>(nowMs - nextRepeatMs_) >= 0) {
    push(toEvent(dir_));   // auto-repeat while held
    nextRepeatMs_ = nowMs + JOY_REPEAT_RATE_MS;
  }
}

JoyEvent Joystick::nextEvent() {
  if (queueCount_ == 0) return JoyEvent::None;
  const JoyEvent event = queue_[queueHead_];
  queueHead_ = (queueHead_ + 1) % JOY_QUEUE_SIZE;
  queueCount_--;
  return event;
}

JoyEvent Joystick::toEvent(Dir dir) {
  switch (dir) {
    case Dir::Up:    return JoyEvent::Up;
    case Dir::Down:  return JoyEvent::Down;
    case Dir::Left:  return JoyEvent::Left;
    case Dir::Right: return JoyEvent::Right;
    case Dir::None:  break;
  }
  return JoyEvent::None;
}

// Averages a few readings to smooth out ADC noise.
int Joystick::readAxis(int pin) const {
  int sum = 0;
  for (int i = 0; i < 4; i++) sum += analogRead(pin);
  return sum / 4;
}

void Joystick::push(JoyEvent event) {
  if (queueCount_ >= JOY_QUEUE_SIZE) return;   // UI isn't keeping up; drop it
  queue_[(queueHead_ + queueCount_) % JOY_QUEUE_SIZE] = event;
  queueCount_++;
}
