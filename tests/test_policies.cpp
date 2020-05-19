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

  std::this_thread::sleep_for(100ms);

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

  std::this_thread::sleep_for(100ms);

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