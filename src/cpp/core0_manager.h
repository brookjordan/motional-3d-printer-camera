#pragma once
#include "manager_base.h"

class Core0Manager : public ManagerBase {
public:
  void setup() override;
  void loop() override; // Note: Core0 doesn't have a traditional loop
  const char* getName() const override { return "Core0Manager"; }
  
  // Singleton access
  static Core0Manager& getInstance() {
    static Core0Manager instance;
    return instance;
  }
};