#include "debug_manager.h"
#include "config.h"
#include <Arduino.h>
extern "C" {
#include "esp_system.h"
}

void DebugManager::setup() {
  Serial.begin(SERIAL_BAUD_RATE);
  delay(DEBUG_STARTUP_DELAY_MS);
  Serial.printf("[%s] Starting setup...\n", getName());
  
  // Debug reset reason
  esp_reset_reason_t reason = esp_reset_reason();
  Serial.printf("Reset reason: %d - ", (int)reason);
  switch (reason) {
  case ESP_RST_POWERON:
    Serial.println("Power on");
    break;
  case ESP_RST_EXT:
    Serial.println("External reset");
    break;
  case ESP_RST_SW:
    Serial.println("Software reset");
    break;
  case ESP_RST_PANIC:
    Serial.println("Exception/panic");
    break;
  case ESP_RST_INT_WDT:
    Serial.println("Interrupt watchdog");
    break;
  case ESP_RST_TASK_WDT:
    Serial.println("Task watchdog");
    break;
  case ESP_RST_WDT:
    Serial.println("Other watchdog");
    break;
  case ESP_RST_DEEPSLEEP:
    Serial.println("Deep sleep");
    break;
  case ESP_RST_BROWNOUT:
    Serial.println("Brownout");
    break;
  case ESP_RST_SDIO:
    Serial.println("SDIO reset");
    break;
  default:
    Serial.println("Unknown");
    break;
  }
}

void DebugManager::loop() {
  // Feed watchdog to prevent resets
  delay(1); // Small yield to prevent tight loop
}