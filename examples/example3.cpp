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

  auto pipe = tdp::producer{get_int} >> square >> tdp::consumer{consume};
  std::this_thread::sleep_for(200ms);
}
