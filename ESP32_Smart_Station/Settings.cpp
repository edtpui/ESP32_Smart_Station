#include "Settings.h"

namespace {
const char* const NVS_NAMESPACE   = "smartstation";
const uint8_t     SETTINGS_SCHEMA = 1;   // bump when the stored layout changes
}  // namespace

const char* fanModeName(FanMode mode) {
  switch (mode) {
    case FanMode::Auto:   return "Auto";
    case FanMode::Toggle: return "Toggle";
    case FanMode::On:     return "On";
    case FanMode::Off:    return "Off";
  }
  return "?";
}

const char* fanModeTag(FanMode mode) {
  switch (mode) {
    case FanMode::Auto:   return "AUTO";
    case FanMode::Toggle: return "TOGGLE";
    case FanMode::On:     return "ON";
    case FanMode::Off:    return "OFF";
  }
  return "?";
}

void Settings::begin() {
  restoreDefaults();
  storageOk_ = prefs_.begin(NVS_NAMESPACE, false);
  if (!storageOk_) {
    Serial.println(F("[SETTINGS] flash storage unavailable, using defaults"));
    return;
  }
  if (prefs_.getUChar("schema", 0) != SETTINGS_SCHEMA) {
    Serial.println(F("[SETTINGS] first boot, saving defaults"));
    saveNow();
    return;
  }

  fanMode     = static_cast<FanMode>(prefs_.getUChar("fanMode", static_cast<uint8_t>(fanMode)));
  fanSpeedPct = prefs_.getUChar("fanSpeed", fanSpeedPct);
  holdIndex   = prefs_.getUChar("holdIdx", holdIndex);
  tempGateOn  = prefs_.getBool("gateOn", tempGateOn);
  gateTempC10 = prefs_.getShort("gateC10", gateTempC10);
  fahrenheit  = prefs_.getBool("fahrenheit", fahrenheit);
  sleepIndex  = prefs_.getUChar("sleepIdx", sleepIndex);
  brightness  = prefs_.getUChar("bright", brightness);
  flipDisplay = prefs_.getBool("flip", flipDisplay);
  autoDim      = prefs_.getBool("autoDim", autoDim);
  soundOn      = prefs_.getBool("sound", soundOn);
  motionChime  = prefs_.getBool("chime", motionChime);
  climateAlert = prefs_.getBool("climAlert", climateAlert);
  alertTempC10 = prefs_.getShort("alertC10", alertTempC10);
  alertHumPct  = prefs_.getUChar("alertHum", alertHumPct);
  sanitize();
  Serial.println(F("[SETTINGS] loaded"));
}

void Settings::restoreDefaults() {
  fanMode     = FanMode::Auto;
  fanSpeedPct = DEFAULT_FAN_SPEED_PCT;
  holdIndex   = DEFAULT_HOLD_INDEX;
  tempGateOn  = false;
  gateTempC10 = DEFAULT_GATE_TEMP_C10;
  fahrenheit  = false;
  sleepIndex  = DEFAULT_SLEEP_INDEX;
  brightness  = DEFAULT_BRIGHTNESS;
  flipDisplay = false;
  autoDim      = true;
  soundOn      = true;
  motionChime  = false;
  climateAlert = true;
  alertTempC10 = DEFAULT_ALERT_TEMP_C10;
  alertHumPct  = DEFAULT_ALERT_HUM_PCT;
}

void Settings::markChanged(uint32_t nowMs) {
  dirty_ = true;
  changedAtMs_ = nowMs;
}

void Settings::update(uint32_t nowMs) {
  if (dirty_ && nowMs - changedAtMs_ >= SETTINGS_SAVE_DELAY_MS) saveNow();
}

void Settings::saveNow() {
  sanitize();
  dirty_ = false;
  if (!storageOk_) return;

  prefs_.putUChar("schema", SETTINGS_SCHEMA);
  prefs_.putUChar("fanMode", static_cast<uint8_t>(fanMode));
  prefs_.putUChar("fanSpeed", fanSpeedPct);
  prefs_.putUChar("holdIdx", holdIndex);
  prefs_.putBool("gateOn", tempGateOn);
  prefs_.putShort("gateC10", gateTempC10);
  prefs_.putBool("fahrenheit", fahrenheit);
  prefs_.putUChar("sleepIdx", sleepIndex);
  prefs_.putUChar("bright", brightness);
  prefs_.putBool("flip", flipDisplay);
  prefs_.putBool("autoDim", autoDim);
  prefs_.putBool("sound", soundOn);
  prefs_.putBool("chime", motionChime);
  prefs_.putBool("climAlert", climateAlert);
  prefs_.putShort("alertC10", alertTempC10);
  prefs_.putUChar("alertHum", alertHumPct);
  Serial.println(F("[SETTINGS] saved"));
}

uint32_t Settings::holdTimeMs() const {
  return HOLD_TIME_OPTIONS_S[holdIndex] * 1000UL;
}

uint32_t Settings::screenSleepMs() const {
  return SCREEN_SLEEP_OPTIONS_S[sleepIndex] * 1000UL;
}

// Keeps values loaded from flash (or typed in by a future feature) in range.
void Settings::sanitize() {
  if (static_cast<uint8_t>(fanMode) >= FAN_MODE_COUNT) fanMode = FanMode::Auto;
  fanSpeedPct = constrain(fanSpeedPct, FAN_SPEED_MIN_PCT, FAN_SPEED_MAX_PCT);
  if (holdIndex >= HOLD_TIME_OPTION_COUNT) holdIndex = DEFAULT_HOLD_INDEX;
  gateTempC10 = constrain(gateTempC10, GATE_TEMP_MIN_C10, GATE_TEMP_MAX_C10);
  if (sleepIndex >= SCREEN_SLEEP_OPTION_COUNT) sleepIndex = DEFAULT_SLEEP_INDEX;
  if (brightness >= BRIGHTNESS_LEVEL_COUNT) brightness = DEFAULT_BRIGHTNESS;
  alertTempC10 = constrain(alertTempC10, ALERT_TEMP_MIN_C10, ALERT_TEMP_MAX_C10);
  alertHumPct  = constrain(alertHumPct, ALERT_HUM_MIN_PCT, ALERT_HUM_MAX_PCT);
}
