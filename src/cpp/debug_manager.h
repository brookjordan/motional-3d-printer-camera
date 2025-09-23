#pragma once
#include "manager_base.h"

class DebugManager : public ManagerBase {
public:
  void setup() override;
  void loop() override;
  const char* getName() const override { return "DebugManager"; }
  
  // Singleton access
  static DebugManager& getInstance() {
    static DebugManager instance;
    return instance;
  }
};