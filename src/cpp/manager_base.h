#pragma once

// Base class for all system managers
class ManagerBase {
public:
  virtual ~ManagerBase() = default;
  
  // Pure virtual functions that all managers must implement
  virtual void setup() = 0;
  virtual void loop() = 0;
  
  // Optional virtual functions with default implementations
  virtual const char* getName() const = 0;
};