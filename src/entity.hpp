#pragma once

#include <atomic>
#include <unordered_set>

#include "component.hpp"

class Entity {
 public:
  using EntityId = ComponentManager::EntityId;

  explicit Entity(std::shared_ptr<ComponentManager> const& componentManager)
      : entityId_(entityIdCounter_++), componentManager_(componentManager) {}

  auto operator<=>(Entity const& rhs) const {
    return entityId_ <=> rhs.entityId_;
  }

  EntityId GetId() const { return entityId_; }

  template <typename ComponentType>
  bool HasComponent() const {
    return components_.contains(ComponentManager::GetTypeId<ComponentType>());
  }

  template <typename ComponentType>
  void AddComponent(ComponentType&& component) {
    auto handle = componentManager_->Create<ComponentType>(
        entityId_, std::forward<ComponentType>(component));
    components_.insert({ComponentManager::GetTypeId<ComponentType>(), handle});
  }

  template <typename ComponentType>
  ComponentType const* GetComponent() const {
    auto& handle = GetComponentHandle<ComponentType>();
    return componentManager_->Get<ComponentType>(handle);
  }

  template <typename ComponentType>
  void SetComponent(ComponentType&& component) {
    auto& handle = GetComponentHandle<ComponentType>();
    componentManager_->Set<ComponentType>(
        handle, std::forward<ComponentType>(component));
  }

  template <typename ComponentType>
  void RemoveComponent() {
    auto& handle = GetComponentHandle<ComponentType>();
    componentManager_->Remove(handle);
    components_.erase(ComponentManager::GetTypeId<ComponentType>());
  }

  void SetIsEnabled(bool const isEnabled) {
    isEnabled_ = isEnabled;
    componentManager_->SetEntityEnabled(entityId_, isEnabled);
  }

  bool GetIsEnabled() const { return isEnabled_; }

 protected:
  template <typename ComponentType>
  ComponentManager::Handle const& GetComponentHandle() const {
    assert(HasComponent<ComponentType>());
    return components_.at(ComponentManager::GetTypeId<ComponentType>());
  }

 private:
  inline static std::atomic<EntityId> entityIdCounter_;
  EntityId entityId_;
  bool isEnabled_;
  std::unordered_map<ComponentManager::TypeId, ComponentManager::Handle>
      components_;
  std::shared_ptr<ComponentManager> componentManager_;
};