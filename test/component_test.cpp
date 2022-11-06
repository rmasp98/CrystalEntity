
#include "component.hpp"

#include <memory>

#include "catch2/catch_test_macros.hpp"
#include "data.hpp"

TEST_CASE("Component Manager") {
  ComponentManager manager;

  SECTION("Create returns a handle for component") {
    REQUIRE(0 == manager.Create<TestComponent>(0, {1}).Data);
  }

  SECTION("Get returns nullptr if component does not exist") {
    auto handle = manager.Create<TestComponent>(99, {1});
    REQUIRE(nullptr ==
            manager.Get<TestComponent>({typeid(TestComponent).hash_code(), 99})
                .lock());
  }

  SECTION("Get returns component from handle") {
    auto handle = manager.Create<TestComponent>(99, {1});
    REQUIRE(1 == manager.Get<TestComponent>(handle).lock()->a);
  }

  SECTION("Can add multiple of the same component") {
    auto handle = manager.Create<TestComponent>(99, {1});
    manager.Create<TestComponent>(88, {2});
    REQUIRE(1 == manager.Get<TestComponent>(handle).lock()->a);
  }

  SECTION("Can add different component types") {
    auto handle1 = manager.Create<TestComponent>(99, {1});
    auto handle2 = manager.Create<TestComponent2>(99, {2});
    REQUIRE(1 == manager.Get<TestComponent>(handle1).lock()->a);
    REQUIRE(2 == manager.Get<TestComponent2>(handle2).lock()->b);
  }

  SECTION("Can remove component") {
    auto handle = manager.Create<TestComponent>(99, {1});
    manager.Remove(handle);
    REQUIRE(nullptr == manager.Get<TestComponent>(handle).lock());
  }

  SECTION("Create for same entity removes old component") {
    auto handle = manager.Create<TestComponent>(99, {1});
    manager.Create<TestComponent>(99, {2});
    REQUIRE(nullptr == manager.Get<TestComponent>(handle).lock());
  }

  SECTION("Get component by entity returns replaced component") {
    manager.Create<TestComponent>(99, {1});
    manager.Create<TestComponent>(99, {5});
    REQUIRE(5 == manager.GetByEntity<TestComponent>(99).lock()->a);
  }

  SECTION("Can get all entitys with a component") {
    manager.Create<TestComponent>(88, {1});
    manager.Create<TestComponent>(99, {1});
    REQUIRE(std::unordered_set<ComponentManager::EntityId>{88, 99} ==
            manager.GetEntitiesWithSharedComponents<TestComponent>());
  }

  SECTION("Can get all entitys with multiple components") {
    manager.Create<TestComponent>(88, {1});
    manager.Create<TestComponent>(99, {1});
    manager.Create<TestComponent2>(88, {1});
    manager.Create<TestComponent2>(99, {1});
    REQUIRE(
        std::unordered_set<ComponentManager::EntityId>{88, 99} ==
        manager
            .GetEntitiesWithSharedComponents<TestComponent, TestComponent2>());
  }

  SECTION("Only gets entities that have all components") {
    manager.Create<TestComponent>(88, {1});
    manager.Create<TestComponent>(99, {1});
    manager.Create<TestComponent2>(88, {1});
    REQUIRE(
        std::unordered_set<ComponentManager::EntityId>{88} ==
        manager
            .GetEntitiesWithSharedComponents<TestComponent, TestComponent2>());
  }

  SECTION("Can disable an entity") {
    manager.Create<TestComponent>(88, {1});
    manager.Create<TestComponent>(99, {1});
    manager.SetEntityEnabled(99, false);
    REQUIRE(std::unordered_set<ComponentManager::EntityId>{88} ==
            manager.GetEntitiesWithSharedComponents<TestComponent>());
  }

  SECTION("Can reenable an entity") {
    manager.Create<TestComponent>(88, {1});
    manager.Create<TestComponent>(99, {1});
    manager.SetEntityEnabled(99, false);
    manager.SetEntityEnabled(99, true);
    REQUIRE(std::unordered_set<ComponentManager::EntityId>{88, 99} ==
            manager.GetEntitiesWithSharedComponents<TestComponent>());
  }

  SECTION("Disabled entities can add components directly to disabled list") {
    manager.Create<TestComponent>(88, {1});
    manager.Create<TestComponent>(99, {1}, false);
    REQUIRE(std::unordered_set<ComponentManager::EntityId>{88} ==
            manager.GetEntitiesWithSharedComponents<TestComponent>());
  }

  SECTION("Can get all entitys with multiple components even when disabled") {
    manager.Create<TestComponent>(88, {1});
    manager.Create<TestComponent>(99, {1});
    manager.Create<TestComponent2>(88, {1});
    manager.Create<TestComponent2>(99, {1});
    manager.SetEntityEnabled(99, false);
    REQUIRE(
        std::unordered_set<ComponentManager::EntityId>{88, 99} ==
        manager.GetEntitiesWithSharedComponents<TestComponent, TestComponent2>(
            false));
  }

  SECTION("Removing component also removes disabled components") {
    auto handle = manager.Create<TestComponent>(99, {1});
    manager.SetEntityEnabled(99, false);
    manager.Remove(handle);
    manager.SetEntityEnabled(99, true);
    REQUIRE(manager.GetEntitiesWithSharedComponents<TestComponent>().empty());
  }

  auto componentAddEvent =
      EventManager::CreateSystemEvent<Entity::Id, ComponentManager::Handle>();
  auto componentRemoveEvent =
      EventManager::CreateSystemEvent<Entity::Id, ComponentManager::Handle>();
  manager.SetSystemEvents(componentAddEvent, componentRemoveEvent);

  SECTION("Can subscribe to component creation events") {
    int i = 0;
    auto observer = componentAddEvent->Subscribe(
        [&](Entity::Id id, ComponentManager::Handle) { i = id; });
    manager.Create<TestComponent>(99, {1});
    observer->Unsubscribe();
    REQUIRE(99 == i);
  }

  SECTION("Can subscribe to component removal events") {
    auto handle = manager.Create<TestComponent>(99, {1});
    int i = 0;
    auto observer = componentRemoveEvent->Subscribe(
        [&](Entity::Id id, ComponentManager::Handle) { i = id; });
    manager.Remove(handle);
    observer->Unsubscribe();
    REQUIRE(99 == i);
  }

  SECTION("Can subscribe to disabled component removal events") {
    auto handle = manager.Create<TestComponent>(99, {1});
    manager.SetEntityEnabled(99, false);
    int i = 0;
    auto observer = componentRemoveEvent->Subscribe(
        [&](Entity::Id id, ComponentManager::Handle) { i = id; });
    manager.Remove(handle);
    observer->Unsubscribe();
    REQUIRE(99 == i);
  }
}