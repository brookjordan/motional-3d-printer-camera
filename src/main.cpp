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

  Settings settings{}; // zero/empty-init; all values come from settings.txt
  loadSettingsFromFFat(settings); // unified settings (includes enterprise creds)

  Serial.println("Loaded settings:");
  Serial.printf("  SSID: %s\n", settings.wifiName.c_str());
  Serial.printf("  Enterprise: %s\n", settings.useEnterprise ? "yes" : "no");
  if (settings.useEnterprise) {
    Serial.printf("  EAP user: %s\n", settings.eapUsername.c_str());
    Serial.printf("  Outer identity: %s\n",
                  settings.eapOuterIdentity.length() ? settings.eapOuterIdentity.c_str() : "<empty>");
    Serial.printf("  CA path: %s\n",
                  settings.eapCaCertPath.length() ? settings.eapCaCertPath.c_str() : "<none>");
  }

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
