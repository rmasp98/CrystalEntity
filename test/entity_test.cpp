
#include "entity.hpp"

#include "catch2/catch_test_macros.hpp"
#include "component.hpp"
#include "data.hpp"

TEST_CASE("Entity Tests") {
  auto componentManager = std::make_shared<ComponentManager>();
  Entity entity(componentManager);

  SECTION("Get entity id") { REQUIRE(0 == entity.GetId()); }

  SECTION("Entity id iterates for each entity") {
    Entity entity2(componentManager);
    // This does start at 0 because entity has been constructed already in
    // previous test
    REQUIRE(1 == entity.GetId());
    REQUIRE(2 == entity2.GetId());
  }

  SECTION("Returns false for invalid component") {
    REQUIRE(false == entity.HasComponent<TestComponent>());
  }

  SECTION("Can add a component to entity") {
    entity.AddComponent<TestComponent>({1});
    REQUIRE(1 == entity.GetComponent<TestComponent>().lock()->a);
  }

  SECTION("Can replace a component to entity") {
    entity.AddComponent<TestComponent>({1});
    entity.AddComponent<TestComponent>({5});
    REQUIRE(5 == entity.GetComponent<TestComponent>().lock()->a);
  }

  SECTION("Has component return true if does have component") {
    entity.AddComponent<TestComponent>({1});
    REQUIRE(true == entity.HasComponent<TestComponent>());
  }

  SECTION("Can remove a component from an entity") {
    entity.AddComponent<TestComponent>({1});
    entity.RemoveComponent<TestComponent>();
    REQUIRE(false == entity.HasComponent<TestComponent>());
  }

  SECTION("Can remove all components from an entity") {
    entity.AddComponent<TestComponent>({1});
    entity.AddComponent<TestComponent2>({2});
    entity.RemoveAllComponents();
    REQUIRE(false == entity.HasComponent<TestComponent>());
    REQUIRE(false == entity.HasComponent<TestComponent2>());
  }

  SECTION("Disabling entity disables in component manager") {
    entity.AddComponent<TestComponent>({1});
    entity.SetIsEnabled(false);
    REQUIRE(entity.GetIsEnabled() == false);
    REQUIRE(componentManager->GetEntitiesWithSharedComponents<TestComponent>()
                .empty());
  }

  SECTION("Disabling entity disables in component manager") {
    entity.AddComponent<TestComponent>({1});
    entity.SetIsEnabled(false);
    entity.SetIsEnabled(true);
    REQUIRE(entity.GetIsEnabled() == true);
    REQUIRE(std::unordered_set<ComponentManager::EntityId>{entity.GetId()} ==
            componentManager->GetEntitiesWithSharedComponents<TestComponent>());
  }

  SECTION("Invalidating entity removes all components") {
    entity.AddComponent<TestComponent>({1});
    entity.AddComponent<TestComponent2>({2});
    entity.Invalidate();
    REQUIRE(false == entity.HasComponent<TestComponent>());
    REQUIRE(false == entity.HasComponent<TestComponent2>());
  }

  SECTION("Invalidating entity prevents adding more components") {
    entity.Invalidate();
    entity.AddComponent<TestComponent>({1});
    REQUIRE(false == entity.HasComponent<TestComponent>());
  }
}