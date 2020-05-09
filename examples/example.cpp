#include <iostream>

#include "tdp/pipeline.hpp"

int print_index = 0;

template <typename... Ts>
void print(Ts&...) {
#ifdef _MSC_VER
  std::cout << print_index++ << ": " << __FUNCSIG__ << '\n';
#else
  std::cout << print_index++ << ": " << __PRETTY_FUNCTION__ << '\n';
#endif
}

int main() {
  constexpr auto get_int = []() -> int { return -1; };
  constexpr auto add = [](auto x, auto y) { return x + y; };
  constexpr auto square = [](auto x) -> decltype(x * x) { return x * x; };
  constexpr auto consume = [](auto x) {
    struct Counter {
      int count = 0;
      ~Counter() { std::cout << "Calls to consume(): " << count << '\n'; }
    };
    thread_local Counter c;
    c.count++;
  };

  auto p1 = tdp::input<int, int> >> add >> tdp::output;
  auto p2 = tdp::input<double> >> square >> tdp::output;
  auto p3 = tdp::input<int, int> >> add >> square >> tdp::output;

  auto p4 = tdp::input<int, int> >> add >> square >> tdp::consumer{consume};
  auto p5 = tdp::producer{get_int} >> square >> tdp::consumer{consume};
  auto p6 = tdp::producer{get_int} >> square >> tdp::output;

  auto p7 = tdp::producer{get_int} >> tdp::consumer{consume};
  auto p8 = tdp::producer{get_int} >> tdp::output;

  // TODO: constexpr auto p3 = tdp::input<int, int> + tdp::policy::queue >> add >> square >> tdp::output;

  print(p1);
  print(p2);
  print(p3);
  print(p4);
  print(p5);
  print(p6);
  print(p7);
  print(p8);
}
