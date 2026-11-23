#include "StationWifi.h"
#include <ESPmDNS.h>
#include <WiFi.h>
#include "Config.h"

void StationWifi::begin() {
  prefs_.begin("wifi", false);

  const uint64_t mac = ESP.getEfuseMac();
  char name[24];
  snprintf(name, sizeof(name), "SmartStation-%02X%02X",
           static_cast<unsigned>(mac >> 32) & 0xFF, static_cast<unsigned>(mac >> 40) & 0xFF);
  apSsid_ = name;

  loadCredentials();
  WiFi.persistent(false);   // credentials are managed here, not in the driver's own storage
  WiFi.setHostname(WIFI_HOSTNAME);
  WiFi.mode(WIFI_STA);
  if (hasCredentials()) connectStation(millis());
  else                  startHotspot(false);
}

void StationWifi::update(uint32_t nowMs) {
  if (hotspot_) dns_.processNextRequest();

  if (applyPending_ && static_cast<int32_t>(nowMs - applyAtMs_) >= 0) {
    applyPending_ = false;
    applyCredentials(nowMs);
  }

  const bool up = WiFi.status() == WL_CONNECTED;
  if (up != linkUp_) {
    linkUp_ = up;
    if (up) {
      onLinkUp();
    } else {
      Serial.println(F("[WIFI] connection lost"));
      attempting_ = true;         // the driver reconnects by itself; the hotspot is the fallback
      attemptStartMs_ = nowMs;
    }
  }
  if (up || !hasCredentials()) return;

  if (attempting_) {
    if (nowMs - attemptStartMs_ < WIFI_CONNECT_TIMEOUT_MS) return;
    switch (WiFi.status()) {
      case WL_NO_SSID_AVAIL:  lastError_ = "Network not found"; break;
      case WL_CONNECT_FAILED: lastError_ = "Login rejected"; break;
      default:                lastError_ = "Connection timed out"; break;
    }
    Serial.printf("[WIFI] could not join \"%s\": %s\n", ssid_.c_str(), lastError_);
    attempting_ = false;
    lastAttemptEndMs_ = nowMs;
    WiFi.disconnect(false, false);   // stops channel scanning, which would upset hotspot clients
    if (!hotspot_) startHotspot(false);
  } else if (hotspot_ && hotspotClients() == 0 && nowMs - lastAttemptEndMs_ >= WIFI_RETRY_INTERVAL_MS) {
    connectStation(nowMs);
  }
}

void StationWifi::saveCredentials(const String& ssid, const String& password, const String& username,
                                     const String& identity, uint32_t nowMs) {
  prefs_.putString("ssid", ssid);
  prefs_.putString("pass", password);
  prefs_.putString("user", username);
  prefs_.putString("ident", identity);
  Serial.printf("[WIFI] saved network \"%s\"\n", ssid.c_str());
  loadCredentials();
  scheduleApply(nowMs);
}

void StationWifi::forgetSavedCredentials(uint32_t nowMs) {
  prefs_.clear();
  Serial.println(F("[WIFI] saved network forgotten"));
  loadCredentials();
  scheduleApply(nowMs);
}

bool StationWifi::setHotspot(bool on) {
  if (on == hotspot_) return true;
  if (on) {
    startHotspot(true);
    return true;
  }
  if (!hasCredentials()) return false;
  stopHotspot();
  if (!linkUp_) connectStation(millis());
  return true;
}

NetState StationWifi::state() const {
  if (linkUp_) return NetState::Connected;
  return hotspot_ ? NetState::Hotspot : NetState::Connecting;
}

String StationWifi::ip() const {
  return linkUp_ ? WiFi.localIP().toString() : String();
}

String StationWifi::apIp() const {
  return hotspot_ ? WiFi.softAPIP().toString() : String();
}

String StationWifi::macAddress() const {
  return WiFi.macAddress();
}

int StationWifi::rssi() const {
  return linkUp_ ? WiFi.RSSI() : 0;
}

uint8_t StationWifi::hotspotClients() const {
  return hotspot_ ? WiFi.softAPgetStationNum() : 0;
}

void StationWifi::loadCredentials() {
  savedInFlash_ = prefs_.isKey("ssid");
  if (savedInFlash_) {
    ssid_ = prefs_.getString("ssid");
    password_ = prefs_.getString("pass");
    username_ = prefs_.getString("user");
    identity_ = prefs_.getString("ident");
  } else {
    ssid_ = WIFI_SSID;
    password_ = WIFI_PASSWORD;
    username_ = WIFI_EAP_USERNAME;
    identity_ = WIFI_EAP_IDENTITY;
  }
}

// Switching networks drops the browser's connection, so wait for the HTTP reply to go out first.
void StationWifi::scheduleApply(uint32_t nowMs) {
  applyPending_ = true;
  applyAtMs_ = nowMs + WIFI_APPLY_DELAY_MS;
}

void StationWifi::applyCredentials(uint32_t nowMs) {
  WiFi.disconnect(false, false);
  lastError_ = "";
  if (hasCredentials()) {
    connectStation(nowMs);
  } else {
    attempting_ = false;
    if (!hotspot_) startHotspot(false);
  }
}

void StationWifi::connectStation(uint32_t nowMs) {
  Serial.printf("[WIFI] connecting to \"%s\"\n", ssid_.c_str());
  attempting_ = true;
  attemptStartMs_ = nowMs;
#if CONFIG_ESP_WIFI_ENTERPRISE_SUPPORT
  if (enterprise()) {
    const String& identity = identity_.length() > 0 ? identity_ : username_;
    WiFi.begin(ssid_.c_str(), WPA2_AUTH_PEAP, identity.c_str(), username_.c_str(), password_.c_str());
    return;
  }
#endif
  WiFi.begin(ssid_.c_str(), password_.c_str());
}

void StationWifi::startHotspot(bool manual) {
  WiFi.mode(WIFI_AP_STA);   // station side stays available for scans and retries
  const char* password = strlen(SETUP_AP_PASSWORD) >= 8 ? SETUP_AP_PASSWORD : nullptr;
  WiFi.softAP(apSsid_.c_str(), password);
  dns_.start(53, "*", WiFi.softAPIP());   // any hostname leads to the dashboard (captive portal)
  hotspot_ = true;
  hotspotManual_ = manual;
  Serial.printf("[WIFI] hotspot \"%s\" open, dashboard at http://%s\n", apSsid_.c_str(), apIp().c_str());
}

void StationWifi::stopHotspot() {
  dns_.stop();
  WiFi.softAPdisconnect(true);
  WiFi.mode(WIFI_STA);
  hotspot_ = false;
  hotspotManual_ = false;
  Serial.println(F("[WIFI] hotspot closed"));
}

void StationWifi::onLinkUp() {
  attempting_ = false;
  lastError_ = "";
  WiFi.setSleep(false);   // modem power-save makes the dashboard noticeably laggy
  MDNS.end();
  if (MDNS.begin(WIFI_HOSTNAME)) MDNS.addService("http", "tcp", WEB_PORT);
  Serial.printf("[WIFI] connected to \"%s\", dashboard at http://%s or http://%s.local (%d dBm)\n",
                ssid_.c_str(), WiFi.localIP().toString().c_str(), WIFI_HOSTNAME, WiFi.RSSI());
  if (hotspot_ && !hotspotManual_) stopHotspot();
}
