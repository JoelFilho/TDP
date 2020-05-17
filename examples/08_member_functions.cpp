#include <cmath>
#include <iostream>

#include "tdp/pipeline.hpp"

//---------------------------------------------------------------------------------------------------------------------
// One small feature of TDP is that it allows the use of member functions as pipeline stages.
//---------------------------------------------------------------------------------------------------------------------

struct Vec2f {
  float x{};
  float y{};

  // The member function that will be used in our pipeline
  float norm() const { return std::sqrt(x * x + y * y); }
};

int main() {
  // We use a pointer to member function as our result!
  auto pipeline = tdp::input<Vec2f> >> &Vec2f::norm >> tdp::output;

  // The rest occurs as always:
  pipeline.input({0, 0});
  pipeline.input({1, 1});
  std::cout << "norm({0,0}) = " << pipeline.wait_get() << std::endl;
  std::cout << "norm({1,1}) = " << pipeline.wait_get() << std::endl;
}