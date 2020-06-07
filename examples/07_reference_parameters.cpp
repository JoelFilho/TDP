#include <algorithm>
#include <iostream>
#include <string>

#include "tdp/pipeline.hpp"

//---------------------------------------------------------------------------------------------------------------------
// Pipeline stages in TDP can't have reference outputs. This guarantees no dangling references will be passed.
// Queues utilized internally by TDP also require value semantics, so passing references would be counterproductive.
//
// In order to prevent additional move constructor calls, one might want pipeline stages to have reference inputs.
// This is a very small optimization, but could reduce some latency in specific applications.
//---------------------------------------------------------------------------------------------------------------------

int main() {
  // Each pipeline step is called with an xvalue of the previous stage's result.
  // It means we can accept a value and move from it, as we've been doing, or we can process directly on the rvalue:
  constexpr auto process_string = [](std::string&& s) {
    std::reverse(s.begin(), s.end());
    // We can move our string from here and pass it forward, without reallocations.
    return std::move(s);
  };

  // lvalue references don't bind to rvalues, so the previous function won't work in a pipeline if we change && to &.
  // But const references do so, we can use const& in our input type, if no modification is going to be done:
  constexpr auto print_string = [](const std::string& s) { std::cout << s << std::endl; };

  // We declare the pipeline.
  // The input must always contain value types.
  auto pipeline = tdp::input<std::string> >> process_string >> tdp::consumer{print_string};

  // Watch it as it goes.
  pipeline.input("!dlroW olleH");
  pipeline.input("TACOCAT");

  // We wait until execution is complete with wait_until_idle()
  // This way, we free the processor from scheduling this thread until then, and don't need to check pipeline.idle()
  pipeline.wait_until_idle();
}