#pragma once
#include <Arduino.h>

struct Settings {
  // Wi-Fi SSID: min 1, max 32 bytes (802.11 SSID length)
  String wifiName;

  // Wi-Fi password: 0 (open network) or 8..63 ASCII chars (WPA/WPA2-PSK)
  String wifiPassword;

  // mDNS host label: 1..63 chars, recommend [a-z0-9-], e.g. http://<name>.local
  String mdnsName; // http://<name>.local

  // LED max brightness: 0..255
  int ledMaxBrightness; // min=0, max=255

  // Image rotation interval (seconds): 0..86400; effective min clamp is 0.25s
  int pictureSpeedSeconds; // min=0, max=86400 (0 -> 250 ms effective min)

  // HTML page auto-refresh (seconds): 1..86400 recommended
  int pageRefreshSeconds; // min=1, max=86400

  // Enterprise Wi-Fi (WPA2-Enterprise / 802.1X)
  // Enable when connecting to networks that require username + password.
  // If enabled, plain WPA2-PSK password is ignored.
  bool useEnterprise;      // min=false, max=true
  String eapUsername;      // min 1, max ~128 (depends on RADIUS)
  String eapPassword;      // min 1, max ~128
  String eapOuterIdentity; // optional; if empty uses username
  String
      eapCaCertPath; // optional path in FFat, e.g. /you-can-change-this/ca.pem
};

// Loads all settings from FFat at /you-can-change-this/settings.txt
// (single unified file: Wi-Fi + enterprise + UI settings)
void loadSettingsFromFFat(Settings &out);
