#pragma once
#include <Arduino.h>

// PIR motion sensor (HC-SR501 or AM312). Ignores the output during the
// power-up warm-up, debounces it and counts motion events.
class MotionSensor {
public:
  void begin(int pin, uint32_t warmupMs);
  void update(uint32_t nowMs);

  bool     ready() const        { return ready_; }        // warm-up finished
  uint32_t warmupRemainingMs(uint32_t nowMs) const;
  bool     active() const       { return active_; }       // motion right now
  bool     rawLevel() const     { return raw_; }          // pin level, for diagnostics
  bool     everDetected() const { return everDetected_; }
  uint32_t lastMotionMs() const { return lastMotionMs_; } // last moment the output was HIGH
  uint32_t eventCount() const   { return events_; }
  void     resetCount()         { events_ = 0; }
  bool     takeNewEvent();      // true once for each new motion event

private:
  int      pin_ = -1;
  uint32_t startMs_ = 0;
  uint32_t warmupMs_ = 0;
  bool     ready_ = false;
  bool     raw_ = false;
  bool     active_ = false;
  bool     everDetected_ = false;
  bool     newEvent_ = false;
  uint32_t rawChangedMs_ = 0;
  uint32_t lastMotionMs_ = 0;
  uint32_t events_ = 0;
};
