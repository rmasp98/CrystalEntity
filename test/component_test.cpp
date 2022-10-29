
#include "component.hpp"

#include <memory>

#include "catch2/catch_test_macros.hpp"

// Tests
// - Register entity to component type

struct TestComponent {
  int a;
};
struct TestComponent2 {
  int b;
};

TEST_CASE("Component Manager") {
  ComponentManager manager;

  SECTION("Create returns a handle for component") {
    REQUIRE(0 == manager.Create<TestComponent>(0, {1}).Data);
  }

  SECTION("Get returns nullptr if component does not exist") {
    auto handle = manager.Create<TestComponent>(99, {1});
    REQUIRE(nullptr == manager.Get<TestComponent>(
                           {typeid(TestComponent).hash_code(), 99}));
  }

  SECTION("Get returns component from handle") {
    auto handle = manager.Create<TestComponent>(99, {1});
    REQUIRE(1 == manager.Get<TestComponent>(handle)->a);
  }

  SECTION("Can add multiple of the same component") {
    auto handle = manager.Create<TestComponent>(99, {1});
    manager.Create<TestComponent>(88, {2});
    REQUIRE(1 == manager.Get<TestComponent>(handle)->a);
  }

  SECTION("Can add different component types") {
    auto handle1 = manager.Create<TestComponent>(99, {1});
    auto handle2 = manager.Create<TestComponent2>(99, {2});
    REQUIRE(1 == manager.Get<TestComponent>(handle1)->a);
    REQUIRE(2 == manager.Get<TestComponent2>(handle2)->b);
  }

  SECTION("Can modify component by handle") {
    auto handle = manager.Create<TestComponent>(99, {1});
    manager.Set<TestComponent>(handle, {5});
    REQUIRE(5 == manager.Get<TestComponent>(handle)->a);
  }

  SECTION("Can remove component") {
    auto handle = manager.Create<TestComponent>(99, {1});
    manager.Remove(handle);
    REQUIRE(nullptr == manager.Get<TestComponent>(handle));
  }

  SECTION("Create for same entity removes old component") {
    auto handle = manager.Create<TestComponent>(99, {1});
    manager.Create<TestComponent>(99, {2});
    REQUIRE(nullptr == manager.Get<TestComponent>(handle));
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

  SECTION("Can run a function over multiple components of a type") {
    manager.Create<TestComponent>(88, {1});
    manager.Create<TestComponent>(99, {2});
    auto i = 0;
    manager.ForEach<TestComponent>(
        [&](ComponentManager::EntityId const entityId, TestComponent const*) {
          REQUIRE(std::vector<int>{88, 99}[i++] == entityId);
        });
  }

  SECTION("Only runs over entities that contain all components") {
    manager.Create<TestComponent>(88, {1});
    manager.Create<TestComponent>(99, {1});
    manager.Create<TestComponent2>(88, {1});
    manager.ForEach<TestComponent, TestComponent2>(
        [&](ComponentManager::EntityId const entityId, TestComponent const*,
            TestComponent2 const*) { REQUIRE(88 == entityId); });
  }

  SECTION("Get component by entity returns replaced component") {
    manager.Create<TestComponent>(99, {1});
    manager.Create<TestComponent>(99, {5});
    manager.ForEach<TestComponent>(
        [&](ComponentManager::EntityId const, TestComponent const* data) {
          REQUIRE(5 == data->a);
        });
  }

  SECTION("Can disable an entity") {
    manager.Create<TestComponent>(88, {1});
    manager.Create<TestComponent>(99, {1});
    manager.SetEntityEnabled(99, false);
    REQUIRE(std::unordered_set<ComponentManager::EntityId>{88} ==
            manager.GetEntitiesWithSharedComponents<TestComponent>());
  }

  SECTION("Can disable an entity") {
    manager.Create<TestComponent>(88, {1});
    manager.Create<TestComponent>(99, {1});
    manager.SetEntityEnabled(99, false);
    manager.SetEntityEnabled(99, true);
    REQUIRE(std::unordered_set<ComponentManager::EntityId>{88, 99} ==
            manager.GetEntitiesWithSharedComponents<TestComponent>());
  }
}