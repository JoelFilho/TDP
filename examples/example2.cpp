#include <iostream>

#include "tdp/pipeline.hpp"

int main() {
  constexpr auto get_int = []() -> int { return -1; };
  constexpr auto add = [](auto x, auto y) { return x + y; };
  constexpr auto square = [](auto x) -> decltype(x * x) { return x * x; };

  auto pipe = tdp::input<int, int> >> add >> square >> tdp::output;

  for (int i = 0; i < 10; i++) {
    for (int j = 0; j < 10; j++) {
      pipe.input(i, j);
    }
  }

  for (int i = 0; i < 100; i++) {
    std::cout << i << ": " <<  pipe.get() << "\n";
  }
}
