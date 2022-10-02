#pragma once

#include <unordered_set>

#include "component.hpp"

class Entity {
 public:
  template <typename ComponentType>
  bool AttachComponent(const ComponentType& component) {
    auto result = components_.emplace(component);
    return result.second;
  }

 private:
  std::unordered_set<Component> components_;
};