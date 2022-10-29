#include "pool.hpp"

#include <memory>

#include "catch2/catch_test_macros.hpp"

TEST_CASE("Pool Test") {
  std::unique_ptr<Pool> pool = std::make_unique<PoolImpl<int>>(10);

  SECTION("Initial size defines the reserve of the underlying vector") {
    std::unique_ptr<Pool> reservedPool = std::make_unique<PoolImpl<int>>(10);
    REQUIRE(10 == reservedPool->Capacity());
  }

  SECTION("Can get the size of the underlying vector") {
    REQUIRE(0 == pool->Size());
  }

  SECTION("Can add an element to the pool") {
    pool->Add<int>(1);
    REQUIRE(1 == pool->Size());
  }

  auto id = pool->Add<int>(1);

  SECTION("Can remove an element from the pool") {
    pool->Remove(id);
    REQUIRE(0 == pool->Size());
  }

  SECTION("Added element will assign to freed Ids first") {
    pool->Remove(id);
    REQUIRE(id == pool->Add<int>(1));
  }

  SECTION("Can get the contents of an element") {
    REQUIRE(1 == *pool->Get<int>(id));
  }

  SECTION("Can replace the data of an id") {
    pool->Set<int>(id, 5);
    REQUIRE(5 == *pool->Get<int>(id));
  }

  SECTION("Get returns nullptr if accessing freed element") {
    pool->Remove(id);
    REQUIRE(nullptr == pool->Get<int>(id));
  }

  SECTION("Remove element twice has no effect on vector") {
    pool->Remove(id);
    pool->Remove(id);
    REQUIRE(0 == pool->Size());
  }

  SECTION("Get returns nullptr for invalid id") {
    REQUIRE(nullptr == pool->Get<int>(5));
  }

  SECTION("Can get element from const pool") {
    std::unique_ptr<Pool const> constPool = std::move(pool);
    REQUIRE(1 == *constPool->Get<int>(id));
  }
}