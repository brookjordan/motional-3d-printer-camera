#pragma once
#include "manager_base.h"

class SystemManager : public ManagerBase {
public:
  void setup() override;
  void loop() override;
  const char* getName() const override { return "SystemManager"; }
  
  // Singleton access
  static SystemManager& getInstance() {
    static SystemManager instance;
    return instance;
  }
};