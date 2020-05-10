#include <iostream>

#include "tdp/pipeline.hpp"

int main() {
  using namespace std::chrono_literals;

  constexpr auto get_int = []() -> int {
    std::this_thread::sleep_for(10ms);
    return -1;
  };

  constexpr auto add = [](auto x, auto y) { return x + y; };
  constexpr auto square = [](auto x) -> decltype(x * x) { return x * x; };

  constexpr auto consume = [](auto x) {
    struct Counter {
      int count = 0;
      ~Counter() { std::cout << "Calls to consume(): " << count << '\n'; }
    };
    thread_local Counter c;
    c.count++;
  };

  // When the processing is faster than the production, it's usually a good idea to use a blocking policy.
  // And, as a queue will not be bigger than one, we can use a buffer instead.
  // Experiment changing it to a queue and notice it won't make a difference in this case.
  auto pipe = tdp::producer{get_int}(tdp::policy::triple_buffer) >> square >> tdp::consumer{consume};
  std::this_thread::sleep_for(200ms);
}
