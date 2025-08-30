#pragma once
#include <Arduino.h>

struct Settings {
  String wifiName = "Brook";
  String wifiPassword = "TryMyWiFi";
  String mdnsName = "esp32s3"; // http://<name>.local
  int ledMaxBrightness = 10;   // 0..255
  int pictureSpeedSeconds = 2; // rotation interval (seconds)
  int pageRefreshSeconds = 5;  // HTML fallback refresh (seconds)
};

// Loads friendly settings from FFat at /you-can-change-this/settings.txt
// Missing file or keys keep defaults.
void loadSettingsFromFFat(Settings &out);
