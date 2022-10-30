#pragma once

#include "system.hpp"

class TestSystem : public System {
 public:
  TestSystem(int& i)
      : InitialiseFunc([&]() { i = 1; }), TickFunc([&](int j) { i = j; }) {}

  void Initialise() override { InitialiseFunc(); }
  void Tick(double const dt) override { TickFunc(dt); }

  std::function<void()> InitialiseFunc;
  std::function<void(int)> TickFunc;
};

struct TestComponent {
  int a;
};
struct TestComponent2 {
  int b;
};