#include "system_manager.h"
#include "config.h"
#include <FFat.h>

void SystemManager::setup() {
  Serial.printf("[%s] Starting setup...\n", getName());
  Serial.println("Setting up shared system resources...");
  
  // Filesystem - shared between cores for camera files and web serving
  if (!FFat.begin()) {
    Serial.println("FFat mount failed");
  }

  // Configuration loaded from config.h at compile time
  Serial.println("Configuration loaded:");
  Serial.printf("  SSID: %s\n", WIFI_SSID);
  Serial.printf("  mDNS name: %s\n", MDNS_HOSTNAME);
  Serial.printf("  Enterprise: %s\n", USE_ENTERPRISE_WIFI ? "yes" : "no");
  Serial.printf("  LED brightness: %d\n", LED_MAX_BRIGHTNESS);
  Serial.printf("  Breath cycle: %.1f seconds (continuous)\n", CAMERA_BREATH_CYCLE_MS / 1000.0f);
  Serial.printf("  Page refresh: %d seconds\n", PAGE_REFRESH_SECONDS);
  
  if (USE_ENTERPRISE_WIFI) {
    const char *oi = EAP_OUTER_IDENTITY;
    Serial.printf("  EAP user: %s\n", EAP_USERNAME ? EAP_USERNAME : "");
    Serial.printf("  Outer identity: %s\n", (oi && *oi) ? oi : "<empty>");
  }
  
  Serial.println("Shared system setup complete");
  Serial.printf("[%s] Setup complete\n", getName());
}

void SystemManager::loop() {
  // System-level operations that can't be pinned to specific cores
  // Currently empty - add cross-core coordination here if needed
  // Examples: system health monitoring, inter-core communication
}

// Settings are now accessed directly from config.h macros
