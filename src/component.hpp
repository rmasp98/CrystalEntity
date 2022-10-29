#pragma once

#include <stdint.h>

#include <algorithm>
#include <functional>
#include <memory>
#include <typeinfo>
#include <unordered_map>

#include "pool.hpp"

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

  struct Mappings {
    std::unique_ptr<Pool> ComponentPool;
    std::unordered_map<EntityId, DataId> EntityMap;
    std::unordered_map<EntityId, DataId> DisabledEntityMap;
  };

  template <typename ComponentType>
  inline static TypeId GetTypeId() {
    return typeid(ComponentType).hash_code();
  }

  bool PoolExists(TypeId typeId) const {
    return componentPools_.contains(typeId);
  }

  template <typename ComponentType>
  Handle Create(EntityId const entityId, ComponentType&& data) {
    auto& mappings = GetOrCreateMappings<ComponentType>();
    auto dataId =
        mappings.ComponentPool->Add(std::forward<ComponentType>(data));

    if (mappings.EntityMap.contains(entityId)) {
      Remove({GetTypeId<ComponentType>(), mappings.EntityMap.at(entityId)});
    }
    mappings.EntityMap.insert({entityId, dataId});

    return {GetTypeId<ComponentType>(), dataId};
  }

  template <typename ComponentType>
  ComponentType const* Get(Handle const& handle) const {
    assert(GetTypeId<ComponentType>() == handle.Type &&
           PoolExists(handle.Type));

    auto const& mappings = GetMappings(handle.Type);
    return mappings.ComponentPool->Get<ComponentType>(handle.Data);
  }

  template <typename ComponentType>
  void Set(Handle const handle, ComponentType&& data) {
    assert(GetTypeId<ComponentType>() == handle.Type &&
           PoolExists(handle.Type));

    auto& mappings = GetMappings(handle.Type);
    mappings.ComponentPool->Set<ComponentType>(
        handle.Data, std::forward<ComponentType>(data));
  }

  void Remove(Handle const& handle) {
    if (PoolExists(handle.Type)) {
      auto& mappings = componentPools_.at(handle.Type);
      mappings.ComponentPool->Remove(handle.Data);
      std::erase_if(mappings.EntityMap, [&](auto const& item) {
        return item.second == handle.Data;
      });
    }
    // TODO: log in else that pool doesn't exist
  }

  void SetEntityEnabled(EntityId const entityId, bool const isEnabled) {
    for (auto& [_, pool] : componentPools_) {
      if (isEnabled && pool.DisabledEntityMap.contains(entityId)) {
        auto const dataId = pool.DisabledEntityMap[entityId];
        pool.DisabledEntityMap.erase(entityId);
        pool.EntityMap.insert({entityId, dataId});
      } else if (!isEnabled && pool.EntityMap.contains(entityId)) {
        auto const dataId = pool.EntityMap[entityId];
        pool.EntityMap.erase(entityId);
        pool.DisabledEntityMap.insert({entityId, dataId});
      }
    }
  }

  template <typename... Args>
  std::unordered_set<EntityId> GetEntitiesWithSharedComponents() {
    return SetIntersection(GetComponentEntities<Args>()...);
  }

  // TODO: figure out how this works and if there is a simpler way
  template <typename T>
  struct identity {
    typedef T type;
  };

  template <typename... Args>
  void ForEach(
      typename identity<
          std::function<void(EntityId const, Args const*...)>>::type func) {
    static_assert(sizeof...(Args) > 0);
    for (auto entityId : GetEntitiesWithSharedComponents<Args...>()) {
      auto out = std::make_tuple(entityId, GetByEntity<Args>(entityId)...);
      std::apply(func, out);
    }
  }

 protected:
  template <typename ComponentType>
  ComponentType const* GetByEntity(EntityId const entityId) const {
    if (PoolExists(GetTypeId<ComponentType>())) {
      Mappings const& mappings = GetMappings<ComponentType>();
      if (mappings.EntityMap.contains(entityId)) {
        auto dataId = mappings.EntityMap.at(entityId);
        return mappings.ComponentPool->Get<ComponentType>(dataId);
      }
    }
    // TODO: log in else that pool doesn't exist
    return nullptr;
  }

  // TODO: probably a more efficient way of doing this
  template <typename ComponentType>
  std::unordered_set<EntityId> GetComponentEntities() const {
    auto const& mappings = GetMappings<ComponentType>();

    std::unordered_set<EntityId> entities;
    for (auto& [entity, _] : mappings.EntityMap) {
      entities.insert(entity);
    }

    return entities;
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
          {typeId, Mappings{std::make_unique<PoolImpl<ComponentType>>(), {}}});
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
};