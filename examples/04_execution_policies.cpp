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
//  - tdp::policy::triple_buffer - Uses a blocking triple-buffer to store values
//  - tdp::policy::triple_buffer_lockfree - Uses a lock-free triple-buffer
//
// This tutorial shows the difference between these policies and how to use them in a pipeline.
//---------------------------------------------------------------------------------------------------------------------

int main() {
  constexpr auto add = [](auto x, auto y) { return x + y; };
  constexpr auto square = [](auto x) { return x * x; };

  // Queue (default) stores everything it can
  auto pipe_q = tdp::input<int, int> >> add >> square >> tdp::output / tdp::policy::queue;

  // Triple buffering only has capacity of one
  auto pipe_tb = tdp::input<int, int> >> add >> square >> tdp::output / tdp::policy::triple_buffer;

  // TDP supports lock-free triple buffers. We can use them here.
  auto pipe_tb_lf = tdp::input<int, int> >> add >> square >> tdp::output / tdp::policy::triple_buffer_lockfree;

  // Provide all pipelines with the same 25 input values
  for (int i = 0; i < 5; i++) {
    for (int j = 0; j < 5; j++) {
      pipe_q.input(i, j);
      pipe_tb.input(i, j);
      pipe_tb_lf.input(i, j);
    }
  }

  // Evaluate the first pipeline's results
  // It will complete exactly 25 times.
  std::cout << "pipe_q's result: \n";
  for (int i = 0; i < 25; i++) {
    std::cout << i << ": " << pipe_q.wait_get() << "\n";
  }
  std::cout << "pipe_q.empty(): " << std::boolalpha << pipe_q.empty() << "\n";

  // Evaluate the second pipeline's results
  // This will only complete once.
  std::cout << "-----\n";
  std::cout << "pipe_tb's result: " << pipe_tb.wait_get() << "\n";
  std::cout << "pipe_tb.empty(): " << std::boolalpha << pipe_tb.empty() << "\n";

  // Evaluate the lock-free pipeline's results
  // The behavior should be the same as the blocking triple buffering.
  // For details on the difference, check the "Lock-free Policies" example.
  std::cout << "-----\n";
  std::cout << "pipe_tb_lf's result: " << pipe_tb_lf.wait_get() << "\n";
  std::cout << "pipe_tb_lf.empty(): " << std::boolalpha << pipe_tb_lf.empty() << "\n";
}
