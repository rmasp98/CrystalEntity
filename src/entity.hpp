#pragma once

#include <atomic>
#include <unordered_set>

#include "AsyncLib/observer.hpp"
#include "component.hpp"

class Entity {
 public:
  using Id = ComponentManager::EntityId;

  explicit Entity(
      std::shared_ptr<async_lib::Subject<Entity::Id>> invalidationSubject,
      std::shared_ptr<ComponentManager> const& componentManager)
      : entityId_(entityIdCounter_++),
        componentManager_(componentManager),
        invalidationSubject_(invalidationSubject) {}

  // Can't be copied (it will break isValid and isEnabled)
  Entity(Entity const&) = delete;
  Entity& operator=(Entity const&) = delete;

  auto operator<=>(Entity const& rhs) const {
    return entityId_ <=> rhs.entityId_;
  }

  Id GetId() const { return entityId_; }

  template <typename ComponentType>
  bool HasComponent() const {
    return components_.contains(ComponentManager::GetTypeId<ComponentType>());
  }

  template <typename ComponentType>
  std::weak_ptr<ComponentType> GetComponent() const {
    auto& handle = GetComponentHandle<ComponentType>();
    return componentManager_->Get<ComponentType>(handle);
  }

  template <typename ComponentType>
  void AddComponent(ComponentType&& component) {
    if (IsValid()) {
      auto handle = componentManager_->Create<ComponentType>(
          entityId_, std::forward<ComponentType>(component), isEnabled_);
      components_.insert_or_assign(ComponentManager::GetTypeId<ComponentType>(),
                                   handle);
    }
  }

  template <typename ComponentType>
  void RemoveComponent() {
    auto& handle = GetComponentHandle<ComponentType>();
    componentManager_->Remove(handle);
    components_.erase(ComponentManager::GetTypeId<ComponentType>());
  }

  void RemoveAllComponents() {
    for (auto const& [_, handle] : components_) {
      componentManager_->Remove(handle);
    }
    components_.clear();
  }

  void SetIsEnabled(bool const isEnabled) {
    isEnabled_ = isEnabled;
    componentManager_->SetEntityEnabled(entityId_, isEnabled);
  }

  bool GetIsEnabled() const { return isEnabled_; }

  void Invalidate() {
    isValid_ = false;
    RemoveAllComponents();
    if (invalidationSubject_) invalidationSubject_->Notify(entityId_);
  }

  bool IsValid() const { return isValid_; }

 protected:
  template <typename ComponentType>
  ComponentManager::Handle const& GetComponentHandle() const {
    assert(HasComponent<ComponentType>());
    return components_.at(ComponentManager::GetTypeId<ComponentType>());
  }

 private:
  Id const entityId_;
  std::atomic<bool> isEnabled_ = true;
  std::atomic<bool> isValid_ = true;
  // TODO: replace with thread safe map
  std::unordered_map<ComponentManager::TypeId, ComponentManager::Handle>
      components_;

  inline static std::atomic<Entity::Id> entityIdCounter_ = 0;
  std::shared_ptr<ComponentManager> componentManager_;
  std::shared_ptr<async_lib::Subject<Entity::Id>> invalidationSubject_;
};