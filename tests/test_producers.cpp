// The Darkest Pipeline - https://github.com/JoelFilho/TDP
// test_producers.cpp - Test suite for pipelines with producer threads

// Copyright Joel P. C. Filho 2020 - 2020
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE.md or copy at https://www.boost.org/LICENSE_1_0.txt)

#include "doctest/doctest.h"
#include "tdp/pipeline.hpp"

using namespace std::chrono_literals;

TEST_CASE("Producers") {
  std::atomic_int produced = 0;
  auto source = [&] { return produced++; };
  auto pipeline = tdp::producer{source} >> tdp::output;

  SUBCASE("pipeline must be producing after construction") {  //
    REQUIRE(pipeline.producing());
  }

  SUBCASE("pause() actually pauses production, and producing() returns false") {
    std::this_thread::sleep_for(50ms);
    REQUIRE_FALSE(pipeline.idle());
    pipeline.pause();
    REQUIRE_FALSE(pipeline.producing());
    REQUIRE_NE(produced, 0);

    std::this_thread::sleep_for(10ms);
    int old_produced = produced;
    std::this_thread::sleep_for(10ms);
    REQUIRE_EQ(old_produced, produced);

    SUBCASE("resume() actually resumes production, and producing() returns true") {
      pipeline.resume();
      REQUIRE(pipeline.producing());

      std::this_thread::sleep_for(10ms);

      REQUIRE_NE(old_produced, produced);
      REQUIRE_FALSE(pipeline.idle());
    }
  }

  SUBCASE("All items produced are processed, and in the right order") {
    std::this_thread::sleep_for(10ms);
    while (produced < 10)
      /* Wait until there's at least 10 produced items in the list */;
    REQUIRE_FALSE(pipeline.idle());
    pipeline.pause();
    std::this_thread::sleep_for(10ms);

    REQUIRE_NE(produced, 0);

    int consumed = 0;
    while (true) {
      auto res = pipeline.try_get();
      if (res) {
        REQUIRE_EQ(*res, consumed);
        consumed++;
      } else {
        break;
      }
    }

    REQUIRE_EQ(produced, consumed);
    REQUIRE(pipeline.idle());
  }
}