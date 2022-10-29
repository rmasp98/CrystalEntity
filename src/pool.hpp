#pragma once

#include <assert.h>
#include <stdint.h>

#include <typeinfo>
#include <unordered_set>
#include <vector>

using ElementId = uint32_t;

class Pool {
 public:
  virtual uint64_t Size() const = 0;
  virtual uint64_t Capacity() const = 0;

  template <typename ElementType>
  ElementType const* Get(ElementId const id) const {
    // TODO: move to static_assert in C++23
    assert(typeid(ElementType) == GetElementType());
    return static_cast<ElementType const*>(GetData(id));
  }

  template <typename ElementType>
  ElementId Add(ElementType&& data) {
    auto id = GetNewElementId();
    Set<ElementType>(id, std::forward<ElementType>(data));
    return id;
  }

  template <typename ElementType>
  void Set(ElementId const id, ElementType&& data) {
    assert(typeid(ElementType) == GetElementType());
    SetData(id, static_cast<void const*>(&data));
  }

  virtual void Remove(ElementId const) = 0;

 protected:
  virtual constexpr std::type_info const& GetElementType() const = 0;
  virtual void const* GetData(ElementId const) const = 0;
  virtual ElementId GetNewElementId() = 0;
  virtual void SetData(ElementId const, void const*) = 0;
};

template <typename ElementType>
class PoolImpl : public Pool {
 public:
  explicit PoolImpl(ElementId const initialSize = 0) {
    data_.reserve(initialSize);
  }

  uint64_t Size() const override { return data_.size() - freeList_.size(); }
  uint64_t Capacity() const override { return data_.capacity(); }

  ElementType const* Get(ElementId const id) const {
    if (!freeList_.contains(id) && id < data_.size()) {
      return &data_[id];
    }
    return nullptr;
  }

  ElementId Add(ElementType&& data) {
    auto id = GetNewElementId();
    Set(id, data);
    return id;
  }

  void Set(ElementId const id, ElementType&& data) {
    assert(!freeList_.contains(id) && id < data_.size());
    data_[id] = data;
  }

  void Remove(ElementId const id) override { freeList_.insert(id); }

 protected:
  constexpr std::type_info const& GetElementType() const override {
    return typeid(ElementType);
  }

  void const* GetData(ElementId const id) const override {
    return static_cast<void const*>(Get(id));
  }

  ElementId GetNewElementId() override {
    if (freeList_.empty()) {
      data_.resize(data_.size() + 1);
      return data_.size() - 1;
    }

    auto freedIter = freeList_.begin();
    auto freedIndex = *freedIter;
    freeList_.erase(freedIter);
    return freedIndex;
  }

  void SetData(ElementId const id, void const* data) override {
    auto elementData = *static_cast<ElementType const*>(data);
    Set(id, std::move(elementData));
  }

 private:
  std::vector<ElementType> data_;
  std::unordered_set<ElementId> freeList_;
};