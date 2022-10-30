#pragma once

#include <atomic>

class System {
 public:
  using Id = int32_t;

  virtual ~System() = default;

  virtual void Initialise() = 0;
  virtual void Tick(double const deltaTime) = 0;

  Id GetId() const { return systemId_; }

 protected:
  System() : systemId_(systemIdCounter_++) {}

 private:
  inline static std::atomic<Id> systemIdCounter_ = 0;
  Id systemId_;
};