#pragma once

#include <algorithm>
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
    std::shared_ptr<Pool> ComponentPool;
    std::unordered_map<EntityId, DataId> EntityMap;
    std::unordered_map<EntityId, DataId> DisabledEntityMap;

    template <typename ComponentType>
    std::shared_ptr<PoolImpl<ComponentType>> GetPool() {
      return std::dynamic_pointer_cast<PoolImpl<ComponentType>>(ComponentPool);
    }
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
    Mappings& mappings = GetOrCreateMappings<ComponentType>();
    auto dataId = mappings.GetPool<ComponentType>()->Add(
        std::forward<ComponentType>(data));

    if (mappings.EntityMap.contains(entityId)) {
      Remove({GetTypeId<ComponentType>(), mappings.EntityMap.at(entityId)});
    }
    mappings.EntityMap.insert({entityId, dataId});

    return {GetTypeId<ComponentType>(), dataId};
  }

  template <typename ComponentType>
  std::weak_ptr<ComponentType> Get(Handle const& handle) {
    assert(GetTypeId<ComponentType>() == handle.Type &&
           PoolExists(handle.Type));

    auto& mappings = GetMappings(handle.Type);
    return mappings.GetPool<ComponentType>()->Get(handle.Data);
  }

  template <typename ComponentType>
  std::weak_ptr<ComponentType> GetByEntity(EntityId const entityId) {
    if (PoolExists(GetTypeId<ComponentType>())) {
      Mappings& mappings = GetMappings<ComponentType>();
      if (mappings.EntityMap.contains(entityId)) {
        auto dataId = mappings.EntityMap.at(entityId);
        return mappings.GetPool<ComponentType>()->Get(dataId);
      }
    }
    // TODO: log in else that pool doesn't exist
    return {};
  }

  void Remove(Handle const& handle) {
    if (PoolExists(handle.Type)) {
      auto& mappings = componentPools_.at(handle.Type);
      mappings.ComponentPool->Remove(handle.Data);

      std::erase_if(mappings.EntityMap, [&](auto const& item) {
        return item.second == handle.Data;
      });

      std::erase_if(mappings.DisabledEntityMap, [&](auto const& item) {
        return item.second == handle.Data;
      });
    }
    // TODO: log in else that pool doesn't exist
  }

  void SetEntityEnabled(EntityId const entityId, bool const isEnabled) {
    // TODO: have to if every iteration, maybe improve
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

    std::unordered_set<EntityId> entities;
    for (auto& [entity, _] : mappings.EntityMap) {
      entities.insert(entity);
    }

    if (!ignoreDisabled) {
      for (auto& [entity, _] : mappings.DisabledEntityMap) {
        entities.insert(entity);
      }
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
          {typeId, Mappings{std::make_shared<PoolImpl<ComponentType>>(), {}}});
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