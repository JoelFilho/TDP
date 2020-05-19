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
    }
  }

  SUBCASE("All items produced are processed, and in the right order") {
    std::this_thread::sleep_for(10ms);
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
  }
}