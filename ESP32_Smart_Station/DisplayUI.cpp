#include "DisplayUI.h"
#include "Config.h"

namespace {
const char DEGREE_CHAR = static_cast<char>(248);   // degree sign in the CP437 font

enum FanRow : uint8_t { FAN_ROW_MODE, FAN_ROW_SPEED, FAN_ROW_HOLD, FAN_ROW_GATE, FAN_ROW_GATE_TEMP, FAN_ROW_COUNT };
const char* const FAN_ROW_LABELS[FAN_ROW_COUNT] = {"Mode", "Speed", "Hold time", "Temp gate", "Gate temp"};

enum SysRow : uint8_t {
  SYS_ROW_UNITS, SYS_ROW_SLEEP, SYS_ROW_BRIGHTNESS, SYS_ROW_AUTO_DIM, SYS_ROW_FLIP,
  SYS_ROW_SOUND, SYS_ROW_CHIME, SYS_ROW_ALERT, SYS_ROW_ALERT_TEMP, SYS_ROW_ALERT_HUM,
  SYS_ROW_RESET_STATS, SYS_ROW_RECAL, SYS_ROW_FACTORY, SYS_ROW_COUNT
};
const char* const SYS_ROW_LABELS[SYS_ROW_COUNT] = {
  "Units", "Screen sleep", "Brightness", "Auto dim", "Flip screen",
  "Sound", "Motion beep", "Climate alert", "Alert temp", "Alert humid",
  "Reset stats", "Centre stick", "Factory reset"
};

const char* const BRIGHTNESS_NAMES[BRIGHTNESS_LEVEL_COUNT] = {"Low", "Medium", "High"};

constexpr int16_t HEADER_H     = 10;   // title bar incl. separator line
constexpr int16_t ROW_H        = 10;
constexpr uint8_t VISIBLE_ROWS = 5;
constexpr float   TWO_PI_F     = 6.2831853f;

// The built-in font is 6 px per character at size 1.
int16_t textWidth(const char* s, uint8_t size) {
  return static_cast<int16_t>(strlen(s) * 6 * size);
}

// "45s", "2m", "1m30s"
void formatSeconds(char* buf, size_t len, uint16_t seconds) {
  const unsigned s = seconds;
  if (s < 60)           snprintf(buf, len, "%us", s);
  else if (s % 60 == 0) snprintf(buf, len, "%um", s / 60);
  else                  snprintf(buf, len, "%um%02us", s / 60, s % 60);
}

// Short (max 12 characters) explanation of the fan state for the Home footer.
void formatReason(char* buf, size_t len, const FanStatus& status) {
  switch (status.reason) {
    case FanReason::WarmingUp:
      snprintf(buf, len, "Warm-up %lus", (unsigned long)((status.warmupRemainingMs + 999) / 1000));
      break;
    case FanReason::Motion:
      snprintf(buf, len, "Motion!");
      break;
    case FanReason::Hold: {
      const unsigned long s = (status.holdRemainingMs + 999) / 1000;
      snprintf(buf, len, "Off in %lu:%02lu", s / 60, s % 60);
      break;
    }
    case FanReason::Idle:       snprintf(buf, len, "No motion"); break;
    case FanReason::TooCool:    snprintf(buf, len, "Too cool"); break;
    case FanReason::ToggledOn:  snprintf(buf, len, "Wave to stop"); break;
    case FanReason::ToggledOff: snprintf(buf, len, "Wave to run"); break;
    case FanReason::ForcedOn:   snprintf(buf, len, "Always on"); break;
    case FanReason::ForcedOff:  snprintf(buf, len, "Always off"); break;
  }
}

float toDisplayTemp(float tempC, bool fahrenheit) {
  return fahrenheit ? tempC * 9.0f / 5.0f + 32.0f : tempC;
}
}  // namespace

DisplayUI::DisplayUI() : oled_(OLED_WIDTH, OLED_HEIGHT, &Wire, -1, I2C_CLOCK_HZ, I2C_CLOCK_HZ) {}

bool DisplayUI::begin(AppContext* app) {
  app_ = app;
  Wire.begin(PIN_I2C_SDA, PIN_I2C_SCL);
  Wire.setClock(I2C_CLOCK_HZ);

  // Most 0.96" modules answer at 0x3C, some at 0x3D: try both.
  const uint8_t candidates[] = {OLED_I2C_ADDRESS, static_cast<uint8_t>(OLED_I2C_ADDRESS == 0x3C ? 0x3D : 0x3C)};
  uint8_t address = 0;
  for (uint8_t candidate : candidates) {
    Wire.beginTransmission(candidate);
    if (Wire.endTransmission() == 0) {
      address = candidate;
      break;
    }
  }
  if (address == 0) return false;
  if (!oled_.begin(SSD1306_SWITCHCAPVCC, address, true, false)) {
    Serial.println(F("[OLED] not enough memory for the frame buffer"));
    return false;
  }
  present_ = true;
  Serial.printf("[OLED] found at 0x%02X\n", address);

  oled_.cp437(true);
  oled_.setTextWrap(false);
  applyDisplaySettings();

  oled_.clearDisplay();
  drawTextCentered(OLED_WIDTH / 2, 6, "SMART", 2);
  drawTextCentered(OLED_WIDTH / 2, 26, "STATION", 2);
  drawTextCentered(OLED_WIDTH / 2, 52, "ESP32  v" FIRMWARE_VERSION);
  oled_.display();

  const uint32_t now = millis();
  lastWakeMs_ = now;
  splashUntilMs_ = now + SPLASH_MS;
  return true;
}

void DisplayUI::keepAwake(uint32_t nowMs) {
  lastWakeMs_ = nowMs;
  if (present_ && !panelOn_) setPanelOn(true, nowMs);
}

void DisplayUI::showToast(const char* message, uint32_t nowMs) {
  snprintf(toast_, sizeof(toast_), "%s", message);
  toastUntilMs_ = nowMs + TOAST_MS;
  lastFrameMs_ = nowMs - UI_FRAME_MS;   // redraw on the next update()
  Serial.printf("[UI] %s\n", toast_);
}

// ---------------------------------------------------------------------------
//  Input
// ---------------------------------------------------------------------------
void DisplayUI::handleEvent(JoyEvent event, uint32_t nowMs) {
  if (event == JoyEvent::None) return;
  const bool wasAsleep = present_ && !panelOn_;
  keepAwake(nowMs);
  if (wasAsleep) return;                 // the first touch only wakes the screen
  lastFrameMs_ = nowMs - UI_FRAME_MS;    // show the result right away

  Settings& s = *app_->settings;

  if (event == JoyEvent::LongPress) {
    if (page_ == UiPage::System && !editing_ && cursor_ == SYS_ROW_FACTORY) {
      activateRow(cursor_, true, nowMs);
    } else {
      goToPage(UiPage::Home);
    }
    return;
  }

  if (editing_) {
    if (event == JoyEvent::Up || event == JoyEvent::Right)     changeRow(cursor_, +1, nowMs);
    else if (event == JoyEvent::Down || event == JoyEvent::Left) changeRow(cursor_, -1, nowMs);
    else if (event == JoyEvent::Press)                           editing_ = false;
    return;
  }

  if (event == JoyEvent::Left || event == JoyEvent::Right) {
    const uint8_t step = (event == JoyEvent::Right) ? 1 : UI_PAGE_COUNT - 1;
    goToPage(static_cast<UiPage>((static_cast<uint8_t>(page_) + step) % UI_PAGE_COUNT));
    return;
  }

  // Remaining events: Up, Down, Press
  switch (page_) {
    case UiPage::Home: {
      char message[20];
      if (event == JoyEvent::Press) {
        s.fanMode = static_cast<FanMode>((static_cast<uint8_t>(s.fanMode) + 1) % FAN_MODE_COUNT);
        snprintf(message, sizeof(message), "Fan mode: %s", fanModeName(s.fanMode));
      } else {
        const int step = (event == JoyEvent::Up) ? FAN_SPEED_STEP_PCT : -FAN_SPEED_STEP_PCT;
        s.fanSpeedPct = constrain(s.fanSpeedPct + step, FAN_SPEED_MIN_PCT, FAN_SPEED_MAX_PCT);
        snprintf(message, sizeof(message), "Fan speed %u%%", s.fanSpeedPct);
      }
      s.markChanged(nowMs);
      showToast(message, nowMs);
      break;
    }

    case UiPage::Fan:
    case UiPage::System: {
      const uint8_t count = rowCount();
      if (event == JoyEvent::Up)        cursor_ = (cursor_ + count - 1) % count;
      else if (event == JoyEvent::Down) cursor_ = (cursor_ + 1) % count;
      else if (rowIsAction(cursor_))    activateRow(cursor_, false, nowMs);
      else                              editing_ = true;

      // Keep the cursor inside the visible window.
      if (cursor_ < scrollTop_) scrollTop_ = cursor_;
      if (cursor_ >= scrollTop_ + VISIBLE_ROWS) scrollTop_ = cursor_ - VISIBLE_ROWS + 1;
      break;
    }

    case UiPage::Trend:
      trendHumidity_ = !trendHumidity_;   // flip between temperature and humidity
      break;

    case UiPage::Network:
      if (event == JoyEvent::Press) {
        const bool turnOn = !app_->network->hotspotActive();
        if (app_->network->setHotspot(turnOn)) showToast(turnOn ? "Hotspot on" : "Hotspot off", nowMs);
        else                                   showToast("No Wi-Fi saved yet", nowMs);
      }
      break;

    case UiPage::Info:
      break;
  }
}

void DisplayUI::goToPage(UiPage page) {
  page_ = page;
  cursor_ = 0;
  scrollTop_ = 0;
  editing_ = false;
}

// ---------------------------------------------------------------------------
//  Menu model (Fan and System pages)
// ---------------------------------------------------------------------------
uint8_t DisplayUI::rowCount() const {
  if (page_ == UiPage::Fan) return FAN_ROW_COUNT;
  if (page_ == UiPage::System) return SYS_ROW_COUNT;
  return 0;
}

const char* DisplayUI::rowLabel(uint8_t row) const {
  if (page_ == UiPage::Fan && row < FAN_ROW_COUNT) return FAN_ROW_LABELS[row];
  if (page_ == UiPage::System && row < SYS_ROW_COUNT) return SYS_ROW_LABELS[row];
  return "";
}

void DisplayUI::rowValue(uint8_t row, char* buf, size_t len) const {
  const Settings& s = *app_->settings;
  buf[0] = '\0';
  if (page_ == UiPage::Fan) {
    switch (row) {
      case FAN_ROW_MODE:      snprintf(buf, len, "%s", fanModeName(s.fanMode)); break;
      case FAN_ROW_SPEED:     snprintf(buf, len, "%u%%", s.fanSpeedPct); break;
      case FAN_ROW_HOLD:      formatSeconds(buf, len, HOLD_TIME_OPTIONS_S[s.holdIndex]); break;
      case FAN_ROW_GATE:      snprintf(buf, len, "%s", s.tempGateOn ? "On" : "Off"); break;
      case FAN_ROW_GATE_TEMP: formatTemp(buf, len, s.gateTempC()); break;
      default: break;
    }
  } else if (page_ == UiPage::System) {
    switch (row) {
      case SYS_ROW_UNITS:
        snprintf(buf, len, "%c%c", DEGREE_CHAR, s.fahrenheit ? 'F' : 'C');
        break;
      case SYS_ROW_SLEEP:
        if (SCREEN_SLEEP_OPTIONS_S[s.sleepIndex] == 0) snprintf(buf, len, "Never");
        else formatSeconds(buf, len, SCREEN_SLEEP_OPTIONS_S[s.sleepIndex]);
        break;
      case SYS_ROW_BRIGHTNESS:  snprintf(buf, len, "%s", BRIGHTNESS_NAMES[s.brightness]); break;
      case SYS_ROW_AUTO_DIM:    snprintf(buf, len, "%s", s.autoDim ? "On" : "Off"); break;
      case SYS_ROW_FLIP:        snprintf(buf, len, "%s", s.flipDisplay ? "Yes" : "No"); break;
      case SYS_ROW_SOUND:       snprintf(buf, len, "%s", s.soundOn ? "On" : "Off"); break;
      case SYS_ROW_CHIME:       snprintf(buf, len, "%s", s.motionChime ? "On" : "Off"); break;
      case SYS_ROW_ALERT:       snprintf(buf, len, "%s", s.climateAlert ? "On" : "Off"); break;
      case SYS_ROW_ALERT_TEMP:  formatTemp(buf, len, s.alertTempC()); break;
      case SYS_ROW_ALERT_HUM:   snprintf(buf, len, "%u%%", s.alertHumPct); break;
      case SYS_ROW_RESET_STATS:
      case SYS_ROW_RECAL:       snprintf(buf, len, "Press"); break;
      case SYS_ROW_FACTORY:     snprintf(buf, len, "Hold"); break;
      default: break;
    }
  }
}

bool DisplayUI::rowIsAction(uint8_t row) const {
  return page_ == UiPage::System &&
         (row == SYS_ROW_RESET_STATS || row == SYS_ROW_RECAL || row == SYS_ROW_FACTORY);
}

void DisplayUI::changeRow(uint8_t row, int delta, uint32_t nowMs) {
  Settings& s = *app_->settings;
  if (page_ == UiPage::Fan) {
    switch (row) {
      case FAN_ROW_MODE:
        s.fanMode = static_cast<FanMode>((static_cast<int>(s.fanMode) + delta + FAN_MODE_COUNT) % FAN_MODE_COUNT);
        break;
      case FAN_ROW_SPEED:
        s.fanSpeedPct = constrain(s.fanSpeedPct + delta * FAN_SPEED_STEP_PCT, FAN_SPEED_MIN_PCT, FAN_SPEED_MAX_PCT);
        break;
      case FAN_ROW_HOLD:
        s.holdIndex = constrain(s.holdIndex + delta, 0, HOLD_TIME_OPTION_COUNT - 1);
        break;
      case FAN_ROW_GATE:
        s.tempGateOn = !s.tempGateOn;
        break;
      case FAN_ROW_GATE_TEMP:
        s.gateTempC10 = constrain(s.gateTempC10 + delta * GATE_TEMP_STEP_C10, GATE_TEMP_MIN_C10, GATE_TEMP_MAX_C10);
        break;
      default:
        return;
    }
  } else if (page_ == UiPage::System) {
    switch (row) {
      case SYS_ROW_UNITS:      s.fahrenheit = !s.fahrenheit; break;
      case SYS_ROW_SLEEP:      s.sleepIndex = constrain(s.sleepIndex + delta, 0, SCREEN_SLEEP_OPTION_COUNT - 1); break;
      case SYS_ROW_BRIGHTNESS: s.brightness = constrain(s.brightness + delta, 0, BRIGHTNESS_LEVEL_COUNT - 1); break;
      case SYS_ROW_AUTO_DIM:   s.autoDim = !s.autoDim; break;
      case SYS_ROW_FLIP:       s.flipDisplay = !s.flipDisplay; break;
      case SYS_ROW_SOUND:      s.soundOn = !s.soundOn; break;
      case SYS_ROW_CHIME:      s.motionChime = !s.motionChime; break;
      case SYS_ROW_ALERT:      s.climateAlert = !s.climateAlert; break;
      case SYS_ROW_ALERT_TEMP:
        s.alertTempC10 = constrain(s.alertTempC10 + delta * ALERT_TEMP_STEP_C10, ALERT_TEMP_MIN_C10, ALERT_TEMP_MAX_C10);
        break;
      case SYS_ROW_ALERT_HUM:
        s.alertHumPct = constrain(s.alertHumPct + delta * ALERT_HUM_STEP_PCT, ALERT_HUM_MIN_PCT, ALERT_HUM_MAX_PCT);
        break;
      default:                 return;
    }
  } else {
    return;
  }
  s.markChanged(nowMs);
}

void DisplayUI::activateRow(uint8_t row, bool longPress, uint32_t nowMs) {
  switch (row) {
    case SYS_ROW_RESET_STATS:
      app_->climate->resetStats();
      app_->motion->resetCount();
      app_->fan->resetRuntime();
      showToast("Stats cleared", nowMs);
      break;
    case SYS_ROW_RECAL:
      showToast("Let go of stick", nowMs);
      recalPending_ = true;
      recalAtMs_ = nowMs + 1000;
      break;
    case SYS_ROW_FACTORY:
      if (!longPress) {
        showToast("Hold to reset", nowMs);
        break;
      }
      app_->settings->restoreDefaults();
      app_->settings->saveNow();
      showToast("Defaults restored", nowMs);
      break;
    default:
      break;
  }
}

// ---------------------------------------------------------------------------
//  Output
// ---------------------------------------------------------------------------
void DisplayUI::update(uint32_t nowMs) {
  if (!present_) return;

  if (recalPending_ && static_cast<int32_t>(nowMs - recalAtMs_) >= 0) {
    recalPending_ = false;
    showToast(app_->joystick->calibrate() ? "Stick centred" : "Stick not found", nowMs);
  }

  applyDisplaySettings();

  // Screen saver: blank the OLED after a while with no motion or joystick use
  // (saves the OLED from burn-in). Motion or a joystick touch wakes it again.
  const uint32_t sleepMs = app_->settings->screenSleepMs();
  if (panelOn_ && sleepMs > 0 && !editing_ && nowMs - lastWakeMs_ > sleepMs) setPanelOn(false, nowMs);
  if (!panelOn_) return;

  if (static_cast<int32_t>(nowMs - splashUntilMs_) < 0) return;   // keep the splash up
  if (nowMs - lastFrameMs_ < UI_FRAME_MS) return;
  lastFrameMs_ = nowMs;
  render(nowMs);
}

void DisplayUI::render(uint32_t nowMs) {
  oled_.clearDisplay();
  switch (page_) {
    case UiPage::Home:   drawHome(nowMs); break;
    case UiPage::Fan:    drawMenu("Fan"); break;
    case UiPage::Trend:  drawTrend(); break;
    case UiPage::Network: drawNetwork(); break;
    case UiPage::System: drawMenu("System"); break;
    case UiPage::Info:   drawInfo(nowMs); break;
  }
  if (static_cast<int32_t>(nowMs - toastUntilMs_) < 0) drawToast();
  oled_.display();
}

void DisplayUI::drawHeader(const char* title) {
  drawText(0, 0, title);
  // Page dots on the right: the filled one is the current page.
  for (uint8_t i = 0; i < UI_PAGE_COUNT; i++) {
    const int16_t x = 94 + i * 6;
    if (i == static_cast<uint8_t>(page_)) oled_.fillCircle(x, 3, 2, SSD1306_WHITE);
    else                                  oled_.drawCircle(x, 3, 1, SSD1306_WHITE);
  }
  oled_.drawFastHLine(0, HEADER_H - 1, OLED_WIDTH, SSD1306_WHITE);
}

void DisplayUI::drawHome(uint32_t nowMs) {
  const Settings& s = *app_->settings;
  const ClimateSensor& climate = *app_->climate;
  const FanController& fan = *app_->fan;
  const MotionSensor& motion = *app_->motion;
  char buf[24];

  drawHeader(STATION_NAME);
  drawWifiStatus(81);

  // Temperature (large)
  formatTemp(buf, sizeof(buf), climate.valid() ? climate.temperatureC() : NAN);
  drawText(0, 14, buf, 2);

  // Humidity (large number, small unit)
  if (climate.valid()) snprintf(buf, sizeof(buf), "%.0f%%", climate.humidity());
  else                 snprintf(buf, sizeof(buf), "--%%");
  drawText(0, 34, buf, 2);
  drawText(textWidth(buf, 2) + 2, 41, "RH");

  // Fan icon: spins at a rate that follows the real PWM duty
  if (fan.duty() > 0) {
    fanAngle_ += 0.15f + 0.45f * fan.duty() / 255.0f;
    if (fanAngle_ > TWO_PI_F) fanAngle_ -= TWO_PI_F;
  }
  drawFanIcon(108, 25, 10, fanAngle_);
  if (fan.running()) snprintf(buf, sizeof(buf), "%u%%", s.fanSpeedPct);
  else               snprintf(buf, sizeof(buf), "OFF");
  drawTextCentered(108, 41, buf);

  // Footer: [MODE] reason ............ motion dot
  oled_.drawFastHLine(0, 52, OLED_WIDTH, SSD1306_WHITE);
  const char* tag = fanModeTag(s.fanMode);
  const int16_t tagW = textWidth(tag, 1) + 3;
  oled_.fillRect(0, 54, tagW, 10, SSD1306_WHITE);
  drawText(2, 55, tag, 1, SSD1306_BLACK);
  formatReason(buf, sizeof(buf), *app_->fanStatus);
  drawText(tagW + 4, 55, buf);

  // Motion dot: solid = motion now, ring = idle, blinking ring = PIR warming up
  if (!motion.ready()) {
    if ((nowMs / 400) % 2 == 0) oled_.drawCircle(123, 58, 3, SSD1306_WHITE);
  } else if (motion.active()) {
    oled_.fillCircle(123, 58, 3, SSD1306_WHITE);
  } else {
    oled_.drawCircle(123, 58, 3, SSD1306_WHITE);
  }
}

void DisplayUI::drawMenu(const char* title) {
  drawHeader(title);
  const uint8_t count = rowCount();
  const int16_t valueRight = 122;   // leaves room for the scrollbar
  char value[16];
  char shown[20];

  for (uint8_t i = 0; i < VISIBLE_ROWS; i++) {
    const uint8_t row = scrollTop_ + i;
    if (row >= count) break;
    const int16_t y = HEADER_H + 1 + i * ROW_H;
    const bool selected = (row == cursor_);
    rowValue(row, value, sizeof(value));

    if (selected && editing_) {
      // Editing: value inverted between arrows = "push up/down to change"
      snprintf(shown, sizeof(shown), "<%s>", value);
      const int16_t w = textWidth(shown, 1);
      drawText(2, y, rowLabel(row));
      oled_.fillRect(valueRight - w - 2, y - 1, w + 3, ROW_H, SSD1306_WHITE);
      drawTextRight(valueRight, y, shown, 1, SSD1306_BLACK);
    } else if (selected) {
      oled_.fillRect(0, y - 1, valueRight + 2, ROW_H, SSD1306_WHITE);
      drawText(2, y, rowLabel(row), 1, SSD1306_BLACK);
      drawTextRight(valueRight, y, value, 1, SSD1306_BLACK);
    } else {
      drawText(2, y, rowLabel(row));
      drawTextRight(valueRight, y, value);
    }
  }

  // Scrollbar when the list is longer than the screen
  if (count > VISIBLE_ROWS) {
    const int16_t trackH = OLED_HEIGHT - HEADER_H;
    const int16_t barH = trackH * VISIBLE_ROWS / count;
    const int16_t barY = HEADER_H + (trackH - barH) * scrollTop_ / (count - VISIBLE_ROWS);
    oled_.fillRect(125, barY, 3, barH, SSD1306_WHITE);
  }
}

void DisplayUI::drawTrend() {
  const ClimateSensor& climate = *app_->climate;
  const bool humidity = trendHumidity_;
  const bool fahrenheit = app_->settings->fahrenheit;
  char buf[24];

  snprintf(buf, sizeof(buf), "%s %lumin", humidity ? "Humidity" : "Temp",
           (unsigned long)(TREND_HISTORY_LEN * TREND_SAMPLE_INTERVAL_MS / 60000UL));
  drawHeader(buf);

  // Current value, plus the low / high since boot
  if (humidity) {
    if (climate.valid()) snprintf(buf, sizeof(buf), "Now %.0f%%", climate.humidity());
    else                 snprintf(buf, sizeof(buf), "Now --%%");
  } else {
    char temp[12];
    formatTemp(temp, sizeof(temp), climate.valid() ? climate.temperatureC() : NAN);
    snprintf(buf, sizeof(buf), "Now %s", temp);
  }
  drawText(0, 12, buf);
  if (climate.hasStats()) {
    const float lo = humidity ? climate.minHumidity() : toDisplayTemp(climate.minTempC(), fahrenheit);
    const float hi = humidity ? climate.maxHumidity() : toDisplayTemp(climate.maxTempC(), fahrenheit);
    snprintf(buf, sizeof(buf), "L%.0f H%.0f", lo, hi);
    drawTextRight(OLED_WIDTH, 12, buf);
  }

  // Graph box: x 26..127, y 21..63. Inner area is 100 x 41 px, one pixel per sample.
  const int16_t gx = 26, gy = 21, gw = 102, gh = 43;
  const int16_t innerW = gw - 2, innerH = gh - 2;
  oled_.drawRect(gx, gy, gw, gh, SSD1306_WHITE);

  const uint8_t n = climate.historyCount();
  float vmin = 1e6f, vmax = -1e6f;
  uint8_t points = 0;
  for (uint8_t i = 0; i < n; i++) {
    float v = humidity ? climate.historyHumidity(i) : climate.historyTempC(i);
    if (isnan(v)) continue;
    if (!humidity) v = toDisplayTemp(v, fahrenheit);
    if (v < vmin) vmin = v;
    if (v > vmax) vmax = v;
    points++;
  }
  if (points < 2) {
    drawTextCentered(gx + gw / 2, gy + gh / 2 - 4, "Collecting...");
    return;
  }

  // Keep a minimum span so sensor noise doesn't look like a cliff.
  const float minSpan = humidity ? 6.0f : 2.0f;
  if (vmax - vmin < minSpan) {
    const float mid = (vmax + vmin) / 2.0f;
    vmin = mid - minSpan / 2.0f;
    vmax = mid + minSpan / 2.0f;
  }
  snprintf(buf, sizeof(buf), "%.0f", vmax);
  drawTextRight(gx - 1, gy, buf);
  snprintf(buf, sizeof(buf), "%.0f", vmin);
  drawTextRight(gx - 1, gy + gh - 8, buf);

  // Newest sample on the right edge; gaps (failed reads) break the line.
  const int16_t x0 = gx + 1 + (innerW - n);
  int16_t prevX = -1, prevY = -1;
  for (uint8_t i = 0; i < n; i++) {
    float v = humidity ? climate.historyHumidity(i) : climate.historyTempC(i);
    if (isnan(v)) {
      prevX = -1;
      continue;
    }
    if (!humidity) v = toDisplayTemp(v, fahrenheit);
    const int16_t x = x0 + i;
    const int16_t y = gy + innerH - static_cast<int16_t>(lroundf((v - vmin) * (innerH - 1) / (vmax - vmin)));
    if (prevX >= 0) oled_.drawLine(prevX, prevY, x, y, SSD1306_WHITE);
    else            oled_.drawPixel(x, y, SSD1306_WHITE);
    prevX = x;
    prevY = y;
  }
}

// Everything needed to reach the dashboard: where to point a browser, and in
// hotspot mode which network to join first.
void DisplayUI::drawNetwork() {
  const StationWifi& net = *app_->network;
  char line[40];

  drawHeader("Wi-Fi");
  if (net.hotspotActive()) {
    drawText(0, 12, net.apSsid().c_str());
    snprintf(line, sizeof(line), "Pass %s", strlen(SETUP_AP_PASSWORD) >= 8 ? SETUP_AP_PASSWORD : "(none)");
    drawText(0, 22, line);
    snprintf(line, sizeof(line), "http://%s", net.apIp().c_str());
    drawText(0, 32, line);
    if (net.state() == NetState::Connected) snprintf(line, sizeof(line), "LAN %s", net.ip().c_str());
    else                                    snprintf(line, sizeof(line), "%s", net.lastError());
    drawText(0, 42, line);
  } else if (net.state() == NetState::Connected) {
    drawText(0, 12, net.stationSsid().c_str());
    snprintf(line, sizeof(line), "http://%s", net.ip().c_str());
    drawText(0, 22, line);
    drawText(0, 32, WIFI_HOSTNAME ".local");
    snprintf(line, sizeof(line), "Signal %d dBm", net.rssi());
    drawText(0, 42, line);
  } else {
    drawText(0, 12, "Connecting to");
    drawText(0, 22, net.stationSsid().c_str());
  }

  oled_.drawFastHLine(0, 53, OLED_WIDTH, SSD1306_WHITE);
  drawText(0, 56, net.hotspotActive() ? "Press: hotspot off" : "Press: open hotspot");
}

// Home header: signal bars when on Wi-Fi, "AP" while only the hotspot is up.
void DisplayUI::drawWifiStatus(int16_t x) {
  const StationWifi& net = *app_->network;
  if (net.state() == NetState::Connected) {
    const int rssi = net.rssi();
    const uint8_t bars = rssi >= -55 ? 4 : rssi >= -67 ? 3 : rssi >= -78 ? 2 : 1;
    for (uint8_t i = 0; i < 4; i++) {
      const int16_t h = 2 + i * 2;
      if (i < bars) oled_.drawFastVLine(x + i * 2, 8 - h, h, SSD1306_WHITE);
      else          oled_.drawPixel(x + i * 2, 7, SSD1306_WHITE);
    }
  } else if (net.hotspotActive()) {
    drawText(x - 1, 0, "AP");
  }
}

void DisplayUI::drawInfo(uint32_t nowMs) {
  const ClimateSensor& climate = *app_->climate;
  const MotionSensor& motion = *app_->motion;
  const Joystick& joy = *app_->joystick;
  char line[28];
  char t[16];

  drawHeader("Info");

  formatDuration(t, sizeof(t), nowMs);
  snprintf(line, sizeof(line), "Up %s", t);
  drawText(0, 11, line);
  drawTextRight(OLED_WIDTH, 11, "v" FIRMWARE_VERSION);

  snprintf(line, sizeof(line), "Motion events: %lu", (unsigned long)motion.eventCount());
  drawText(0, 20, line);

  formatDuration(t, sizeof(t), app_->fan->runtimeMs());
  snprintf(line, sizeof(line), "Fan on-time %s", t);
  drawText(0, 29, line);

  snprintf(line, sizeof(line), "DHT%u ok %lu bad %lu", DHT_TYPE,
           (unsigned long)climate.goodReads(), (unsigned long)climate.failedReads());
  drawText(0, 38, line);

  // Live joystick readings: handy for checking wiring and JOY_INVERT_* settings
  if (joy.stickOk()) snprintf(line, sizeof(line), "Stick %d,%d%s", joy.rawX(), joy.rawY(), joy.buttonHeld() ? " BTN" : "");
  else               snprintf(line, sizeof(line), "Stick not found%s", joy.buttonHeld() ? " BTN" : "");
  drawText(0, 47, line);

  // PIR pin level, light level, and the code of the last remote key received
  char key[4] = "--";
  if (app_->remote->everReceived()) snprintf(key, sizeof(key), "%02X", app_->remote->lastCommand());
  snprintf(line, sizeof(line), "PIR %s  L%u%%  IR %s", motion.rawLevel() ? "hi" : "lo",
           app_->light->percent(), key);
  drawText(0, 56, line);
}

void DisplayUI::drawToast() {
  const int16_t w = textWidth(toast_, 1) + 12;
  const int16_t x = (OLED_WIDTH - w) / 2;
  oled_.fillRect(x, 22, w, 19, SSD1306_BLACK);
  oled_.drawRect(x, 22, w, 19, SSD1306_WHITE);
  drawText(x + 6, 28, toast_);
}

void DisplayUI::drawFanIcon(int16_t cx, int16_t cy, int16_t r, float angle) {
  oled_.drawCircle(cx, cy, r + 2, SSD1306_WHITE);
  for (uint8_t blade = 0; blade < 3; blade++) {
    const float a = angle + blade * (TWO_PI_F / 3.0f);
    const int16_t x1 = cx + static_cast<int16_t>(lroundf(cosf(a - 0.4f) * r));
    const int16_t y1 = cy + static_cast<int16_t>(lroundf(sinf(a - 0.4f) * r));
    const int16_t x2 = cx + static_cast<int16_t>(lroundf(cosf(a + 0.4f) * r));
    const int16_t y2 = cy + static_cast<int16_t>(lroundf(sinf(a + 0.4f) * r));
    oled_.fillTriangle(cx, cy, x1, y1, x2, y2, SSD1306_WHITE);
  }
  oled_.fillCircle(cx, cy, 2, SSD1306_BLACK);   // hub
  oled_.drawCircle(cx, cy, 2, SSD1306_WHITE);
}

void DisplayUI::drawText(int16_t x, int16_t y, const char* s, uint8_t size, uint16_t color) {
  oled_.setTextSize(size);
  oled_.setTextColor(color);
  oled_.setCursor(x, y);
  oled_.print(s);
}

void DisplayUI::drawTextRight(int16_t xRight, int16_t y, const char* s, uint8_t size, uint16_t color) {
  drawText(xRight - textWidth(s, size), y, s, size, color);
}

void DisplayUI::drawTextCentered(int16_t xCenter, int16_t y, const char* s, uint8_t size) {
  drawText(xCenter - textWidth(s, size) / 2, y, s, size);
}

// ---------------------------------------------------------------------------
//  Panel control
// ---------------------------------------------------------------------------
void DisplayUI::applyDisplaySettings() {
  const Settings& s = *app_->settings;
  // Auto dim: in a dark room the panel drops to the Low level.
  const bool dim = s.autoDim && app_->light->dark();
  const uint8_t contrast = BRIGHTNESS_CONTRAST[dim ? 0 : s.brightness];
  if (appliedContrast_ != contrast) {
    appliedContrast_ = contrast;
    oled_.ssd1306_command(SSD1306_SETCONTRAST);
    oled_.ssd1306_command(contrast);
  }
  const int8_t flip = s.flipDisplay ? 1 : 0;
  if (appliedFlip_ != flip) {
    appliedFlip_ = flip;
    oled_.setRotation(flip ? 2 : 0);   // 180 degrees, for a board mounted upside down
  }
}

void DisplayUI::setPanelOn(bool on, uint32_t nowMs) {
  panelOn_ = on;
  oled_.ssd1306_command(on ? SSD1306_DISPLAYON : SSD1306_DISPLAYOFF);
  if (on) lastFrameMs_ = nowMs - UI_FRAME_MS;   // draw a fresh frame right away
}

void DisplayUI::formatTemp(char* buf, size_t len, float tempC) const {
  const bool fahrenheit = app_->settings->fahrenheit;
  const char unit = fahrenheit ? 'F' : 'C';
  if (isnan(tempC)) snprintf(buf, len, "--.-%c%c", DEGREE_CHAR, unit);
  else              snprintf(buf, len, "%.1f%c%c", toDisplayTemp(tempC, fahrenheit), DEGREE_CHAR, unit);
}
