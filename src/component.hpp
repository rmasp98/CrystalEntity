
#include <stdint.h>

#include <algorithm>
#include <functional>
#include <memory>
#include <typeinfo>
#include <unordered_map>

#include "pool.hpp"

class Component {
 public:
  virtual ~Component() = default;

  uint64_t GetComponentId() const { return GetComponentType().hash_code(); }

  template <typename ComponentType>
  bool IsA() const {
    return typeid(ComponentType) == GetComponentType();
  }

 protected:
  virtual constexpr std::type_info const& GetComponentType() const = 0;
};

template <typename ComponentType>
class ComponentData : public Component {
 public:
  explicit ComponentData(const ComponentType& data) : data_(data) {}

  void Set(const ComponentType& data) { data_ = data; }
  const ComponentType& Get() const { return data_; }

 protected:
  constexpr std::type_info const& GetComponentType() const override {
    return typeid(ComponentType);
  }

 private:
  ComponentType data_;
};

// TODO: is there a better way to do this?
template <typename ComponentType>
inline ComponentData<ComponentType>* GetComponentData(Component* component) {
  return dynamic_cast<ComponentData<ComponentType>*>(component);
}

template <typename ComponentType>
inline uint64_t GetComponentTypeId() {
  return typeid(ComponentType).hash_code();
}

using ComponentHandle = uint64_t;

class ComponentManager {
  struct ComponentMappings {
    std::unique_ptr<Pool> ComponentPool;
    std::unordered_map<uint32_t, ComponentHandle> EntityMap;
  };

 public:
  template <typename ComponentType>
  ComponentHandle Create(ComponentType&& data, const uint32_t entityId) {
    auto& mappings = GetComponentMappings<ComponentType>();
    auto handle =
        mappings.ComponentPool->Add(std::forward<ComponentType>(data));

    if (mappings.EntityMap.contains(entityId)) {
      Remove<ComponentType>(mappings.EntityMap.at(entityId));
    }
    mappings.EntityMap.insert({entityId, handle});

    return handle;
  }

  template <typename ComponentType>
  ComponentType* Get(ComponentHandle const handle) {
    ComponentMappings& mappings = GetComponentMappings<ComponentType>();
    return mappings.ComponentPool->Get<ComponentType>(handle);
  }

  template <typename ComponentType>
  ComponentType* GetByEntity(uint32_t const entityId) {
    ComponentMappings& mappings = GetComponentMappings<ComponentType>();
    if (mappings.EntityMap.contains(entityId)) {
      auto handle = mappings.EntityMap.at(entityId);
      return mappings.ComponentPool->Get<ComponentType>(handle);
    }
    return nullptr;
  }

  template <typename ComponentType>
  void Remove(ComponentHandle const handle) {
    ComponentMappings& mappings = GetComponentMappings<ComponentType>();
    mappings.ComponentPool->Remove(handle);
    mappings.EntityMap.erase(handle);
  }

  // TODO: figure out how this works and if there is a simpler way
  template <typename T>
  struct identity {
    typedef T type;
  };

  template <typename... Args>
  void ForEach(typename identity<std::function<void(Args*...)>>::type func) {
    static_assert(sizeof...(Args) > 0);
    for (auto entityId : GetEntitiesWithSharedComponents<Args...>()) {
      auto out = std::make_tuple(GetByEntity<Args>(entityId)...);
      std::apply(func, out);
    }
  }

 protected:
  template <typename... Args>
  std::unordered_set<uint64_t> GetEntitiesWithSharedComponents() {
    return SetIntersection(GetComponentEntities<Args>()...);
  }

  // TODO: probably a more efficient way of doing this
  template <typename ComponentType>
  std::unordered_set<uint64_t> GetComponentEntities() {
    ComponentMappings& mappings = GetComponentMappings<ComponentType>();

    std::unordered_set<uint64_t> entities;
    for (auto& [entity, _] : mappings.EntityMap) {
      entities.insert(entity);
    }

    return entities;
  }

  template <typename... Sets>
  std::unordered_set<uint64_t> SetIntersection(
      std::unordered_set<uint64_t> set1, Sets... sets) {
    if constexpr (sizeof...(Sets) == 0) {
      return set1;
    } else {
      auto set2 = SetIntersection(sets...);

      std::unordered_set<uint64_t> intersection;
      std::set_intersection(set1.begin(), set1.end(), set2.begin(), set2.end(),
                            std::inserter(intersection, intersection.end()));
      return intersection;
    }
  }

  template <typename ComponentType>
  ComponentMappings& GetComponentMappings() {
    auto typeId = GetComponentTypeId<ComponentType>();
    if (!componentPools_.contains(typeId)) {
      // TODO: can this ever fail?
      componentPools_.insert(
          {typeId,
           ComponentMappings{std::make_unique<PoolImpl<ComponentType>>(), {}}});
    }

    return componentPools_.at(typeId);
  }

 private:
  std::unordered_map<uint64_t, ComponentMappings> componentPools_;
};