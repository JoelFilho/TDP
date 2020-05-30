// The Darkest Pipeline - https://github.com/JoelFilho/TDP
// test_exceptions.cpp - Test suite for exception safety on pipeline construction

// Copyright Joel P. C. Filho 2020 - 2020
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE.md or copy at https://www.boost.org/LICENSE_1_0.txt)

#include "doctest/doctest.h"
#include "tdp/pipeline.hpp"

struct thrower {
  int count = 0;
  thrower() = default;
  thrower(int x) : count(x) {}
  thrower(thrower&& t) {
    if (t.count == 0)
      throw std::runtime_error("!");
    count = t.count - 1;
  }

  void operator()(float) {}
  int operator()(int) { return 0; }
  int operator()() { return 0; };
};

TEST_CASE("Exceptions") {
  SUBCASE("Pipeline stage") {
    REQUIRE_THROWS_AS(auto pipeline = tdp::input<int> >> thrower{} >> tdp::output, std::runtime_error);
  }

  SUBCASE("Deep pipeline stage") {
    REQUIRE_THROWS_AS(auto pipeline = tdp::input<int> >> thrower{2} >> thrower{2} >> thrower{10} >> tdp::output,  //
        std::runtime_error);
  }

  SUBCASE("Producer") {
    REQUIRE_THROWS_AS(auto pipeline = tdp::producer{thrower{1}} >> tdp::output, std::runtime_error);
  }

  SUBCASE("Consumer") {
    auto make_int = []() { return 0; };
    auto to_float = [](int) { return 0.0f; };
    REQUIRE_THROWS_AS(auto pipeline = tdp::producer{make_int} >> to_float >> tdp::consumer{thrower{1}},  //
        std::runtime_error);
  }

  SUBCASE("Producer to Consumer") {
    auto empty_consumer = [](auto) {};
    REQUIRE_THROWS_AS(auto pipeline = tdp::producer{thrower{2}} >> tdp::consumer{empty_consumer}, std::runtime_error);
  }
}

// Commpile-time tests
namespace static_tests {
// clang-format off
void consume(int) {}
int  produce() { return 0; }
int  passthrough(int) { return 0; }
// clang-format on

// Construction until output must be noexcept
// Even if the functions aren't, since what matters is their construction.
static_assert(noexcept(tdp::input<int> >> passthrough));
static_assert(noexcept(tdp::producer{produce} >> passthrough >> passthrough));

// Construction on output is not noexcept, as std::thread's constructor throws.
static_assert(!noexcept(tdp::producer{produce} >> passthrough >> tdp::consumer{consume}));
static_assert(!noexcept(tdp::producer{produce} >> tdp::consumer{consume}));

}  // namespace static_tests