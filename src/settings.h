#pragma once
#include <Arduino.h>

struct Settings {
  // Wi‑Fi SSID: min 1, max 32 bytes (802.11 SSID length)
  String wifiName = "wifiName";

  // Wi‑Fi password: 0 (open network) or 8..63 ASCII chars (WPA/WPA2‑PSK)
  String wifiPassword = "wifiPassword";

  // mDNS host label: 1..63 chars, recommend [a‑z0‑9-], e.g. http://<name>.local
  String mdnsName = "esp32s3"; // http://<name>.local

  // LED max brightness: 0..255
  int ledMaxBrightness = 10; // min=0, max=255

  // Image rotation interval (seconds): 0..86400; effective min clamp is 0.25s
  int pictureSpeedSeconds = 2; // min=0, max=86400 (0 -> 250 ms effective min)

  // HTML page auto‑refresh (seconds): 1..86400 recommended
  int pageRefreshSeconds = 5; // min=1, max=86400
};

// Loads friendly settings from FFat at /you-can-change-this/settings.txt
// Missing file or keys keep defaults.
void loadSettingsFromFFat(Settings &out);
