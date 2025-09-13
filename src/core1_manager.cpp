#include "core1_manager.h"
#include "config.h"
#include "website_routes.h"
#include "wifi_and_name.h"
#include <ESPAsyncWebServer.h>
#include <WiFi.h>
#include <ESPmDNS.h>

// Web server instance
static AsyncWebServer webServer(WEB_SERVER_PORT);

void Core1Manager::setup() {
  Serial.printf("[%s] Starting setup...\n", getName());
  
  // Robust Wi‑Fi connect (handles WPA2‑Enterprise and SoftAP fallback)
  connectWiFi();

  // Start web server regardless (works in STA or SoftAP)
  setupRoutes(webServer);
  webServer.begin();
  Serial.println("Web server started");
  
  Serial.printf("[%s] Setup complete\n", getName());
}

void Core1Manager::loop() {
  // Core 1 handles web server (AsyncWebServer runs automatically)
  // File reads happen here when serving images
  delay(10); // Small yield for web server operations
}
