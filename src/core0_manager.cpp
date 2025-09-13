#include "core0_manager.h"
#include "camera_cycle.h"
#include "led_breathe.h"
#include "config.h"
#include <Arduino.h>
extern "C" {
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
}

static TaskHandle_t cameraTaskHandle = nullptr;

// Core 0 camera task - continuous operation
static void cameraTask(void* parameter) {
  Serial.println("Core 0: Camera task started");
  
  // Initialize both camera and LED on Core 0
  Serial.println("Core 0: Initializing camera...");
  CameraCycle::setup();
  Serial.println("Core 0: Camera setup complete");
  
  Serial.println("Core 0: Initializing LED...");
  LEDBreathe::setup();
  
  Serial.println("Core 0: Camera + LED task running - continuous operation");
  
  uint32_t lastHealthCheck = millis();
  
  // Continuous operation on Core 0
  for (;;) {
    CameraCycle::loop();  // Camera capture every 3 seconds
    LEDBreathe::loop();   // LED breathing animation
    
    // Health check every 30 seconds
    uint32_t now = millis();
    if (now - lastHealthCheck > 30000) {
      Serial.printf("Core 0: Health check - Free heap: %d bytes\n", ESP.getFreeHeap());
      lastHealthCheck = now;
      
      // Reset if memory is critically low
      if (ESP.getFreeHeap() < 30000) {
        Serial.println("Core 0: Critical memory low - restarting ESP32");
        ESP.restart();
      }
    }
    
    vTaskDelay(1);        // Yield to other tasks
  }
}

void Core0Manager::setup() {
  Serial.printf("[%s] Starting setup...\n", getName());
  
  Serial.println("Testing camera and LED system...");
  
  // Create camera task pinned to Core 0
  xTaskCreatePinnedToCore(
    cameraTask,              // Task function
    "camera_core0",         // Task name
    CAMERA_TASK_STACK_SIZE,  // Stack size
    nullptr,                 // Parameters
    CAMERA_TASK_PRIORITY,    // Priority
    &cameraTaskHandle,       // Task handle
    CAMERA_TASK_CORE         // Pin to Core
  );
  
  Serial.println("Camera task created on Core 0");
  Serial.printf("[%s] Setup complete\n", getName());
}

void Core0Manager::loop() {
  // Camera runs on dedicated Core 0 task
  // This loop is unused
}
