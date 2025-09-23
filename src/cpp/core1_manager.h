#pragma once
#include "manager_base.h"

class Core1Manager : public ManagerBase {
public:
  void setup() override;
  void loop() override;
  const char* getName() const override { return "Core1Manager"; }
  
  // Singleton access
  static Core1Manager& getInstance() {
    static Core1Manager instance;
    return instance;
  }
};