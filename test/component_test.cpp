
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

TEST_CASE("Component Tests") {
  std::unique_ptr<Component> component1 =
      std::make_unique<ComponentData<TestComponent>>(TestComponent{1});
  std::unique_ptr<Component> component2 =
      std::make_unique<ComponentData<TestComponent2>>(TestComponent2{2});

  SECTION("IsA returns if component is of a particular type") {
    REQUIRE(component1->IsA<TestComponent>());
    REQUIRE(!component1->IsA<TestComponent2>());
  }

  SECTION("GetComponentId returns unique idenifier") {
    std::unique_ptr<Component> component3 =
        std::make_unique<ComponentData<TestComponent>>(TestComponent{3});
    REQUIRE(component1->GetComponentId() != component2->GetComponentId());
    REQUIRE(component1->GetComponentId() == component3->GetComponentId());
  }

  SECTION("GetComponentData returns underlying type") {
    REQUIRE(GetComponentData<TestComponent>(component1.get())->Get().a == 1);
  }

  SECTION("GetComponentData returns nullptr for invalid type") {
    REQUIRE(GetComponentData<TestComponent2>(component1.get()) == nullptr);
  }
}

TEST_CASE("Component Manager") {
  // tests
  //  - serialise and deserialise?

  ComponentManager manager;

  SECTION("Get returns nullptr if component does not exist") {
    REQUIRE(manager.Get<TestComponent>(0) == nullptr);
  }

  SECTION("Create returns a handle for component") {
    REQUIRE(manager.Create<TestComponent>({1}, 0) == 0);
  }

  SECTION("Get returns component from handle") {
    auto handle = manager.Create<TestComponent>({1}, 0);
    REQUIRE(manager.Get<TestComponent>(handle)->a == 1);
  }

  SECTION("Can add multiple of the same component") {
    auto handle = manager.Create<TestComponent>({1}, 0);
    manager.Create<TestComponent>({2}, 1);
    REQUIRE(manager.Get<TestComponent>(handle)->a == 1);
  }

  SECTION("Can add different component types") {
    auto handle1 = manager.Create<TestComponent>({1}, 0);
    auto handle2 = manager.Create<TestComponent2>({2}, 0);
    REQUIRE(manager.Get<TestComponent>(handle1)->a == 1);
    REQUIRE(manager.Get<TestComponent2>(handle2)->b == 2);
  }

  SECTION("Can get component for particular entity") {
    manager.Create<TestComponent>({1}, 0);
    REQUIRE(manager.GetByEntity<TestComponent>(0)->a == 1);
  }

  SECTION("Get component by entity returns nullptr if doesn't exist") {
    REQUIRE(manager.GetByEntity<TestComponent>(0) == nullptr);
  }

  SECTION("Can remove component") {
    auto handle = manager.Create<TestComponent>({1}, 0);
    manager.Remove<TestComponent>(handle);
    REQUIRE(manager.Get<TestComponent>(handle) == nullptr);
  }

  SECTION("Create for same entity removes old entity") {
    auto handle = manager.Create<TestComponent>({1}, 0);
    manager.Create<TestComponent>({2}, 0);
    REQUIRE(manager.Get<TestComponent>(handle) == nullptr);
  }

  SECTION("Can run a function over a components of a type") {
    manager.Create<TestComponent>({1}, 0);
    manager.ForEach<TestComponent>(
        [&](TestComponent* component) { REQUIRE(component->a == 1); });
  }

  SECTION("Can run a function over multiple components of a type") {
    manager.Create<TestComponent>({1}, 0);
    manager.Create<TestComponent>({2}, 1);
    auto i = 1;
    manager.ForEach<TestComponent>([&](TestComponent*) { i++; });
    REQUIRE(i == 3);
  }

  SECTION("Can run a function over multiple components types") {
    manager.Create<TestComponent>({1}, 0);
    manager.Create<TestComponent2>({1}, 0);
    auto i = 1;
    manager.ForEach<TestComponent, TestComponent2>(
        [&](TestComponent*, TestComponent2*) { i++; });
    REQUIRE(i == 2);
  }

  SECTION("Only runs over entities that contain all components") {
    manager.Create<TestComponent>({1}, 0);
    manager.Create<TestComponent2>({1}, 1);
    auto i = 1;
    manager.ForEach<TestComponent, TestComponent2>(
        [&](TestComponent*, TestComponent2*) { i++; });
    REQUIRE(i == 1);
  }
}