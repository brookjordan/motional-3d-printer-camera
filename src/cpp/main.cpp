// Clean modular dual-core camera system
#include <Arduino.h>
#include "debug_manager.h"
#include "system_manager.h"
#include "core1_manager.h"
#include "core0_manager.h"

// Manager instances
auto& debugManager = DebugManager::getInstance();
auto& systemManager = SystemManager::getInstance();
auto& core1Manager = Core1Manager::getInstance();
auto& core0Manager = Core0Manager::getInstance();

void setup() {
  debugManager.setup();
  systemManager.setup();
  core1Manager.setup();
  core0Manager.setup();
}

void loop() {
  debugManager.loop();
  systemManager.loop();
  core1Manager.loop();
  core0Manager.loop();
}
