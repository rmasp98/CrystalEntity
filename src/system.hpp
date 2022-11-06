#pragma once

#include <atomic>

class System {
 public:
  using Id = uint64_t;

  virtual ~System() = default;

  virtual void Tick(double const deltaTime) = 0;

  template <typename SystemClass>
  static System::Id GetId() {
    return typeid(SystemClass).hash_code();
  }

  Id GetId() const { return systemId_; }

 protected:
  System() = default;

 private:
  friend class Manager;
  Id systemId_;
};