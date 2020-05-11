# TODO

## Functional

- [x] Allow controlling producers
- [ ] Additional concurrent queues
  - [x] Blocking triple-buffer
  - [ ] Lock-free SPSC queue
  - [ ] Lock-free triple buffering
- [x] Non-locking get() interface
- [x] Apply execution policies
- [ ] `idle()` interface
- [ ] Use `std::invoke` wherever applicable
- [ ] `[[nodiscard]]`, `const` and `noexcept` correctness

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