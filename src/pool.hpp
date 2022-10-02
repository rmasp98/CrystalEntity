#pragma once

#include <assert.h>
#include <stdint.h>

#include <typeinfo>
#include <unordered_set>
#include <vector>

class Pool {
 public:
  virtual uint64_t Size() const = 0;
  virtual uint64_t Capacity() const = 0;

  template <typename ElementType>
  uint32_t Add(ElementType&& data) {
    // TODO: move to static_assert in C++23
    assert(typeid(ElementType) == GetElementType());
    return AddData(static_cast<void const*>(&data));
  }

  virtual void Remove(const uint32_t id) = 0;

  template <typename ElementType>
  ElementType* Get(uint32_t const id) {
    // TODO: move to static_assert in C++23
    assert(typeid(ElementType) == GetElementType());
    return static_cast<ElementType*>(GetData(id));
  }

 protected:
  virtual constexpr std::type_info const& GetElementType() const = 0;
  virtual uint32_t AddData(void const* data) = 0;
  virtual void* GetData(uint32_t const id) = 0;
};

template <typename ElementType>
class PoolImpl : public Pool {
 public:
  explicit PoolImpl(uint32_t const initialSize = 0) {
    if (initialSize > 0) {
      data_.reserve(initialSize);
    }
  }

  uint32_t Add(ElementType&& data) {
    if (freeList_.empty()) {
      data_.push_back(data);
      return data_.size() - 1;
    }

    auto freedIter = freeList_.begin();
    auto freedIndex = *freedIter;
    freeList_.erase(freedIter);
    data_[freedIndex] = data;
    return freedIndex;
  }

  void Remove(uint32_t const id) override { freeList_.insert(id); }

  ElementType* Get(uint32_t const id) {
    if (!freeList_.contains(id)) {
      return &data_[id];
    }
    return nullptr;
  }

  uint64_t Size() const override { return data_.size() - freeList_.size(); }
  uint64_t Capacity() const override { return data_.capacity(); }

 protected:
  constexpr std::type_info const& GetElementType() const override {
    return typeid(ElementType);
  }

  uint32_t AddData(void const* data) override {
    auto elementData = *static_cast<ElementType const*>(data);
    return Add(std::move(elementData));
  }

  void* GetData(const uint32_t id) override {
    return static_cast<void*>(Get(id));
  }

 private:
  std::vector<ElementType> data_;
  std::unordered_set<uint32_t> freeList_;
};