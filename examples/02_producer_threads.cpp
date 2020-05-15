#include <iostream>

#include "tdp/pipeline.hpp"

//---------------------------------------------------------------------------------------------------------------------
// This example will show how to use producer threads
//
// Similar to how we can have consumer threads and remove the need to poll for results, we can also create producers.
// Producers threads will continuously call their functions, eliminating the need for input(...) calls in the pipeline.
//---------------------------------------------------------------------------------------------------------------------

int main() {
  using namespace std::chrono_literals;

  // A producer function. It will be invoked in a loop in the producer thread.
  // We'll use 10ms to simulate slower input
  constexpr auto get_int = []() -> int {
    std::this_thread::sleep_for(10ms);
    return -1;
  };

  constexpr auto square = [](auto x) { return x * x; };

  // Create a basic pipe responsible for squaring a number produced by our producer
  auto pipe = tdp::producer{get_int} >> square >> tdp::output;

  // Wait a little, to give the pipeline the opportunity to produce some results
  std::this_thread::sleep_for(200ms);

  // We pause the producer and check how many outputs it generated
  pipe.pause();

  int executions = 0;

  // If it didn't stop, it will never leave this loop
  while (pipe.available()) {
    [[maybe_unused]] auto res = pipe.wait_get();
    ++executions;
  }

  std::cout << "Runs before pause: " << executions << "\n";
  std::cout << "pipe.running(): " << std::boolalpha << pipe.running() << "\n";

  // We now resume execution
  pipe.resume();

  // Sleep a little
  std::this_thread::sleep_for(50ms);
  std::cout << "pipe.running(): " << std::boolalpha << pipe.running() << "\n";

  // The destructor stops the pipeline
}
