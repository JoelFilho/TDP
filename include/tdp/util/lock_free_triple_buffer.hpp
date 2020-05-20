// The Darkest Pipeline - https://github.com/JoelFilho/TDP
// lock_free_triple_buffer.hpp - A lock-free implementation of a triple buffer

// Copyright Joel P. C. Filho 2020 - 2020
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE.md or copy at https://www.boost.org/LICENSE_1_0.txt)

#ifndef TDP_LOCK_FREE_TRIPLE_BUFFER_HPP
#define TDP_LOCK_FREE_TRIPLE_BUFFER_HPP

#include <array>
#include <atomic>
#include <cstdint>
#include <optional>

#include "helpers.hpp"

namespace tdp::util {

struct buffer_control_block {
  std::uint8_t write_idx : 2;
  std::uint8_t buffer_idx : 2;
  std::uint8_t read_idx : 2;
  bool available : 2;

  [[nodiscard]] friend buffer_control_block write_value(const buffer_control_block& block) noexcept {
    return {block.buffer_idx, block.write_idx, block.read_idx, true};
  }

  [[nodiscard]] friend buffer_control_block read_value(const buffer_control_block& block) noexcept {
    return {block.write_idx, block.read_idx, block.buffer_idx, false};
  }
};

template <typename T>
class lock_free_triple_buffer {
  using control_block_t = std::atomic<buffer_control_block>;
  static_assert(dependent_bool<control_block_t::is_always_lock_free, T>);

 public:
  void push(T val) {
    auto old = _control.load();

    _buffer[old.write_idx] = std::move(val);

    while (!_control.compare_exchange_weak(old, write_value(old)))
      ;
  }

  T pop() {
    while (!_control.load().available)
      ;

    auto old = _control.load();
    auto next = read_value(old);

    while (!_control.compare_exchange_weak(old, next))
      next = read_value(old);

    return std::move(_buffer[next.read_idx]);
  }

  template <typename Pred>
  std::optional<T> pop_unless(Pred&& p) {
    while (!_control.load().available)
      if (p())
        break;

    auto old = _control.load();

    if (!old.available)
      return std::nullopt;

    auto next = read_value(old);
    while (!_control.compare_exchange_weak(old, next))
      next = read_value(old);

    return std::move(_buffer[next.read_idx]);
  }

  bool empty() const noexcept { return !_control.load().available; }

  void wake() {}

 private:
  std::array<T, 3> _buffer;  // TODO: similar to the blocking version, should it support non-default construction?
  control_block_t _control{{0, 1, 2, false}};
};

}  // namespace tdp::util

#endif