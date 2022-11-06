
#include "manager.hpp"

#include <memory>

Manager::Manager(std::shared_ptr<ComponentManager> const& componentManager)
    : componentManager_(componentManager),
      entityCreationEvent_(
          EventManager::CreateSystemEvent<std::shared_ptr<Entity>>()),
      entityInvalidationEvent_(EventManager::CreateSystemEvent<Entity::Id>()),
      entityInvalidationObserver_(entityInvalidationEvent_->Subscribe(std::bind(
          &Manager::DestoryEntityInternal, this, std::placeholders::_1))) {
  auto componentAddEvent =
      EventManager::CreateSystemEvent<Entity::Id, ComponentManager::Handle>();
  auto componentRemoveEvent =
      EventManager::CreateSystemEvent<Entity::Id, ComponentManager::Handle>();

  componentManager_->SetSystemEvents(componentAddEvent, componentRemoveEvent);

  eventManager_ = std::make_shared<EventManager>(
      std::unordered_map<SystemEvent, EventBasePtr>{
          {SystemEvent::CreateEntity, entityCreationEvent_},
          {SystemEvent::DestroyEntity, entityInvalidationEvent_},
          {SystemEvent::AssignComponent, componentAddEvent},
          {SystemEvent::RemoveComponent, componentRemoveEvent}});
}

std::weak_ptr<Entity> Manager::CreateEntity() {
  auto entity =
      std::make_shared<Entity>(entityInvalidationEvent_, componentManager_);
  auto id = entity->GetId();
  entities_.insert({id, entity});

  entityCreationEvent_->Notify(entity);
  return entity;
}

std::weak_ptr<Entity> Manager::GetEntity(Entity::Id const entityId) {
  if (entities_.contains(entityId)) {
    return entities_.at(entityId);
  }
  return {};
}

void Manager::DestroyEntity(Entity::Id const entityId) {
  if (entities_.contains(entityId)) {
    entities_.at(entityId)->Invalidate();
    DestoryEntityInternal(entityId);
  }
}

void Manager::Tick(double const deltaTime) {
  for (auto& systemId : systemOrder_) {
    systems_.at(systemId)->Tick(deltaTime);
  }
}

void Manager::RemoveSystem(System::Id const systemId) {
  systems_.erase(systemId);
  std::erase_if(systemOrder_,
                [&](auto const& item) { return item == systemId; });
}

bool Manager::SetSystemOrder(std::vector<System::Id> const& systemsOrder) {
  for (auto systemId : systemsOrder) {
    if (!systems_.contains(systemId)) {
      return false;
    }
  }
  systemOrder_ = systemsOrder;
  return true;
}

void Manager::AddSystemInternal(System::Id const id,
                                std::unique_ptr<System>&& system) {
  if (!systems_.contains(id)) {
    system->systemId_ = id;
    systems_.insert({id, std::move(system)});
    systemOrder_.push_back(id);
  }
}

void Manager::DestoryEntityInternal(Entity::Id const& entityId) {
  entities_.erase(entityId);
}
