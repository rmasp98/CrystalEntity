#pragma once

#include <algorithm>
#include <memory>
#include <typeinfo>
#include <unordered_map>
#include <utility>

#include "AsyncLib/pool.hpp"
#include "events.hpp"

class ComponentManager {
  using DataId = int64_t;

 public:
  // Has to be uint to be compatible with hash_code
  using TypeId = uint64_t;
  using EntityId = int32_t;

  struct Handle {
    TypeId Type;
    DataId Data;
  };

 protected:
  class EntityMappings {
   public:
    void Set(EntityId const entityId, DataId const dataId, bool isEnabled) {
      if (isEnabled) {
        enabledEntityMap_.insert({entityId, dataId});
      } else {
        disabledEntityMap_.insert({entityId, dataId});
      }
    }

    DataId GetData(EntityId const entityId,
                   bool const ignoreDisabled = true) const {
      if (Contains(entityId)) {
        return enabledEntityMap_.at(entityId);
      } else if (!ignoreDisabled && Contains(entityId, false)) {
        return disabledEntityMap_.at(entityId);
      }
      // TODO: maybe throw excption
      return {~0};
    }

    EntityId GetEntity(DataId const dataId,
                       bool const ignoreDisabled = true) const {
      if (auto const entityIdPtr = GetEntity(dataId, enabledEntityMap_)) {
        return *entityIdPtr;
      } else if (!ignoreDisabled) {
        if (auto const entityIdPtr = GetEntity(dataId, disabledEntityMap_)) {
          return *entityIdPtr;
        }
      }
      // TODO: maybe throw excption
      return {~0};
    }

    std::unordered_set<EntityId> GetAllEntities(
        bool const ignoreDisabled = true) const {
      std::unordered_set<EntityId> entities;
      for (auto const& [entityId, _] : enabledEntityMap_) {
        entities.insert(entityId);
      }

      if (!ignoreDisabled) {
        for (auto& [entityId, _] : disabledEntityMap_) {
          entities.insert(entityId);
        }
      }

      return entities;
    }

    bool Contains(EntityId const entityId, bool ignoreDisabled = true) const {
      return enabledEntityMap_.contains(entityId) ||
             (!ignoreDisabled && disabledEntityMap_.contains(entityId));
    }

    void SetIsEnabled(EntityId const entityId, bool isEnabled) {
      if (Contains(entityId, false)) {
        auto dataId = GetData(entityId, false);
        Remove(entityId);
        Set(entityId, dataId, isEnabled);
      }
    }

    void Remove(EntityId const entityId) {
      enabledEntityMap_.erase(entityId);
      disabledEntityMap_.erase(entityId);
    }

    void Remove(DataId const dataId) {
      std::erase_if(enabledEntityMap_,
                    [&](auto const& item) { return item.second == dataId; });
      std::erase_if(disabledEntityMap_,
                    [&](auto const& item) { return item.second == dataId; });
    }

   protected:
    EntityId const* GetEntity(
        DataId const dataId,
        std::unordered_map<EntityId, DataId> const& map) const {
      auto it = std::find_if(map.begin(), map.end(), [&](auto const& item) {
        return item.second == dataId;
      });
      if (it != map.end()) {
        return &it->first;
      }
      return nullptr;
    }

   private:
    std::unordered_map<EntityId, DataId> enabledEntityMap_;
    std::unordered_map<EntityId, DataId> disabledEntityMap_;
  };

  struct Mappings {
    std::shared_ptr<async_lib::PoolBase> ComponentPool;
    EntityMappings EntityMap;

    template <typename ComponentType>
    std::shared_ptr<async_lib::Pool<ComponentType>> GetPool() {
      return std::static_pointer_cast<async_lib::Pool<ComponentType>>(
          ComponentPool);
    }
  };

 public:
  void SetSystemEvents(
      EventPtr<EntityId, ComponentManager::Handle> const& addEvent,
      EventPtr<EntityId, ComponentManager::Handle> const& removeEvent) {
    addEvent_ = addEvent;
    removeEvent_ = removeEvent;
  }

  template <typename ComponentType>
  inline static TypeId GetTypeId() {
    return typeid(ComponentType).hash_code();
  }

  bool PoolExists(TypeId typeId) const {
    return componentPools_.contains(typeId);
  }

  template <typename ComponentType>
  Handle Create(EntityId const entityId, ComponentType&& data,
                bool const isEnabled = true) {
    Mappings& mappings = GetOrCreateMappings<ComponentType>();
    auto dataId = mappings.GetPool<ComponentType>()->Add(
        std::forward<ComponentType>(data));

    // TODO: need to check DisabledEntityMap
    if (mappings.EntityMap.Contains(entityId, false)) {
      Remove({GetTypeId<ComponentType>(),
              mappings.EntityMap.GetData(entityId, false)});
    }

    mappings.EntityMap.Set(entityId, dataId, isEnabled);

    Handle handle{GetTypeId<ComponentType>(), dataId};
    if (addEvent_) addEvent_->Notify(entityId, handle);

    return handle;
  }

  template <typename ComponentType>
  std::weak_ptr<ComponentType> Get(Handle const& handle) {
    assert(GetTypeId<ComponentType>() == handle.Type &&
           PoolExists(handle.Type));

    auto& mappings = GetMappings(handle.Type);
    return mappings.GetPool<ComponentType>()->Get(handle.Data);
  }

  template <typename ComponentType>
  std::weak_ptr<ComponentType> GetByEntity(EntityId const entityId,
                                           bool const ignoreDisabled = true) {
    if (PoolExists(GetTypeId<ComponentType>())) {
      Mappings& mappings = GetMappings<ComponentType>();
      if (mappings.EntityMap.Contains(entityId, ignoreDisabled)) {
        auto dataId = mappings.EntityMap.GetData(entityId, ignoreDisabled);
        return mappings.GetPool<ComponentType>()->Get(dataId);
      }
    }
    // TODO: log in else that pool doesn't exist
    return {};
  }

  void Remove(Handle const& handle) {
    if (PoolExists(handle.Type)) {
      auto& mappings = componentPools_.at(handle.Type);
      auto entityId = mappings.EntityMap.GetEntity(handle.Data, false);

      mappings.ComponentPool->Remove(handle.Data);
      mappings.EntityMap.Remove(handle.Data);

      if (removeEvent_) removeEvent_->Notify(entityId, handle);
    }
    // TODO: log in else that pool doesn't exist
  }

  void SetEntityEnabled(EntityId const entityId, bool const isEnabled) {
    // TODO: have to if every iteration, maybe improve
    for (auto& [_, pool] : componentPools_) {
      pool.EntityMap.SetIsEnabled(entityId, isEnabled);
    }
  }

  template <typename... Args>
  std::unordered_set<EntityId> GetEntitiesWithSharedComponents(
      bool const ignoreDisabled = true) const {
    return SetIntersection(GetComponentEntities<Args>(ignoreDisabled)...);
  }

 protected:
  // TODO: probably a more efficient way of doing this
  template <typename ComponentType>
  std::unordered_set<EntityId> GetComponentEntities(
      bool const ignoreDisabled) const {
    auto const& mappings = GetMappings<ComponentType>();
    return mappings.EntityMap.GetAllEntities(ignoreDisabled);
  }

  void RemoveEntity(std::unordered_map<EntityId, DataId>& entityMap,
                    Handle const& handle) {
    auto it = std::find_if(
        entityMap.begin(), entityMap.end(),
        [&](auto const& item) { return item.second == handle.Data; });
    if (it != entityMap.end()) {
      if (removeEvent_) removeEvent_->Notify(it->first, handle);
      entityMap.erase(it);
    }
  }

  template <typename... Sets>
  std::unordered_set<EntityId> SetIntersection(
      std::unordered_set<EntityId> const& set1, Sets const&... sets) const {
    if constexpr (sizeof...(Sets) == 0) {
      return set1;
    } else {
      auto set2 = SetIntersection(sets...);

      std::unordered_set<EntityId> intersection;
      std::set_intersection(set1.begin(), set1.end(), set2.begin(), set2.end(),
                            std::inserter(intersection, intersection.end()));
      return intersection;
    }
  }

  template <typename ComponentType>
  Mappings& GetOrCreateMappings() {
    auto typeId = GetTypeId<ComponentType>();
    if (!componentPools_.contains(typeId)) {
      // TODO: can this ever fail?
      componentPools_.insert(
          {typeId,
           Mappings{std::make_shared<async_lib::Pool<ComponentType>>(), {}}});
    }

    return GetMappings(typeId);
  }

  template <typename ComponentType>
  Mappings& GetMappings() {
    return const_cast<Mappings&>(
        std::as_const(*this).GetMappings<ComponentType>());
  }

  template <typename ComponentType>
  Mappings const& GetMappings() const {
    auto typeId = GetTypeId<ComponentType>();
    return GetMappings(typeId);
  }

  Mappings& GetMappings(TypeId const typeId) {
    return const_cast<Mappings&>(std::as_const(*this).GetMappings(typeId));
  }

  Mappings const& GetMappings(TypeId const typeId) const {
    return componentPools_.at(typeId);
  }

 private:
  std::unordered_map<TypeId, Mappings> componentPools_;

  EventPtr<EntityId, Handle> addEvent_;
  EventPtr<EntityId, Handle> removeEvent_;
};