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

    std::this_thread::sleep_for(100ms);

    REQUIRE_EQ(consumed.size(), input_count);

    SUBCASE("In the same order") {
      for (int i = 0; i < input_count; i++) {
        REQUIRE_EQ(i, consumed[i] - 1);
      }
    }
  }
}