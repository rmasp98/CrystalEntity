
#include "manager.hpp"

std::weak_ptr<Entity> Manager::CreateEntity() {
  auto entity = std::make_shared<Entity>(componentManager_);
  auto id = entity->GetId();
  entities_.insert({id, std::move(entity)});
  return GetEntity(id);
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
    entities_.erase(entityId);
  }
}

System::Id Manager::AddSystem(std::unique_ptr<System>&& system) {
  system->Initialise();
  auto id = system->GetId();
  systems_.push_back(std::move(system));
  return id;
}

void Manager::Tick(System::Id const systemId, double const deltaTime) {
  auto systemIt = std::find_if(
      systems_.begin(), systems_.end(),
      [&](auto const& element) { return element->GetId() == systemId; });
  if (systemIt != systems_.end()) {
    (*systemIt)->Tick(deltaTime);
  }
}

void Manager::Tick(double const deltaTime) {
  for (auto& system : systems_) {
    system->Tick(deltaTime);
  }
}