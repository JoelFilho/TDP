#include <iostream>

#include "tdp/pipeline.hpp"

//---------------------------------------------------------------------------------------------------------------------
// This example demonstrates how to use consumer threads
//
// When you don't want the resposibility of acquiring the result of the pipeline, you can make TDP do it for you.
//---------------------------------------------------------------------------------------------------------------------

int main() {
  // First, we'll create the functions the pipeline will work.
  // We can use C++'s lambda expressions for that.

  auto scale = [](int x, double y) { return int(x * y); };
  auto make_even = [](int x) { return x & ~1; };

  // We will use a thread to consume the output.
  // It can also be either a funcion or a lambda
  // And lambdas can have captures!
  int count = 0;
  auto print = [&count](int x) {
    count++;
    std::cout << "Consumed: " << x << "\n";
  };

  // Now, we define a pipeline.
  // The first two steps are the same: an input<int, double> and the functions we'll use
  // The output, however, will be replaced by tdp::consumer:
  auto pipeline = tdp::input<int, double> >> scale >> make_even >> tdp::consumer{print};

  // As before, the pipeline is now running.
  // We can provide inputs to it, and the results will be processed in parallel and displayed without request!
  pipeline.input(1, 3.5);
  pipeline.input(2, 3.5);
  pipeline.input(7, 0.75);

  // We can wait a little, do anything else, and the production will be done.
  // Sometimes we want to guarantee the pipeline has finished running.
  // For that, we'll use the idle() member function.
  // TODO
  using namespace std::chrono_literals;
  std::this_thread::sleep_for(100ms);

  // The capture we used from the lambda!
  std::cout << "print() has been called " << count << " times.\n";
}