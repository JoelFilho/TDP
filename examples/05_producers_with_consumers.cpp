#include <iostream>

#include "tdp/pipeline.hpp"

//---------------------------------------------------------------------------------------------------------------------
// This example shows how producers work in combination to consumers
//---------------------------------------------------------------------------------------------------------------------

// We will use a functor class for the consumer
// It can have anything inside it, it just needs to be move-constructible.
struct consume {
  // We will print a count of how many times it consumed something
  int count = 0;
  
  ~consume() {
    if (count) // Stuff is move-constructed, so we may have a few more destruction calls...
      std::cout << "The consumer was called " << count << " times.\n";
  }

  template <typename T>
  void operator()(T) {
    count++;
  }
};

int main() {
  using namespace std::chrono_literals;

  // A Producer that returns an integer and takes 10ms to execute
  constexpr auto get_int = []() -> int {
    std::this_thread::sleep_for(10ms);
    return -1;
  };

  // Our processing function
  constexpr auto square = [](auto x) { return x * x; };

  // When the processing is faster than the production, it's usually a good idea to use a blocking policy.
  // And, as a queue will not be bigger than one, we can use a buffer instead.
  // Experiment changing it to a queue and notice it won't make a difference in this case.
  auto pipe = tdp::producer{get_int}(tdp::policy::triple_buffer) >> square >> tdp::consumer{consume()};

  // We can do anything in this thread, including sleep. The pipeline is running on
  std::this_thread::sleep_for(200ms);

  // We exit the program.
  // The consumer will be destroyed and, on destruction, it will print how many times it was called.
}
