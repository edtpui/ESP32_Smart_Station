#pragma once
#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "AppContext.h"

enum class UiPage : uint8_t { Home = 0, Fan, Trend, Network, System, Info };
constexpr uint8_t UI_PAGE_COUNT = 6;

// Draws the OLED pages and turns joystick events into menu actions.
//
//   Left / Right .... switch page (Home, Fan, Trend, Wi-Fi, System, Info)
//   Up / Down ....... move the cursor          (Home: fan speed +/-)
//   Press ........... edit / confirm a value   (Home: cycle fan mode,
//                                               Wi-Fi: hotspot on / off)
//   Long press ...... back to Home             (on "Factory reset": reset)
class DisplayUI {
public:
  DisplayUI();
  bool begin(AppContext* app);                     // false if no OLED answers on I2C
  void handleEvent(JoyEvent event, uint32_t nowMs);
  void keepAwake(uint32_t nowMs);                  // motion / activity resets the sleep timer
  void update(uint32_t nowMs);                     // redraws at UI_FRAME_MS
  void showToast(const char* message, uint32_t nowMs);

private:
  // Pages
  void render(uint32_t nowMs);
  void drawHeader(const char* title);
  void drawHome(uint32_t nowMs);
  void drawMenu(const char* title);
  void drawTrend();
  void drawNetwork();
  void drawWifiStatus(int16_t x);
  void drawInfo(uint32_t nowMs);
  void drawToast();
  void drawFanIcon(int16_t cx, int16_t cy, int16_t r, float angle);
  void drawText(int16_t x, int16_t y, const char* s, uint8_t size = 1, uint16_t color = SSD1306_WHITE);
  void drawTextRight(int16_t xRight, int16_t y, const char* s, uint8_t size = 1, uint16_t color = SSD1306_WHITE);
  void drawTextCentered(int16_t xCenter, int16_t y, const char* s, uint8_t size = 1);

  // Menu model for the Fan and System pages
  uint8_t     rowCount() const;
  const char* rowLabel(uint8_t row) const;
  void        rowValue(uint8_t row, char* buf, size_t len) const;
  bool        rowIsAction(uint8_t row) const;
  void        changeRow(uint8_t row, int delta, uint32_t nowMs);
  void        activateRow(uint8_t row, bool longPress, uint32_t nowMs);

  void goToPage(UiPage page);
  void applyDisplaySettings();
  void setPanelOn(bool on, uint32_t nowMs);
  void formatTemp(char* buf, size_t len, float tempC) const;

  Adafruit_SSD1306 oled_;
  AppContext* app_ = nullptr;
  bool     present_ = false;
  bool     panelOn_ = true;
  uint32_t lastWakeMs_ = 0;
  uint32_t lastFrameMs_ = 0;
  uint32_t splashUntilMs_ = 0;
  UiPage   page_ = UiPage::Home;
  uint8_t  cursor_ = 0;
  uint8_t  scrollTop_ = 0;
  bool     editing_ = false;
  bool     trendHumidity_ = false;
  float    fanAngle_ = 0.0f;
  char     toast_[20] = {0};
  uint32_t toastUntilMs_ = 0;
  int16_t  appliedContrast_ = -1;
  int8_t   appliedFlip_ = -1;
  bool     recalPending_ = false;
  uint32_t recalAtMs_ = 0;
};
