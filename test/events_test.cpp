
#include "events.hpp"

#include <memory>

#include "catch2/catch_test_macros.hpp"
#include "entity.hpp"

TEST_CASE("Events test") {
  auto systemEvent = EventManager::CreateSystemEvent<Entity::Id>();
  EventManager manager({{SystemEvent::CreateEntity, systemEvent}});

  SECTION("Can create a subject for event") {
    REQUIRE(nullptr != manager.CreateSubject<int>());
  }

  auto subject = manager.CreateSubject<int>();
  int i = 0;

  SECTION("Can subscribe to a subject") {
    auto observer = manager.Subscribe<int>([&](int const& j) { i = j; });
    subject->Notify(2);
    REQUIRE(2 == i);
    observer->Unsubscribe();
  }

  SECTION("Can create mulitple pointers to same subject") {
    auto subject2 = manager.CreateSubject<int>();
    auto observer = manager.Subscribe<int>([&](int const& j) { i = j; });
    subject->Notify(2);
    REQUIRE(2 == i);
    subject->Notify(4);
    REQUIRE(4 == i);
    observer->Unsubscribe();
  }

  SECTION("Can create multiple subjects") {
    REQUIRE(nullptr != manager.CreateSubject<double>());
    REQUIRE(nullptr != manager.CreateSubject<bool>());
  }

  double d = -2.0;
  SECTION("Can subscribe to event that hasn't been created yet") {
    auto observer = manager.Subscribe<double>([&](double const& j) { d = j; });
    auto subject2 = manager.CreateSubject<double>();
    subject2->Notify(2.0);
    REQUIRE(0.0 < d);
    observer->Unsubscribe();
  }

  SECTION("Can create a system event") {
    auto systemEvent = EventManager::CreateSystemEvent<int>();
    EventManager manager2({{SystemEvent::AssignComponent, systemEvent}});
    REQUIRE(nullptr != systemEvent);
  }

  SECTION("Can subscribe to a system event") {
    int eId = 0;
    auto observer = manager.SubscribeSystem<Entity::Id>(
        SystemEvent::CreateEntity,
        [&](Entity::Id const& entityId) { eId = entityId; });
    systemEvent->Notify(99);
    REQUIRE(99 == eId);
    observer->Unsubscribe();
  }
}