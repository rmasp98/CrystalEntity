#pragma once

#include "component.hpp"
#include "entity.hpp"
#include "system.hpp"

class Manager {
 public:
  explicit Manager(std::shared_ptr<ComponentManager> const& componentManager =
                       std::make_shared<ComponentManager>())
      : componentManager_(componentManager) {}

  std::weak_ptr<Entity> CreateEntity();
  std::weak_ptr<Entity> GetEntity(Entity::Id const);
  void DestroyEntity(Entity::Id const);

  std::shared_ptr<ComponentManager> const& GetComponentManager() const {
    return componentManager_;
  }

  // TODO: figure out how this works and if there is a simpler way
  template <typename T>
  struct identity {
    typedef T type;
  };

  template <typename... Args>
  void ForEach(
      typename identity<std::function<
          void(std::shared_ptr<Entity>, std::shared_ptr<Args>...)>>::type func,
      bool const ignoreDisabled = true) {
    static_assert(sizeof...(Args) > 0);

    auto entities = componentManager_->GetEntitiesWithSharedComponents<Args...>(
        ignoreDisabled);
    for (auto entityId : entities) {
      if (auto entity = GetEntity(entityId).lock()) {
        // TODO: can components ever be null?
        auto out = std::make_tuple(
            entity, componentManager_->GetByEntity<Args>(entityId).lock()...);
        std::apply(func, out);
      }
    }
  }

  System::Id AddSystem(std::unique_ptr<System>&& system);
  void Tick(System::Id const, double const deltaTime);
  void Tick(double const deltaTime);

  // Remove system
  // tickall
  // tick system

 private:
  std::unordered_map<Entity::Id, std::shared_ptr<Entity>> entities_;
  std::shared_ptr<ComponentManager> componentManager_;
  std::vector<std::unique_ptr<System>> systems_;
};