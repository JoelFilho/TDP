#include <iostream>
#include <string_view>

#include "tdp/pipeline.hpp"

//---------------------------------------------------------------------------------------------------------------------
// The Darkest Pipeline provides two ways of declaring inputs and two ways of declaring outputs.
// From those, we can do a lot. Let's take a look.
//---------------------------------------------------------------------------------------------------------------------

// We've used functions and lambdas in the previous examples.
// We can also use functor classes as pipeline stages!
template <typename T>
struct get {
  T operator()() {
    // Simulating a thread that produces a result every 10ms
    using namespace std::chrono_literals;
    std::this_thread::sleep_for(10ms);
    return {};
  }
};

struct add {
  // This function adds any two numbers
  template <typename T1, typename T2>
  auto operator()(T1 a, T2 b) {
    return a + b;
  }
};

struct square {
  // This function takes the square of a number
  template <typename T>
  auto operator()(T x) {
    return x * x;
  }
};

template <typename T>
struct consume {
  // This function returns void. It's going to be used as the end of a pipeline
  void operator()(T) {}
};

// This function will show us its name, as seen by the compiler.
// This will help us understand the nature of pipelines.
template <typename... Ts>
void print_type(Ts&...);

int main() {
  // Pipelines using manual input and output
  auto p0 = tdp::input<int, int> >> add() >> tdp::output;
  auto p1 = tdp::input<double> >> square() >> tdp::output;
  auto p2 = tdp::input<int, int> >> add() >> square() >> tdp::output;

  // Pipeline using a producer and polled output
  // The "Producer threads" example shows how you can control production.
  auto p3 = tdp::producer{get<int>()} >> square() >> tdp::output;

  // You can't use a pipeline as input>>output, but can use a producer to generate output!
  auto p4 = tdp::producer{get<int>()} >> tdp::output;

  // Pipelines using user-generated input and a consumer
  // The consumer processes the data, and no output given.
  // With templates, we can have two similar pipelines for different types!
  auto p5 = tdp::input<int, int> >> add() >> square() >> tdp::consumer{consume<int>()};
  auto p6 = tdp::input<double, double> >> add() >> square() >> tdp::consumer{consume<double>()};

  // Producer-consumer pipelines.
  // We don't need to provide input OR output, it just runs until destruction.
  // See more details on the "Producers with Consumers" example.
  auto p7 = tdp::producer{get<int>()} >> square() >> tdp::consumer{consume<int>()};
  auto p8 = tdp::producer{get<double>()} >> tdp::consumer{consume<double>()};

  // We can set policies to our pipelines!
  // See the Execution Policies example for more details.
  auto p9 = tdp::input<int, int>(tdp::policy::queue) >> add() >> square() >> tdp::output;
  auto p10 = tdp::input<int>(tdp::policy::triple_buffer) >> square() >> tdp::output;

  // Here, we see the underlying types utilized. And why we should use auto in this library.
  print_type(p1);
  print_type(p2);
  print_type(p3);
  print_type(p4);
  print_type(p5);
  print_type(p6);
  print_type(p7);
  print_type(p8);
  print_type(p9);
  print_type(p10);
}

//---------------------------------------------------------------------------------------------------------------------
// Don't worry about this function. It's not part of the example, only its output is.
// 
// This was tested on GCC 7/8 (Linux), Clang 8/9 (Linux) and MSVC 2017/2019 (Windows).
// Also works with Clang + VS 2019. Guaranteed not to work on Clang + VS 2017.
// Come on, don't look at me with that face. It's an example, you're probably not even running it.
//---------------------------------------------------------------------------------------------------------------------
template <typename... Ts>
void print_type(Ts&...) {
#ifdef _MSC_VER
  constexpr const char* name = __FUNCSIG__;
  constexpr char first_char = '(';
  constexpr char last_char = ')';
  constexpr auto offset = 1u;
  constexpr auto offset_r = 2u;
#elif defined(__clang__)
  constexpr const char* name = __PRETTY_FUNCTION__;
  constexpr char first_char = '<';
  constexpr char last_char = ']';
  constexpr auto offset = 1u;
  constexpr auto offset_r = 1u;
#elif defined(__GNUC__)
  const char* name = __PRETTY_FUNCTION__;
  constexpr char first_char = '{';
  constexpr char last_char = '}';
  constexpr auto offset = 1u;
  constexpr auto offset_r = 0u;
#else
#error "Compiler not supported for this example."
#endif

  auto find_char = [=](char c) {
    const char* pos = name;
    for (; *pos != c && *pos != '\0'; pos++) {
    }
    return pos;
  };

  const char* name_begin = find_char(first_char);
  const char* name_end = find_char(last_char);

  if (*name_begin == '\0') {
    std::cout << ". " << name << "\n";
  } else if (*name_end == '\0') {
    std::cout << ". " << name_begin << "\n";
  } else {
    name_begin += offset;
    name_end -= offset_r;
    std::cout << "- " << std::string_view{name_begin, std::size_t(name_end - name_begin)} << "\n";
  }
}