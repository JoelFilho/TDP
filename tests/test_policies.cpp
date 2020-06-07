// The Darkest Pipeline - https://github.com/JoelFilho/TDP
// test_policies.cpp - Test suite for pipelines with execution policies

// Copyright Joel P. C. Filho 2020 - 2020
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE.md or copy at https://www.boost.org/LICENSE_1_0.txt)

#include <math.h>

#include "doctest/doctest.h"
#include "tdp/pipeline.hpp"

using namespace std::chrono_literals;

constexpr int input_count = 100'000;

double proc(double a, double b) noexcept {
  return (a * b) / (a + b + 1.0);
}

TEST_CASE("Blocking Queue policy") {
  auto pipeline = tdp::input<double, double> >> proc >> tdp::output / tdp::policy::queue;

  for (int i = 0; i < input_count; i++) {
    pipeline.input(i, i + 2);
  }

  for (int i = 0; i < input_count; i++) {
    [[maybe_unused]] auto res = pipeline.wait_get();
  }

  auto res = pipeline.try_get();
  REQUIRE(!res.has_value());
}

TEST_CASE("Blocking Triple Buffer policy") {
  auto pipeline = tdp::input<double, double> >> proc >> tdp::output / tdp::policy::triple_buffer;

  for (int i = 0; i < input_count; i++) {
    pipeline.input(i, i + 2);
  }

  pipeline.wait_until_idle();
  REQUIRE(pipeline.idle());

  int processed = 0;

  while (true) {
    auto res = pipeline.try_get();
    if (res) {
      processed++;
    } else {
      break;
    }
  }

  REQUIRE_LE(processed, input_count);

  auto res = pipeline.try_get();
  REQUIRE(!res.has_value());
}

TEST_CASE("Lock-free Triple Buffer policy") {
  auto pipeline = tdp::input<double, double> >> proc >> tdp::output / tdp::policy::triple_buffer_lockfree;

  for (int i = 0; i < input_count; i++) {
    pipeline.input(i, i + 2);
  }

  pipeline.wait_until_idle();
  REQUIRE(pipeline.idle());

  int processed = 0;

  while (true) {
    auto res = pipeline.try_get();
    if (res) {
      processed++;
    } else {
      break;
    }
  }

  REQUIRE_LE(processed, input_count);

  auto res = pipeline.try_get();
  REQUIRE(!res.has_value());
}