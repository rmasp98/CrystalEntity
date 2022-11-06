
#include "manager.hpp"

#include <cstdlib>

#include "catch2/catch_test_macros.hpp"
#include "data.hpp"

TEST_CASE("Manager tests") {
  Manager manager;

  SECTION("Manager entity tests") {
    SECTION("Can get entity") {
      auto entity = manager.CreateEntity();
      REQUIRE(manager.GetEntity(entity.lock()->GetId()).lock() ==
              entity.lock());
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

    SECTION("Invalidating entity removes from manager") {
      auto entity = manager.CreateEntity();
      auto id = entity.lock()->GetId();
      entity.lock()->Invalidate();
      REQUIRE(manager.GetEntity(id).expired());
    }
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

  SECTION("Manager system tests") {
    SECTION("Add a system returns SystemId") {
      auto id = manager.AddSystem<TestSystem>(nullptr, nullptr, nullptr);
      REQUIRE(System::GetId<TestSystem>() == id);
    }

    SECTION("Can tick a particular system") {
      SystemState state = SystemState::None;
      manager.AddSystem<TestSystem>(&state, nullptr, nullptr);
      REQUIRE(SystemState::Initialised == state);
      manager.Tick<TestSystem>(60.0f);
      REQUIRE(SystemState::Ticked == state);
    }

    std::vector<System::Id> executionOrder;
    SystemState state1 = SystemState::None;
    auto id = manager.AddSystem<TestSystem>(&state1, nullptr, &executionOrder);

    SECTION("Can remove system from manager") {
      manager.RemoveSystem<TestSystem>();
      manager.Tick<TestSystem>(60.0f);
      REQUIRE(SystemState::Initialised == state1);
    }

    SECTION("Can remove system from manager by id") {
      manager.RemoveSystem(id);
      manager.Tick<TestSystem>(60.0f);
      REQUIRE(SystemState::Initialised == state1);
    }

    SystemState state2 = SystemState::None;
    auto id2 = manager.AddSystem<TestSystem2>(&state2, &executionOrder);

    SECTION("Can tick over all systems") {
      manager.Tick(60.0f);
      REQUIRE(SystemState::Ticked == state1);
      REQUIRE(SystemState::Ticked == state2);
    }

    SECTION("Can define the order in which the systems are executed") {
      std::vector<System::Id> order{id2, id};
      manager.SetSystemOrder(order);
      manager.Tick(60.0f);
      REQUIRE(order == executionOrder);
    }

    SECTION("Returns false and does nothing if order contains invalid id") {
      std::vector<System::Id> order{id, 99};
      REQUIRE(false == manager.SetSystemOrder(order));
      REQUIRE(manager.GetSystemOrder() != order);
    }

    SECTION("Removing system removes from order list") {
      manager.RemoveSystem(id);
      REQUIRE(std::vector<System::Id>{id2} == manager.GetSystemOrder());
    }

    SECTION("Adding same system multiple times doesn't add multiple to order") {
      std::vector<System::Id> order{id, id2};
      manager.SetSystemOrder(order);
      manager.AddSystem<TestSystem>(nullptr, nullptr, nullptr);
      manager.Tick(60.0f);
      REQUIRE(order == executionOrder);
    }
  }

  SECTION("Manager Event tests") {
    EventState eventState = EventState::None;
    manager.AddSystem<TestSystem>(nullptr, &eventState, nullptr);

    SECTION("System can subscribe to entity creation event") {
      manager.CreateEntity();
      REQUIRE(EventState::EntityCreated == eventState);
    }

    SECTION("System can subscribe to entity destruction event") {
      auto entity = manager.CreateEntity();
      manager.DestroyEntity(entity.lock()->GetId());
      REQUIRE(EventState::EntityDestroyed == eventState);
    }

    SECTION("System can subscribe to component add event") {
      auto entity = manager.CreateEntity();
      if (auto e = entity.lock()) {
        e->AddComponent<TestComponent>({1});
      }
      REQUIRE(EventState::ComponentAssigned == eventState);
    }

    SECTION("System can subscribe to component add event") {
      auto entity = manager.CreateEntity();
      if (auto e = entity.lock()) {
        e->AddComponent<TestComponent>({1});
        e->RemoveComponent<TestComponent>();
      }
      REQUIRE(EventState::ComponentRemoved == eventState);
    }
  }
}
