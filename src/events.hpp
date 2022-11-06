#pragma once

#include <memory>
#include <unordered_map>

#include "AsyncLib/observer.hpp"

enum class SystemEvent {
  CreateEntity,
  DestroyEntity,
  AssignComponent,
  RemoveComponent,
};

template <typename... EventArgs>
using Event = async_lib::Subject<EventArgs...>;
template <typename... EventArgs>
using EventPtr = std::shared_ptr<Event<EventArgs...>>;
using EventBasePtr = std::shared_ptr<async_lib::SubjectBase>;
using ObserverPtr = std::shared_ptr<async_lib::ObserverBase>;

class EventManager {
 public:
  explicit EventManager(
      std::unordered_map<SystemEvent, EventBasePtr> const& systemEvents)
      : systemEvents_(systemEvents) {}

  template <typename... EventType>
  static EventPtr<EventType...> CreateSystemEvent() {
    return std::make_shared<Event<EventType...>>();
  }

  template <typename EventType>
  EventPtr<EventType> CreateSubject() {
    auto eventId = GetEventId<EventType>();
    if (events_.contains(eventId)) {
      return std::static_pointer_cast<Event<EventType>>(events_.at(eventId));
    }

    auto subject = std::make_shared<Event<EventType>>();
    events_.insert({eventId, subject});
    return subject;
  }

  template <typename EventType>
  ObserverPtr Subscribe(std::function<void(EventType const&)> callback) {
    if (auto subject = CreateSubject<EventType>()) {
      return subject->Subscribe(callback);
    }
    return nullptr;
  }

  // TODO: figure out how this works and if there is a simpler way
  template <typename T>
  struct identity {
    typedef T type;
  };

  template <typename... EventArgs>
  ObserverPtr SubscribeSystem(
      SystemEvent const eventName,
      typename identity<std::function<void(EventArgs const&...)>>::type
          callback) {
    if (systemEvents_.contains(eventName)) {
      if (EventPtr<EventArgs...> event =
              std::static_pointer_cast<Event<EventArgs...>>(
                  systemEvents_.at(eventName))) {
        return event->Subscribe(callback);
      }
    }
    return nullptr;
  }

 protected:
  using EventId = uint64_t;

  template <typename EventType>
  EventId GetEventId() {
    return typeid(EventType).hash_code();
  }

 private:
  std::unordered_map<EventId, EventBasePtr> events_;
  std::unordered_map<SystemEvent, EventBasePtr> systemEvents_;
};