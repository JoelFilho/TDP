# TDP's To-do list: Alpha release

## Functionality

- [x] Allow controlling producers
- [ ] Additional concurrent queues
  - [x] Blocking triple-buffer
  - [ ] Lock-free SPSC queue
  - [ ] Lock-free triple buffering
- [x] Non-locking `get()` interface
  - [x] Rename old interface to make it clearer it blocks
- [x] Apply execution policies
- [ ] `idle()` interface
  - [x] Rename `running()` to remove ambiguity
- [x] Use `std::invoke` wherever applicable
- [x] Prohibit reference outputs (mutable lvalue reference parameters already invalid)
  - [ ] Allow reference output, but use `decay_t` for storage and propagation?
- [x] Create a move-constructible variant of the pipeline (smart pointer?)
  - [x] Restructure the policy interface to support policy + construction type (`operator /`?)
- [ ] `[[nodiscard]]`, `const` and `noexcept` correctness
  - [x] `[[nodiscard]]` in member functions
  - [ ] `[[nodiscard]]` in types (Blocker: clang-format breaks `&&`: [Bug report](https://bugs.llvm.org/show_bug.cgi?id=45942))
  - [x] `const`
  - [ ] `noexcept`: Will not have conditional `noexcept` for first release. If it throws, it terminates.
- [ ] Allow `input >> consumer`, symmetric to `producer >> output`?

## Project

- [ ] Documentation
  - [x] Comments
  - [ ] Markdown
- [ ] `static_assert` descriptions
- [ ] Unit tests
- [x] Better examples
- [ ] License headers
- [ ] Improve CMake integration (install interface)
- [ ] CI
- [ ] Add to VCPKG/Conan when feature complete

## Possible features

- Declaring pipeline "slices": reusable building blocks
- Forking (task parallelism)
- Tuple adapter: calling `std::apply` in a tuple return in the pipeline
- Load analysis (possible issue: false sharing)