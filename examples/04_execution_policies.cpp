#include <iostream>

#include "tdp/pipeline.hpp"

//---------------------------------------------------------------------------------------------------------------------
// This example shows how to use execution policies with TDP
//
// An execution policy defines the internal data structure utilized for thread communication in a pipeline.
//---------------------------------------------------------------------------------------------------------------------

int main() {
  constexpr auto add = [](auto x, auto y) { return x + y; };
  constexpr auto square = [](auto x) { return x * x; };

  // Queue (default) stores everything it can
  auto pipe = tdp::input<int, int>(tdp::policy::queue) >> add >> square >> tdp::output;

  // Triple buffering only has capacity of one
  auto pipe2 = tdp::input<int, int>(tdp::policy::triple_buffer) >> add >> square >> tdp::output;

  // Provide both pipelines with 25 input values
  for (int i = 0; i < 5; i++) {
    for (int j = 0; j < 5; j++) {
      pipe.input(i, j);
      pipe2.input(i, j);
    }
  }

  // Evaluate the first pipeline's results
  // It will complete exactly 25 times.
  std::cout << "pipe's result: \n";
  for (int i = 0; i < 25; i++) {
    std::cout << i << ": " << pipe.wait_get() << "\n";
  }
  std::cout << "pipe.empty(): " << std::boolalpha << pipe.empty() << "\n";

  // Evaluate the second pipeline's results
  // This will only complete once.
  std::cout << "-----\n";
  std::cout << "pipe2's result: " << pipe2.wait_get() << "\n";
  std::cout << "pipe2.empty(): " << std::boolalpha << pipe2.empty() << "\n";
}
