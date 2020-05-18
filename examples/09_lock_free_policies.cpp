#include <cmath>
#include <iostream>
#include <utility>

#include "tdp/pipeline.hpp"

//---------------------------------------------------------------------------------------------------------------------
// We've seen in "Execution Policies" that we can use blocking or lock-free structures in our pipelines.
//
// This example presents a lock-free use case.  In our case, the production is faster than the consumption.
// In this situation, for real-time processing, we don't want to use queues, otherwise they'll grow indefinitely.
//
// So, this example will show where lock-free triple buffers are the best: constant latency with high contention.
// The rates this example uses are very unbalanced, unlike in real life, in order to show
//---------------------------------------------------------------------------------------------------------------------

// We will use the same consumer idea from the "Producers with Consumers" example.
struct consume {
  const char* name;
  int last = -1;
  int count = 0;

  ~consume() {
    if (count)
      std::cout << "- " << name << "'s consumer was called " << count << " times. "
                << "The last seen input index was " << last << ".\n";
  }

  template <typename T>
  void operator()(const T& val) {
    last = val.first;
    count++;
  }
};

// The runner we'll use to compare the policies.
template <const auto& policy>
void run(const char* name, int iterations, std::chrono::milliseconds delay) {
  // The pipeline will compute the norm of a vector, given by its (x,y) input.
  // We'll add a tag to store the iteration it was invoked.
  auto sum_of_squares = [](int id, auto x, auto y) { return std::pair{id, x * x + y * y}; };
  auto sqrt = [](auto x) { return std::pair{x.first, std::sqrt(x.second)}; };

  // Pipeline construction, as always
  auto pipeline = tdp::input<int, double, double> >> sum_of_squares >> sqrt >> tdp::consumer{consume{name}} / policy;

  // We'll consider the time of input, in case it's significant and different between policies
  auto start = std::chrono::high_resolution_clock::now();

  // Now, we provide the pipeline with a lot of inputs, without delay.
  // It'll simulate a toy case where the production is way faster than the processing.
  for (int i = 1; i <= iterations; i++) {
    pipeline.input(i, i - 1, i + 1);
  }

  auto end = std::chrono::high_resolution_clock::now();
  auto deductible = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

  // Then we just wait. The output will be given when the destructor of the consumer is called.
  if (delay > deductible)
    std::this_thread::sleep_for(delay - deductible);
}

// Then we run all policies with a time limit
int main() {
  using namespace std::chrono_literals;
  constexpr int iterations = 1'000'000;
  constexpr auto delay = 100ms;

  std::cout << "Each policy will be given " << delay.count() << "ms to process " << iterations
            << " different input values.\n";

  // Blocking triple buffering should perform bad in this case.
  // It's better for when processing is faster than production, and we have a lot of stages.
  run<tdp::policy::triple_buffer>("blocking   triple-buffer", iterations, delay);

  // Lock-free triple buffering should perform better.
  // As the threads keep running because we never lock a mutex, it should be faster.
  run<tdp::policy::triple_buffer_lockfree>("lock-free  triple-buffer", iterations, delay);

  // As noted in the introduction, queues are bad for this kind of application.
  // Throughput here will be good, but latency would increase eternally.
  run<tdp::policy::queue>("blocking unbounded queue", iterations, delay);
}