
#include "manager.hpp"

#include "catch2/catch_test_macros.hpp"
#include "data.hpp"

TEST_CASE("Manager tests") {
  Manager manager;

  SECTION("Can get entity") {
    auto entity = manager.CreateEntity();
    REQUIRE(manager.GetEntity(entity.lock()->GetId()).lock() == entity.lock());
  }

  SECTION("Can destroy an entity") {
    auto id = manager.CreateEntity().lock()->GetId();
    manager.DestroyEntity(id);
    REQUIRE(manager.GetEntity(id).expired());
  }

  SECTION("Destroying an entity removes all components") {
    auto entity = manager.CreateEntity().lock();
    entity->AddComponent<TestComponent>({1});
    manager.DestroyEntity(entity->GetId());
    REQUIRE(!entity->HasComponent<TestComponent>());
  }

  SECTION("Manager foreach tests") {
    auto weakEntity1 = manager.CreateEntity();
    if (auto entity = weakEntity1.lock()) {
      entity->AddComponent<TestComponent>({1});
      entity->AddComponent<TestComponent2>({2});
    }

    auto weakEntity2 = manager.CreateEntity();
    if (auto entity = weakEntity2.lock()) {
      entity->AddComponent<TestComponent>({1});
    }

    SECTION("Can run a function over multiple components of a type") {
      auto i = 0;
      manager.ForEach<TestComponent>(
          [&](std::shared_ptr<Entity> entity, std::shared_ptr<TestComponent>) {
            REQUIRE(std::unordered_set<int>{weakEntity1.lock()->GetId(),
                                            weakEntity2.lock()->GetId()}
                        .contains(entity->GetId()));
            ++i;
          });
      REQUIRE(2 == i);
    }

    SECTION("Only runs over entities that contain all components") {
      auto i = 0;
      manager.ForEach<TestComponent, TestComponent2>(
          [&](std::shared_ptr<Entity> entity, std::shared_ptr<TestComponent>,
              std::shared_ptr<TestComponent2>) {
            REQUIRE(weakEntity1.lock() == entity);
            ++i;
          });
      REQUIRE(1 == i);
    }

    SECTION("Runs over entities even if disabled when ignoreDisabled is true") {
      if (auto entity = weakEntity1.lock()) {
        entity->SetIsEnabled(false);
      }

      auto i = 0;
      manager.ForEach<TestComponent, TestComponent2>(
          [&](std::shared_ptr<Entity> entity, std::shared_ptr<TestComponent>,
              std::shared_ptr<TestComponent2>) {
            REQUIRE(weakEntity1.lock() == entity);
            ++i;
          },
          false);
      REQUIRE(1 == i);
    }
  }

  // TODO: plan the system stuff out a bit more before writing...
  SECTION("Can add a system and initialise is called") {
    int i = 0;
    auto system = std::make_unique<TestSystem>(i);
    manager.AddSystem(std::move(system));
    REQUIRE(1 == i);
  }

  SECTION("Can tick over a system") {
    int i = 0;
    auto system = std::make_unique<TestSystem>(i);
    auto id = manager.AddSystem(std::move(system));
    manager.Tick(id, 2.0f);
    REQUIRE(2 == i);
  }

  SECTION("Can tick over all systems") {
    int i = 0;
    int j = 0;
    auto system1 = std::make_unique<TestSystem>(i);
    auto system2 = std::make_unique<TestSystem>(j);
    manager.AddSystem(std::move(system1));
    manager.AddSystem(std::move(system2));
    manager.Tick(2.0f);
    REQUIRE(2 == i);
    REQUIRE(2 == j);
  }
}
