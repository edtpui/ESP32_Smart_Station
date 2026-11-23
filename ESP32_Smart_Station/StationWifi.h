#pragma once
#include <Arduino.h>
#include <DNSServer.h>
#include <Preferences.h>

enum class NetState : uint8_t { Connecting, Connected, Hotspot };

// Joins the configured network (home WPA2 or WPA2-Enterprise). When there is no
// network to join, or it can't be reached, it opens a setup hotspot so the
// dashboard is still available at http://192.168.4.1 and retries in the background.
class StationWifi {
public:
  void begin();
  void update(uint32_t nowMs);

  void saveCredentials(const String& ssid, const String& password, const String& username,
                       const String& identity, uint32_t nowMs);
  void forgetSavedCredentials(uint32_t nowMs);
  bool setHotspot(bool on);   // false if it must stay on (no network to fall back to)

  NetState    state() const;
  bool        hotspotActive() const  { return hotspot_; }
  bool        savedInFlash() const   { return savedInFlash_; }
  bool        enterprise() const     { return username_.length() > 0; }
  const char* lastError() const      { return lastError_; }
  String      stationSsid() const    { return ssid_; }
  String      apSsid() const         { return apSsid_; }
  String      ip() const;
  String      apIp() const;
  String      macAddress() const;
  int         rssi() const;
  uint8_t     hotspotClients() const;

private:
  bool hasCredentials() const { return ssid_.length() > 0; }
  void loadCredentials();
  void scheduleApply(uint32_t nowMs);
  void applyCredentials(uint32_t nowMs);
  void connectStation(uint32_t nowMs);
  void startHotspot(bool manual);
  void stopHotspot();
  void onLinkUp();

  Preferences prefs_;
  DNSServer   dns_;
  String      ssid_;
  String      password_;
  String      username_;
  String      identity_;
  String      apSsid_;
  bool        savedInFlash_ = false;
  bool        hotspot_ = false;
  bool        hotspotManual_ = false;
  bool        linkUp_ = false;
  bool        attempting_ = false;
  bool        applyPending_ = false;
  uint32_t    attemptStartMs_ = 0;
  uint32_t    lastAttemptEndMs_ = 0;
  uint32_t    applyAtMs_ = 0;
  const char* lastError_ = "";
};
