#include <iostream>

#include "tdp/pipeline.hpp"

//---------------------------------------------------------------------------------------------------------------------
// Due to how they're implemented, TDP pipelines aren't copy-constructible nor move-constructible.
// In order to allow ownership changes of a pipeline, TDP allows dynamic construction in smart pointers.
//
// To construct a pipeline with smart pointers, use one of these tokens at the end of the pipeline, after a /
//     tdp::as_unique_ptr: Creates an std::unique_ptr<...> containing the pipeline
//     tdp::as_shared_ptr: Creates an std::shared_ptr<...> containing the pipeline
//
// This example will demonstrate how this can be done.
//---------------------------------------------------------------------------------------------------------------------

int main() {
  // Reminder: we can use constexpr lambdas for our pipeline stages!
  constexpr auto get_int = []() -> int {
    using namespace std::chrono_literals;
    std::this_thread::sleep_for(10ms);
    return -1;
  };
  constexpr auto add = [](int x, int y) { return x + y; };
  constexpr auto null_consumer = [](int) {};

  // This creates an std::unique_ptr containing the pipeline
  auto unique_pipeline = tdp::input<int, int> >> add >> tdp::output / tdp::as_unique_ptr;

  // As it's a smart pointer, it has pointer semantics:
  unique_pipeline->input(2, 2);
  std::cout << "2+2=" << unique_pipeline->wait_get() << std::endl;

  // This creates an std::shared_ptr containing the pipeline
  // It's useful for sharing ownership of the pipeline between a consumer class and a producer class,
  // when producer and consumer threads aren't desired.
  // A reminder: TDP uses single-producer, single-consumer queues.
  //             Don't use this to share to multiple producers/consumers!
  auto shared_pipeline = tdp::input<int, int> >> add >> tdp::output / tdp::as_shared_ptr;

  // Declaration of output type can be combined with policies
  // The wrapper type must always follow the policy!
  auto pipeline = tdp::input<int, int> >> add  //
                  >> tdp::consumer{null_consumer} / tdp::policy::triple_buffer / tdp::as_unique_ptr;

  // As always, they work with producers:
  auto producer_pipe_shared = tdp::producer{get_int} >> tdp::consumer{null_consumer} / tdp::as_shared_ptr;
  auto producer_pipe_unique = tdp::producer{get_int} >> tdp::output / tdp::as_unique_ptr;

  std::cout << "Producers running? " << std::boolalpha << producer_pipe_shared->running() << " and "
            << producer_pipe_unique->running() << std::endl;

  // As they're unique pointers, lifetimes will also end at the end of the scope!
}