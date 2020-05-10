#include <iostream>

#include "tdp/pipeline.hpp"

int main() {
  constexpr auto get_int = []() -> int { return -1; };
  constexpr auto add = [](auto x, auto y) { return x + y; };
  constexpr auto square = [](auto x) -> decltype(x * x) { return x * x; };

  // Queue (default) stores everything it can
  auto pipe = tdp::input<int, int>(tdp::policy::queue) >> add >> square >> tdp::output;

  // Triple buffering only has capacity of one
  auto pipe2 = tdp::input<int, int>(tdp::policy::triple_buffer) >> add >> square >> tdp::output;

  // Provide both pipelines with 100 input values
  for (int i = 0; i < 10; i++) {
    for (int j = 0; j < 10; j++) {
      pipe.input(i, j);
      pipe2.input(i, j);
    }
  }

  // Evaluate the first pipeline's results
  // It will complete exactly 100 times.
  std::cout << "pipe's result: \n";
  for (int i = 0; i < 100; i++) {
    std::cout << i << ": " << pipe.get() << "\n";
  }
  std::cout << "pipe.empty(): " << std::boolalpha << !pipe.available() << "\n";

  // Evaluate the second pipeline's results
  // This will only complete once.
  std::cout << "-----\n";
  std::cout << "pipe2's result: " << pipe2.get() << "\n";
  std::cout << "pipe2.empty(): " << std::boolalpha << !pipe2.available() << "\n";
}
