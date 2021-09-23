// The Darkest Pipeline - https://github.com/JoelFilho/TDP
// test_consumers.cpp - Test suite for pipelines with consumer threads

// Copyright Joel P. C. Filho 2020 - 2020
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE.md or copy at https://www.boost.org/LICENSE_1_0.txt)

#include <vector>

#include "doctest/doctest.h"
#include "tdp/pipeline.hpp"

using namespace std::chrono_literals;

TEST_CASE("Consumers") {
  std::vector<int> consumed;
  auto sink = [&](int v) { consumed.push_back(v); };
  auto increment = [](auto x) { return x + 1; };

  auto pipeline = tdp::input<int> >> increment >> tdp::consumer{sink};

  REQUIRE(consumed.empty());

  SUBCASE("Must process the same amount of input given") {
    constexpr int input_count = 10;
    for (int i = 0; i < input_count; i++) {
      pipeline.input(i);
    }

    pipeline.wait_until_idle();
    REQUIRE(pipeline.idle());

    REQUIRE_EQ(consumed.size(), input_count);

    SUBCASE("In the same order") {
      for (int i = 0; i < input_count; i++) {
        REQUIRE_EQ(i, consumed[i] - 1);
      }
    }
  }
}

TEST_CASE("Single-thread Consumers") {
  std::vector<int> consumed;
  auto sum_consume = [&](auto a, auto b) { consumed.push_back(a + b); };

  auto pipeline = tdp::input<int, int> >> tdp::consumer{sum_consume} / tdp::policy::queue / tdp::as_unique_ptr;

  REQUIRE(consumed.empty());

  SUBCASE("Must process the same amount of input given") {
    constexpr int input_count = 10;
    for (int i = 0; i < input_count; i++) {
      pipeline->input(i, i);
    }

    pipeline->wait_until_idle();
    REQUIRE(pipeline->idle());

    REQUIRE_EQ(consumed.size(), input_count);

    SUBCASE("In the same order") {
      for (int i = 0; i < input_count; i++) {
        REQUIRE_EQ(i + i, consumed[i]);
      }
    }
  }
}