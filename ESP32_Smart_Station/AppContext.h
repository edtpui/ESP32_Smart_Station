#pragma once
#include <Arduino.h>
#include "Settings.h"
#include "FanController.h"
#include "MotionSensor.h"
#include "ClimateSensor.h"
#include "Joystick.h"
#include "StationWifi.h"
#include "LightSensor.h"
#include "Buzzer.h"
#include "IrRemote.h"

// Why the fan is (or isn't) running right now. Shown on the OLED and in the log.
enum class FanReason : uint8_t {
  WarmingUp,   // PIR still settling after power-up
  Motion,      // motion detected right now
  Hold,        // motion stopped, counting down the hold time
  Idle,        // no recent motion
  TooCool,     // motion, but below the temperature gate
  ToggledOn,   // Toggle mode: the last wave switched it on
  ToggledOff,  // Toggle mode: the last wave switched it off
  ForcedOn,    // mode = On
  ForcedOff    // mode = Off
};

inline const char* fanReasonName(FanReason reason) {
  switch (reason) {
    case FanReason::WarmingUp:  return "PIR warming up";
    case FanReason::Motion:     return "motion detected";
    case FanReason::Hold:       return "holding after motion";
    case FanReason::Idle:       return "no motion";
    case FanReason::TooCool:    return "below gate temperature";
    case FanReason::ToggledOn:  return "toggled on by motion";
    case FanReason::ToggledOff: return "toggled off by motion";
    case FanReason::ForcedOn:   return "manual on";
    case FanReason::ForcedOff:  return "manual off";
  }
  return "?";
}

struct FanStatus {
  FanReason reason;
  uint32_t  holdRemainingMs;
  uint32_t  warmupRemainingMs;
};

// Climate alerts that are active right now (see updateAlerts in the sketch).
struct AlertStatus {
  bool tempHigh;
  bool humidityHigh;
};

// Everything the UI needs to read or change, in one place.
struct AppContext {
  Settings*      settings;
  FanController* fan;
  MotionSensor*  motion;
  ClimateSensor* climate;
  Joystick*      joystick;
  FanStatus*     fanStatus;
  StationWifi* network;
  LightSensor*   light;
  Buzzer*        buzzer;
  IrRemote*      remote;
  AlertStatus*   alerts;
};

// Formats a duration as "02:03:04", or "1d 02:03:04" past a day.
inline void formatDuration(char* buf, size_t len, uint32_t ms) {
  const unsigned long totalS = ms / 1000UL;
  const unsigned long days = totalS / 86400UL;
  const unsigned long h = (totalS / 3600UL) % 24UL;
  const unsigned long m = (totalS / 60UL) % 60UL;
  const unsigned long s = totalS % 60UL;
  if (days > 0) snprintf(buf, len, "%lud %02lu:%02lu:%02lu", days, h, m, s);
  else          snprintf(buf, len, "%02lu:%02lu:%02lu", h, m, s);
}
