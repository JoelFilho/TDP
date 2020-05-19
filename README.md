# TDP: The Darkest Pipeline

**The Darkest Pipeline** is a header-only library for building multi-threaded software pipelines on modern C++.

Built on C++17, it allows static declaration of processing pipelines using an embedded DSL (Domain-Specific Language):

```c++
auto world = [](std::string s){ return s + " World!\n"; };
auto print = [](auto msg){ std::cout << msg; };

// Pipeline declaration
auto pipeline = tdp::input<std::string> >> world >> tdp::consumer{print};

// Usage: provide an arbitrary amount of data, let TDP do the rest.
pipeline.input("Hello");
pipeline.input("Salutations");
pipeline.input("Ahoy");
```

Jump to [Overview](#overview) for a quick overlook at the DSL API. **See the [examples](examples/README.md)** for use cases and deeper details.

Note: **TDP is still under development**, so instabilities may exist, and functionality may be missing. See the [TODO file](TODO.md) for the roadmap. For feature requests or bug reports, [submit an issue](https://github.com/JoelFilho/TDP/issues/new) or [contact me on Twitter](https://twitter.com/_JoelFilho).

**Index**:

- [Features](#features)
- [Why pipelines?](#why-pipelines)
- [Getting Started](#getting-started)
- [Overview](#overview)
  - [Input Types](#input-types)
  - [Output Types](#output-types)
  - [Stages](#stages)
  - [Policies](#policies)
  - [Wrappers](#wrappers)
- [License](#license)

## Features

* Simplified declaration using an [embedded DSL](#overview)
* Launches one thread per pipeline stage, and manages lifetime of all threads
* Starts at construction, stops at destruction. No need for `start()`/`stop()` functions!
* Structure built at compile time, using stack storage whenever possible
  * Provides [`std::unique_ptr` and `std::shared_ptr` wrappers](#wrappers) for ownership models
* Allows direct user input and output or producer/consumer threads ([see the types](#input-types))
* [Allows selection of many internal data types](#policies), suitable for diverse applications

## Why pipelines?

We can't always parallelize a process. Some algorithms are inherently sequential. Sometimes we depend on blocking I/O.

For example, video capture from a USB camera can't be parallel, neither can displaying that video:

```c++
auto frame = camera.capture();
auto output = process(frame);
display(output);
```

In this example, even if we can parallelize `process()`, the throughput of the system is limited by the accumulated latency of all three stages.
Using a pipeline, the latency should be similar, but the throughput is limited by _the highest_ latency among the three stages.

In short, use a pipeline where **throughput matters, but it can't be reached by reducing latency**.

## Getting Started

**The Darkest Pipeline** is a header-only library. You can copy the contents of the `include` folder anywhere into your project!

TDP also utilizes CMake, so it allows:

* Using this repository as a subdirectory, with `add_subdirectory`
  * You can use this git repository as a submodule!
* Installing, with the CMake install interface
  * e.g. on Linux, a `sudo make install` will make it globally available in the system

Then, include `tdp/pipeline.hpp` in your code, and enjoy a life of easier pipelining!

## Overview

    auto pipeline = input_type >> [stage1 >> ... stageN >>] >> output_type [/policy] [/wrapper];

### Input Types

* `tdp::input<Args...>`: User-provided input. Can be provided from main thread with `pipeline.input(args...)`.
* `tdp::producer{functor}`: A thread that automatically calls `functor()` to generate data for the pipeline. Allows control with the `pause()`/`resume()` interface.

### Output Types

* `tdp::output`: User-polled output. Can be obtained from main thread with `wait_get()` (blocking) or the non-blocking member function `try_get()`.
* `tdp::consumer{functor}`: A thread that processes the pipeline output and returns `void`, removing the output interface from the pipeline.

### Stages

The additional pipeline stages are functions that operate on input data and return data for the next pipeline stage.

They can be:

* Function pointers/references
* Lambdas
* Function objects, e.g. `std::function`
* Pointers to member functions

### Policies

Execution policies define the internal data structure utilized for communication between stages. TDP currently provides these policies:

* `tdp::policy::queue` (default): A blocking unbounded queue
* `tdp::policy::triple_buffer`: Utilizes triple-buffering, for applications where the latest value is more important than processing all values
* `tdp::policy::triple_buffer_lockfree`: A lock-free implementation of triple buffering, for applications with high throughput

### Wrappers

By default, a pipeline is constructed on the stack. Due to its internals, it can't be copy-constructed, nor move-constructed.

For applications where move operations are required, or shared ownership is desired, TDP allows using smart pointer wrappers:

* `tdp::as_unique_ptr`: Returns an `std::unique_ptr` containing the pipeline
* `tdp::as_shared_ptr`: Returns an `std::shared_ptr` containing the pipeline

## License

Copyright © 2020 Joel P. C. Filho

This software is released under the Boost Software License - Version 1.0. Refer to the [License file](LICENSE.md) for more details.

The software in the `examples` folder is on public domain, released under [The Unlicense](examples/UNLICENSE.md).