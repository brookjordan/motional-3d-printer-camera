#pragma once
#include <Arduino.h>

struct PageWords {
  String pageTitle = "ESP32-S3 Secrets";
  String heading = "ESP32-S3 Secrets";
  String helpLine = "Auto-refreshing diagnostics. Becomes live when JavaScript loads.";
};

void loadPageWords(PageWords &out);
