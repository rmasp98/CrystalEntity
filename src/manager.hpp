#pragma once

#include "component.hpp"
#include "entity.hpp"
#include "events.hpp"
#include "system.hpp"

class Manager {
 public:
  explicit Manager(std::shared_ptr<ComponentManager> const& componentManager =
                       std::make_shared<ComponentManager>());

  ~Manager() {
    if (entityInvalidationObserver_) entityInvalidationObserver_->Unsubscribe();
  }

  std::weak_ptr<Entity> CreateEntity();
  std::weak_ptr<Entity> GetEntity(Entity::Id const);
  void DestroyEntity(Entity::Id const);

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

  template <typename SystemClass, typename... Args>
  System::Id AddSystem(Args... args) {
    auto id = System::GetId<SystemClass>();
    AddSystemInternal(id,
                      std::make_unique<SystemClass>(eventManager_, args...));
    return id;
  }

  template <typename SystemClass>
  void RemoveSystem() {
    auto systemId = System::GetId<SystemClass>();
    RemoveSystem(systemId);
  }

  void RemoveSystem(System::Id const systemId);

  // Returns true if successful
  bool SetSystemOrder(std::vector<System::Id> const& systemsOrder);

  std::vector<System::Id> const& GetSystemOrder() const {
    return systemOrder_;
  };

  // Only ticks template system
  template <typename SystemClass>
  void Tick(double const deltaTime) {
    auto systemId = System::GetId<SystemClass>();
    if (systems_.contains(systemId)) {
      systems_.at(systemId)->Tick(deltaTime);
    }
  }

  // Ticks all systems in system order
  void Tick(double const deltaTime);

 protected:
  void AddSystemInternal(System::Id const, std::unique_ptr<System>&&);
  void DestoryEntityInternal(Entity::Id const&);

 private:
  std::unordered_map<Entity::Id, std::shared_ptr<Entity>> entities_;
  std::unordered_map<System::Id, std::unique_ptr<System>> systems_;
  std::vector<System::Id> systemOrder_;

  std::shared_ptr<ComponentManager> componentManager_;
  std::shared_ptr<EventManager> eventManager_;

  EventPtr<std::shared_ptr<Entity>> entityCreationEvent_;
  EventPtr<Entity::Id> entityInvalidationEvent_;

  ObserverPtr entityInvalidationObserver_;
};