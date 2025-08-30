// Minimal app entry wiring modules together
#include <Arduino.h>
#include <FFat.h>
#include <ESPAsyncWebServer.h>

#include "settings.h"
#include "wifi_and_name.h"
#include "website_routes.h"
#include "light_breathing.h"
#include "pictures.h"

AsyncWebServer srvr(80);

void setup() {
  Serial.begin(115200);
  delay(200);

  if (!FFat.begin()) {
    Serial.println("FFat mount failed");
  }

  Settings settings; // defaults
  loadSettingsFromFFat(settings);

  // Routes (static files + dynamic endpoints)
  setupRoutes(srvr, settings);

  // Network
  connectWiFi(settings);

  // Apply settings for modules and start them
  LedBreath::setMaxBrightness(settings.ledMaxBrightness);
  ImageRotator::setIntervalMs((unsigned long)settings.pictureSpeedSeconds * 1000UL);
  LedBreath::setup();
  ImageRotator::setup();

  srvr.begin();
  Serial.println("HTTP server started");
  Serial.println("Edit settings at /you-can-change-this/settings.txt");
}

void loop() {
  LedBreath::loop();
  ImageRotator::tick();
}
