#include "pool.hpp"

#include <memory>

#include "catch2/catch_test_macros.hpp"

TEST_CASE("Pool Test") {
  std::unique_ptr<Pool> pool = std::make_unique<PoolImpl<int>>(10);

  SECTION("Initial size defines the reserve of the underlying vector") {
    std::unique_ptr<Pool> reservedPool = std::make_unique<PoolImpl<int>>(10);
    REQUIRE(reservedPool->Capacity() == 10);
  }

  SECTION("Can get the size of the underlying vector") {
    REQUIRE(pool->Size() == 0);
  }

  SECTION("Can add an element to the pool") {
    pool->Add<int>(1);
    REQUIRE(pool->Size() == 1);
  }

  auto id = pool->Add<int>(1);

  SECTION("Can remove an element from the pool") {
    pool->Remove(id);
    REQUIRE(pool->Size() == 0);
  }

  SECTION("Added element will assign to freed Ids first") {
    pool->Remove(id);
    REQUIRE(pool->Add<int>(1) == id);
  }

  SECTION("Can get the contents of an element") {
    REQUIRE(*pool->Get<int>(id) == 1);
  }

  SECTION("Get returns nullptr if accessing freed element") {
    pool->Remove(id);
    REQUIRE(pool->Get<int>(id) == nullptr);
  }

  SECTION("Remove element twice has no effect on vector") {
    pool->Remove(id);
    pool->Remove(id);
    REQUIRE(pool->Size() == 0);
  }
}