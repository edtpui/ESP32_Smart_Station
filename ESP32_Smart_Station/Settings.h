#pragma once
#include <Arduino.h>
#include <Preferences.h>
#include "Config.h"

// How the fan decides when to run.
enum class FanMode : uint8_t {
  Auto   = 0,  // runs while motion is detected, then for "Hold time" after it stops
  Toggle = 1,  // every new motion event flips the fan on/off ("wave to toggle")
  On     = 2,  // always on
  Off    = 3   // always off
};
constexpr uint8_t FAN_MODE_COUNT = 4;

const char* fanModeName(FanMode mode);   // "Auto", "Toggle", "On", "Off"
const char* fanModeTag(FanMode mode);    // "AUTO", "TOGGLE", "ON", "OFF"

// Choices offered by the menus. The settings store an index into these lists.
constexpr uint16_t HOLD_TIME_OPTIONS_S[]     = {5, 10, 15, 30, 45, 60, 90, 120, 180, 300, 600};
constexpr uint8_t  HOLD_TIME_OPTION_COUNT    = sizeof(HOLD_TIME_OPTIONS_S) / sizeof(HOLD_TIME_OPTIONS_S[0]);
constexpr uint16_t SCREEN_SLEEP_OPTIONS_S[]  = {0, 15, 30, 60, 120, 300};   // 0 = never
constexpr uint8_t  SCREEN_SLEEP_OPTION_COUNT = sizeof(SCREEN_SLEEP_OPTIONS_S) / sizeof(SCREEN_SLEEP_OPTIONS_S[0]);
constexpr uint8_t  BRIGHTNESS_CONTRAST[]     = {0x08, 0x5F, 0xCF};            // Low / Medium / High
constexpr uint8_t  BRIGHTNESS_LEVEL_COUNT    = sizeof(BRIGHTNESS_CONTRAST);

constexpr uint8_t FAN_SPEED_MIN_PCT  = 10;
constexpr uint8_t FAN_SPEED_MAX_PCT  = 100;
constexpr uint8_t FAN_SPEED_STEP_PCT = 10;
constexpr int16_t GATE_TEMP_MIN_C10  = 100;   // 10.0 C
constexpr int16_t GATE_TEMP_MAX_C10  = 400;   // 40.0 C
constexpr int16_t GATE_TEMP_STEP_C10 = 5;     // 0.5 C
constexpr int16_t ALERT_TEMP_MIN_C10  = 150;  // 15.0 C
constexpr int16_t ALERT_TEMP_MAX_C10  = 450;  // 45.0 C
constexpr int16_t ALERT_TEMP_STEP_C10 = 5;    // 0.5 C
constexpr uint8_t ALERT_HUM_MIN_PCT   = 30;
constexpr uint8_t ALERT_HUM_MAX_PCT   = 95;
constexpr uint8_t ALERT_HUM_STEP_PCT  = 5;

// User settings. Kept in RAM and saved to the ESP32's flash (NVS) so they
// survive power cycles and re-uploads.
class Settings {
public:
  FanMode fanMode     = FanMode::Auto;
  uint8_t fanSpeedPct = DEFAULT_FAN_SPEED_PCT;
  uint8_t holdIndex   = DEFAULT_HOLD_INDEX;
  bool    tempGateOn  = false;                  // Auto mode only runs the fan when it's warm enough
  int16_t gateTempC10 = DEFAULT_GATE_TEMP_C10;  // that threshold, in tenths of a degree C
  bool    fahrenheit  = false;
  uint8_t sleepIndex  = DEFAULT_SLEEP_INDEX;
  uint8_t brightness  = DEFAULT_BRIGHTNESS;
  bool    flipDisplay = false;
  bool    autoDim      = true;                    // drop the OLED to Low brightness in a dark room
  bool    soundOn      = true;                    // master switch for the buzzer
  bool    motionChime  = false;                   // short beep on every new motion event
  bool    climateAlert = true;                    // beep when it gets too hot or too humid
  int16_t alertTempC10 = DEFAULT_ALERT_TEMP_C10;  // tenths of a degree C
  uint8_t alertHumPct  = DEFAULT_ALERT_HUM_PCT;

  void begin();                      // load from flash (saves defaults on first boot)
  void restoreDefaults();
  void markChanged(uint32_t nowMs);  // schedule a save; repeated changes are batched
  void update(uint32_t nowMs);       // performs the scheduled save
  void saveNow();

  uint32_t holdTimeMs() const;
  uint32_t screenSleepMs() const;    // 0 = never sleep
  float    gateTempC() const { return gateTempC10 / 10.0f; }
  float    alertTempC() const { return alertTempC10 / 10.0f; }

private:
  void sanitize();

  Preferences prefs_;
  bool        storageOk_ = false;
  bool        dirty_ = false;
  uint32_t    changedAtMs_ = 0;
};
