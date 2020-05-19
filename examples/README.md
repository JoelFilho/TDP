# TDP Examples

These examples are released under [the Unlicense](UNLICENSE.md). Copy them and modify them as much as you want.

## Introduction [\[code\]](00_introduction.cpp)

* How to define a pipeline with user input and output.
* How to provide input
* How to access output

## Consumer Threads [\[code\]](01_consumer_threads.cpp)

* How to define a consumer
* How to use a pipeline with user input and a consumer

## Producer Threads [\[code\]](02_producer_threads.cpp)

* How to define a producer
* How to control production

## Pipeline Types [\[code\]](03_pipeline_types.cpp)

* Possible input and output combinations to declare pipelines
* Why it's a good idea to use `auto`

## Execution policies [\[code\]](04_execution_policies.cpp)

* How to use policies
* Differences between the data structures utilized

## Producers with Consumers [\[code\]](05_producers_with_consumers.cpp)

* The combination of producer threads and consumer threads (independent pipeline)

## Smart Pointer Wrappers [\[code\]](06_smart_pointer_wrappers.cpp)

* How to declare and use pipelines with `std::unique_ptr` and `std::shared_ptr`

## Reference Parameters [\[code\]](07_reference_parameters.cpp)

* A display of the internals of the library, allowing micro-optimizations

## Member Functions [\[code\]](08_member_functions.cpp)

* An obscure language feature, that just works, if you're insane enough to try it

## Lock-free policies [\[code\]](09_lock_free_policies.cpp)

* A use case where lock-free is good