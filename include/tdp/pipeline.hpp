#ifndef TDP_PIPELINE_HPP
#define TDP_PIPELINE_HPP

#include "pipeline.impl.hpp"

//-------------------------------------------------------------------------------------------------
// Welcome to The Darkest Pipeline!
//
// This is a library for easier multi-threaded processing, using the concept of pipelines.
//
// The pipelines are declared using a terse DSL, and are launched using the syntax:
//
//   auto pipeline = Input >> functionA >> functionB >> ... >> Output;
//
// Input can be declared two ways: tdp::input<Args...> and tdp::producer{ functor }
//   - See the "Input Types" section below for more information
//
// Output can also have two types: tdp::output and tdp::consumer{ functor }
//   - See the "Output Types" section below for more information
//
// function... can be function pointers, lambda objects or functor objects:
//
//     int square_function(int x) { return x*x; }
//     auto square_lambda = [](int x) { return x*x; };
//     struct square_functor { int operator()(int){ return x*x; } };
//
// The input type of each function in the pipeline must be the output of the previous stage.
//
// Optionally, the data structure utilized for communication can be modified.
//   - See the "Execution Policies" section for information on this topic
//
// For usage examples, see the examples folder.
//
//-------------------------------------------------------------------------------------------------

namespace tdp {

//-------------------------------------------------------------------------------------------------
// Input Types
//
// The input options describe the way the input is provided to the rest of the pipeline:
//
// tdp::input<Args...>
//
//     Describes an user input.
//     Input must be provided by calling pipeline.input(args...)
//
//     Example:
//       auto combine = [](std::string s, int x){ return s + ": " + std::to_string(x); };
//       auto pipeline = tdp::input<std::string, int> >> combine >> tdp::output;
//       pipeline.input("Hello", 5);
//       std::cout << pipeline.wait_get() << std::endl; // Prints "Hello: 5"
//
// tdp::producer{ function }
//
//     Describes automatically-generated input.
//     Input is generated by repeatedly calling function().
//     function must be callable without arguments and return a non-void type.
//
//     A pipeline created with a producer doesn't have the input(args...) member function.
//     Instead, it provides pause(), resume() and running() in its interface.
//
//-------------------------------------------------------------------------------------------------

template <typename... InputArgs>
inline constexpr detail::input_type<InputArgs...> input = {};

using detail::producer;

//-------------------------------------------------------------------------------------------------
// Output Types
//
// Both output options are utilized to defined the behavior of the pipeline.
// After an output type is utilized in the pipeline definition, the pipeline is returned.
//
// The basic form of output is polled output.
// It means the output is stored in a queue and available for manual access.
//
// Example 1: Using a polled output
//
//     auto pipeline = tdp::input<int> >> square >> tdp::output;
//         // At this point, the pipeline is constructed, and threads start running.
//     pipeline.input(5); // Provides an input for the pipeline
//     auto res = pipeline.wait_get(); // Gets the result, equivalent to calling square(5)
//
// Example 2: Using a consumer/sink thread
//
//     auto print = [](auto x){ std::cout << x << std::endl; };
//     auto pipeline = tdp::input<int> >> square >> tdp::consumer{print};
//         // pipeline.wait_get(); will yield a compiler error.
//     pipeline.input(5);
//         // the value of square(5) will be automatically printed
//
//-------------------------------------------------------------------------------------------------

/// Determines the end of the pipeline, indicating the output should be polled.
/// Output from the last function will be available on the interface.
inline constexpr detail::end_type output = {};

/// A consumer output, eliminating the need for polling.
/// Must be used only at the end of the pipeline declaration.
/// Usage: ... >> tdp::consumer{ function }; // With curly braces!
/// Return type of 'function' must be void.
using detail::consumer;

}  // namespace tdp

//-------------------------------------------------------------------------------------------------
// Execution Policies
//
// A policy determines the internal communication data structure used in a pipeline.
// When not specified, the default is a blocking queue.
//
// The syntax is:
//   Input(policy) >> ...
//
// Examples:
//    // Use a queue with user-provided input:
//    auto pipeline = tdp::input<int>(tdp::policy::queue) >> /* functions */ >> tdp::output;
//
//    // Use a producer and triple-buffering:
//    auto pipeline = tdp::producer{ function }(tdp::policy::triple_buffer)
//                    >> functions >> tdp::output;
//-------------------------------------------------------------------------------------------------

namespace tdp::policy {

/// Triple buffering policy, discarding missed outputs.
/// Better used when having the most recent data is more relevant, e.g. real-time video.
inline constexpr detail::policy_type<util::blocking_triple_buffer> triple_buffer = {};

/// Queue, with "unlimited" storage.
/// Better for cases where no input can be missed.
/// Beware of unbalanced pipelines, they can cause high memory usage.
inline constexpr detail::policy_type<util::blocking_queue> queue = {};

};  // namespace tdp::policy

#endif