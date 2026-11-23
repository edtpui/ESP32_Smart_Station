#pragma once
#include <Arduino.h>

enum class JoyEvent : uint8_t { None, Up, Down, Left, Right, Press, LongPress };
const char* joyEventName(JoyEvent event);

constexpr uint8_t JOY_QUEUE_SIZE = 8;

// 2-axis analog joystick with push button (KY-023 style module).
// Turns the stick into Up / Down / Left / Right events with auto-repeat, and
// the button into Press / LongPress events.
class Joystick {
public:
  void     begin(int pinX, int pinY, int pinButton);
  bool     calibrate();               // learn the resting centre; false if the stick looks unplugged
  void     update(uint32_t nowMs);    // call every loop
  JoyEvent nextEvent();               // JoyEvent::None when nothing is waiting

  bool     stickOk() const        { return stickOk_; }
  int      rawX() const           { return rawX_; }
  int      rawY() const           { return rawY_; }
  bool     buttonHeld() const     { return buttonStable_; }

private:
  enum class Dir : uint8_t { None, Up, Down, Left, Right };
  static JoyEvent toEvent(Dir dir);
  int  readAxis(int pin) const;
  void push(JoyEvent event);

  int      pinX_ = -1;
  int      pinY_ = -1;
  int      pinButton_ = -1;
  bool     stickOk_ = false;
  int      rawX_ = 0;
  int      rawY_ = 0;
  int      centerX_ = 2048;
  int      centerY_ = 2048;
  Dir      dir_ = Dir::None;
  uint32_t lastSampleMs_ = 0;
  uint32_t nextRepeatMs_ = 0;

  bool     buttonRaw_ = false;
  bool     buttonStable_ = false;
  bool     longFired_ = false;
  uint32_t buttonChangedMs_ = 0;
  uint32_t buttonDownMs_ = 0;

  JoyEvent queue_[JOY_QUEUE_SIZE];
  uint8_t  queueHead_ = 0;
  uint8_t  queueCount_ = 0;
};
