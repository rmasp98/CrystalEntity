#pragma once

#include <assert.h>
#include <stdint.h>

#include <memory>
#include <typeinfo>
#include <unordered_map>
#include <unordered_set>
#include <vector>

using ElementId = uint32_t;

class Pool {
 public:
  virtual uint64_t Size() const = 0;
  virtual uint64_t Capacity() const = 0;

  virtual void Remove(ElementId const) = 0;
};

template <typename ElementType>
class PoolImpl : public Pool {
 public:
  explicit PoolImpl(ElementId const initialSize = 0) {
    data_.reserve(initialSize);
  }

  uint64_t Size() const override {
    return data_.size() - freeList_.size() - garbageList_.size();
  }
  uint64_t Capacity() const override { return data_.capacity(); }

  std::weak_ptr<ElementType> Get(ElementId const id) {
    if (!freeList_.contains(id) && !garbageList_.contains(id) &&
        id < data_.size()) {
      accessors_.insert(
          {id, std::shared_ptr<ElementType>(&data_[id], [&, id](ElementType*) {
             garbageList_.erase(id);
             freeList_.insert(id);
           })});
      return accessors_.at(id);
    }
    return {};
  }

  ElementId Add(ElementType&& data) {
    auto id = GetNewElementId();
    data_[id] = data;
    return id;
  }

  void Remove(ElementId const id) override {
    if (accessors_.contains(id)) {
      garbageList_.insert(id);
      accessors_.erase(id);
    } else if (!garbageList_.contains(id)) {
      freeList_.insert(id);
    }
  }

 protected:
  ElementId GetNewElementId() {
    if (freeList_.empty()) {
      data_.resize(data_.size() + 1);
      return data_.size() - 1;
    }

    auto freedIter = freeList_.begin();
    auto freedIndex = *freedIter;
    freeList_.erase(freedIter);
    return freedIndex;
  }

 private:
  std::vector<ElementType> data_;
  std::unordered_set<ElementId> garbageList_;
  std::unordered_set<ElementId> freeList_;
  std::unordered_map<ElementId, std::shared_ptr<ElementType>> accessors_;
};