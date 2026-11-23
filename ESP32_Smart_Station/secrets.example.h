#pragma once
// =============================================================================
//  secrets.example.h - copy this file to secrets.h and fill in your details.
//  secrets.h is git-ignored. Without it, Config.h falls back to empty Wi-Fi
//  details and the station starts its setup hotspot (http://192.168.4.1).
//
//  A network saved from the dashboard takes priority over this file.
// =============================================================================

// Wi-Fi network name (SSID) and password
#define WIFI_SSID          ""
#define WIFI_PASSWORD      ""

// Only for networks where you log in with a username (WPA2-Enterprise: eduroam
// and most campus Wi-Fi). Leave both empty for home Wi-Fi and phone hotspots.
#define WIFI_EAP_USERNAME  ""
#define WIFI_EAP_IDENTITY  ""   // outer identity; empty = same as the username

// Password of the station's own setup hotspot (8+ characters, or empty for an
// open hotspot).
#define SETUP_AP_PASSWORD  "smartstation"

// Dashboard login. Leave the password empty for no login.
#define DASHBOARD_USERNAME "admin"
#define DASHBOARD_PASSWORD ""
