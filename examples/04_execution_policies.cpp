#include <iostream>

#include "tdp/pipeline.hpp"

//---------------------------------------------------------------------------------------------------------------------
// This example shows how to use execution policies with TDP
//
// An execution policy defines the internal data structure utilized for thread communication in a pipeline.
//
// To use an execution policy, you can define them with an '/' after the output.
//
// The currently supported policies are:
//  - tdp::policy::queue - Uses a blocking queue to store the values between threads
//  - tdp::policy::triple_buffer - Uses a blocking triple buffer to store values
//
// This tutorial shows the difference between these policies and how to use them in a pipeline.
//---------------------------------------------------------------------------------------------------------------------

int main() {
  constexpr auto add = [](auto x, auto y) { return x + y; };
  constexpr auto square = [](auto x) { return x * x; };

  // Queue (default) stores everything it can
  auto pipe = tdp::input<int, int> >> add >> square >> tdp::output / tdp::policy::queue;

  // Triple buffering only has capacity of one
  auto pipe2 = tdp::input<int, int> >> add >> square >> tdp::output / tdp::policy::triple_buffer;

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
