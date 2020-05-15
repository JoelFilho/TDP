#include <iostream>

#include "tdp/pipeline.hpp"

//---------------------------------------------------------------------------------------------------------------------
// In this example, we will create a pipeline to add two numbers
//---------------------------------------------------------------------------------------------------------------------

// First, we define a function that will be used in the pipeline
int add(int x, int y) {
  return x + y;
}

int main() {
    // Then, we declare a pipeline.

    // First element is the input. As add takes two integers, we use tdp::input<int,int>
    // Second step is defining the pipeline of function calls. In our case, it's add. We do that with operator >>.
    // You may define as many steps you want, as long as their input is compatible with the previous step's output.
    // Then, we construct the pipeline by giving it an output. It's tdp::output.
    auto pipeline = tdp::input<int, int> >> add >> tdp::output;

    // At this point, the pipeline is already running! It has one thread, responsible for calling add().
    // To call add(), we must provide an input.
    pipeline.input(2, 2);

    // We can repeat this whenever we want, how many times we want.
    pipeline.input(1, 5);
    pipeline.input(3, 6);

    // As the pipeline runs, we may want to get an output.
    // We can do that by using wait_get():
    int result = pipeline.wait_get();

    // As pipelines work first-in, first out, we know the result should be 4:
    std::cout << "First output: " << result << "\n";

    // We can also loop to get a value from the output.
    while(pipeline.available()){
        std::cout << "Loop output: " << pipeline.wait_get() << "\n";
    }

    // However, only use wait_get() if you know there will b anything available.
    // Otherwise, the thread can hang indefinitely!
    // If you don't want to call available(), use the optional interface:
    std::optional<int> opt_result = pipeline.try_get();
    
    // Then we can check the result before using it
    if(!opt_result){
        std::cout << "Pipeline is empty, can't get another result!\n";
    }

    // At the end, we don't need to do anything.
    // The pipeline stops all threads when its destructor is called.
}