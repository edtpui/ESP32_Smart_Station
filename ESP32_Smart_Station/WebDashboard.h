#pragma once
#include <Arduino.h>
#include <WebServer.h>
#include "AppContext.h"

// HTTP server: serves the dashboard page from flash and the JSON API it uses.
//
//   GET  /                    dashboard page (gzipped, see dashboard/ folder)
//   GET  /api/status          live readings, settings and network state
//   GET  /api/history         trend samples for the chart
//   POST /api/fan             mode=auto|toggle|on|off, speed=10-100
//   POST /api/settings        holdSeconds, tempGateOn, gateTempC, fahrenheit,
//                             screenSleep, brightness, flipDisplay, autoDim,
//                             soundOn, motionChime, climateAlert, alertTempC,
//                             alertHumidity
//   POST /api/buzzer/test     sounds two beeps
//   POST /api/reset-stats     clears min/max, motion count and fan on-time
//   POST /api/factory-reset   restores default settings (keeps Wi-Fi)
//   GET  /api/wifi/scan       starts a scan; poll until "scanning" is false
//   POST /api/wifi            ssid, password, username, identity
//   POST /api/wifi/forget     forgets the network saved from the dashboard
class WebDashboard {
public:
  WebDashboard();
  void begin(AppContext* app);
  void update();

private:
  typedef void (WebDashboard::*Handler)();
  void route(const char* path, HTTPMethod method, Handler handler);
  bool authorized();
  void sendJson(const String& json, int code = 200);
  void sendError(int code, const char* message);
  void sendStatus();

  void handleIndex();
  void handleHistory();
  void handleFan();
  void handleSettings();
  void handleResetStats();
  void handleBuzzerTest();
  void handleFactoryReset();
  void handleWifiScan();
  void handleWifiSave();
  void handleWifiForget();
  void handleNotFound();

  WebServer   server_;
  AppContext* app_ = nullptr;
};
