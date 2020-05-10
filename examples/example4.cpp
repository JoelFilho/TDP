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

  // Using queue policy. We don't want to miss outputs.
  auto pipe = tdp::producer{get_int}(tdp::policy::queue) >> square >> tdp::output;
  std::this_thread::sleep_for(200ms);

  // We pause the producer and check how many outputs it generated
  pipe.pause();

  int executions = 0;

  // If it didn't stop, it will never leave this loop
  while (pipe.available()) {
    [[maybe_unused]] auto res = pipe.get();
    ++executions;
  }

  std::cout << "Runs before pause: " << executions << "\n";
  std::cout << "pipe.running(): " << std::boolalpha << pipe.running() << "\n";

  // We now resume execution
  pipe.resume();

  // Sleep a little
  std::this_thread::sleep_for(100ms);
  std::cout << "pipe.running(): " << std::boolalpha << pipe.running() << "\n";

  // The destructor stops the pipeline
}
