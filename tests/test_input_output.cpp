// The Darkest Pipeline - https://github.com/JoelFilho/TDP
// test_input_output.cpp - Test suite for pipelines with manual input and polled output

// Copyright Joel P. C. Filho 2020 - 2020
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE.md or copy at https://www.boost.org/LICENSE_1_0.txt)

#include "doctest/doctest.h"
#include "tdp/pipeline.hpp"

TEST_CASE("Basic Input and Output") {
  constexpr auto square = [](auto x) { return x * x; };
  auto pipeline = tdp::input<int> >> square >> tdp::output;

  SUBCASE("Empty pipeline should yield no output on try_get()") {  //
    auto res = pipeline.try_get();
    REQUIRE(!res.has_value());
  }

  SUBCASE("Single input yields a single output") {
    pipeline.input(5);
    REQUIRE(pipeline.wait_get() == 25);
    REQUIRE_FALSE(pipeline.try_get());
  }

  SUBCASE("Multiple inputs yield the same amount of outputs, in the correct order.") {
    constexpr int input_count = 10;
    for (int i = 0; i < input_count; i++) {
      pipeline.input(i);
    }
    for (int i = 0; i < input_count; i++) {
      int res = pipeline.wait_get();
      REQUIRE_EQ(res, square(i));
    }
    auto res = pipeline.try_get();
    REQUIRE(!res.has_value());
  }
}