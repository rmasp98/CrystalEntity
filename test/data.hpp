#pragma once

#include <iostream>

#include "entity.hpp"
#include "events.hpp"
#include "system.hpp"

enum class SystemState {
  None,
  Initialised,
  Ticked,
  EventTriggered,
};

enum class EventState {
  None,
  EntityCreated,
  EntityDestroyed,
  ComponentAssigned,
  ComponentRemoved,
  Custom,
};

class TestSystem : public System {
 public:
  TestSystem(std::shared_ptr<EventManager> eventManager, SystemState* state,
             EventState* event, std::vector<Id>* executionOrder)
      : state_(state), event_(event), executionOrder_(executionOrder) {
    if (state_) *state_ = SystemState::Initialised;
    if (event_) {
      createEntityObserver_ = eventManager->SubscribeSystem<Entity>(
          SystemEvent::CreateEntity,
          [&](Entity const&) { *event_ = EventState::EntityCreated; });
      destroyEntityObserver_ = eventManager->SubscribeSystem<Entity::Id>(
          SystemEvent::DestroyEntity,
          [&](Entity::Id const&) { *event_ = EventState::EntityDestroyed; });
      addComponentObserver_ =
          eventManager->SubscribeSystem<ComponentManager::EntityId,
                                        ComponentManager::Handle>(
              SystemEvent::AssignComponent,
              [&](ComponentManager::EntityId const&,
                  ComponentManager::Handle const&) {
                *event_ = EventState::ComponentAssigned;
              });
      removeComponentObserver_ =
          eventManager->SubscribeSystem<ComponentManager::EntityId,
                                        ComponentManager::Handle>(
              SystemEvent::RemoveComponent,
              [&](ComponentManager::EntityId const&,
                  ComponentManager::Handle const&) {
                *event_ = EventState::ComponentRemoved;
              });
    }
  }

  ~TestSystem() {
    if (createEntityObserver_) createEntityObserver_->Unsubscribe();
    if (destroyEntityObserver_) destroyEntityObserver_->Unsubscribe();
    if (addComponentObserver_) addComponentObserver_->Unsubscribe();
    if (removeComponentObserver_) removeComponentObserver_->Unsubscribe();
  }

  void Tick(double const dt) override {
    if (state_) *state_ = SystemState::Ticked;
    if (executionOrder_) executionOrder_->push_back(GetId());
  }

  SystemState* state_ = nullptr;
  EventState* event_ = nullptr;
  std::vector<Id>* executionOrder_ = nullptr;

  ObserverPtr createEntityObserver_;
  ObserverPtr destroyEntityObserver_;
  ObserverPtr addComponentObserver_;
  ObserverPtr removeComponentObserver_;
};

class TestSystem2 : public System {
 public:
  TestSystem2(std::shared_ptr<EventManager>, SystemState* state,
              std::vector<Id>* executionOrder)
      : state_(state), executionOrder_(executionOrder) {
    if (state_) *state_ = SystemState::Initialised;
  }
  void Tick(double const dt) override {
    if (state_) *state_ = SystemState::Ticked;
    if (executionOrder_) executionOrder_->push_back(GetId());
  }
  SystemState* state_ = nullptr;
  std::vector<Id>* executionOrder_ = nullptr;
};

struct TestComponent {
  int a;
};
struct TestComponent2 {
  int b;
};