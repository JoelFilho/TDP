// The Darkest Pipeline - https://github.com/JoelFilho/TDP
// pipeline.impl.hpp - Implementation details of TDP pipelines

// Copyright Joel P. C. Filho 2020 - 2020
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE.md or copy at https://www.boost.org/LICENSE_1_0.txt)

#ifndef TDP_PIPELINE_IMPL_HPP
#define TDP_PIPELINE_IMPL_HPP

#include <array>
#include <atomic>
#include <functional>
#include <thread>
#include <tuple>
#include <type_traits>

#include "util/blocking_queue.hpp"
#include "util/blocking_triple_buffer.hpp"
#include "util/helpers.hpp"
#include "util/lock_free_triple_buffer.hpp"
#include "util/type_list.hpp"

namespace tdp::detail {

//-------------------------------------------------------------------------------------------------
// Processing threads
//-------------------------------------------------------------------------------------------------

template <template <typename...> class Queue, typename Input, typename Callable, typename = void, typename = void>
struct thread_worker;

// Normal input
template <template <typename...> class Queue, typename... InputArgs, typename Callable>
struct thread_worker<Queue, jtc::type_list<InputArgs...>, Callable,  //
    std::enable_if_t<sizeof...(InputArgs) != 0>,                     //
    std::enable_if_t<!std::is_same_v<std::invoke_result_t<Callable, InputArgs...>, void>>> {
  using input_t = std::tuple<InputArgs...>;
  using output_t = std::invoke_result_t<Callable, InputArgs...>;

  Callable _f;
  Queue<input_t>& _input_queue;
  Queue<output_t>& _output_queue;
  const std::atomic_bool& _stop;

  void operator()() noexcept {
    while (!_stop) {
      auto val = _input_queue.pop_unless([&] { return _stop.load(); });
      if (!val)
        break;
      auto&& res = std::apply(_f, std::move(*val));
      _output_queue.push(std::move(res));
    }
    _output_queue.wake();
  }
};

// Producer thread
template <template <typename...> class Queue, typename Callable>
struct thread_worker<Queue, jtc::type_list<>, Callable> {
  using output_t = std::invoke_result_t<Callable>;

  Callable _f;
  Queue<output_t>& _output_queue;
  const std::atomic_bool& _pause;
  const std::atomic_bool& _stop;

  void operator()() noexcept {
    while (!_stop) {
      if (!_pause)
        _output_queue.push(std::invoke(_f));
    }
    _output_queue.wake();
  }
};

// Consumer thread
template <template <typename...> class Queue, typename Input, typename Callable>
struct thread_worker<Queue, Input, Callable,                           //
    std::enable_if_t<!util::is_instance_of_v<Input, jtc::type_list>>,  //
    std::enable_if_t<std::is_same_v<std::invoke_result_t<Callable, Input>, void>>> {
  Callable _f;
  Queue<Input>& _input_queue;
  const std::atomic_bool& _stop;

  void operator()() noexcept {
    while (!_stop) {
      auto val = _input_queue.pop_unless([&] { return _stop.load(); });
      if (!val)
        break;
      std::invoke(_f, std::move(*val));
    }
  }
};

// Input+Consumer thread for a consumer-only pipeline
template <template <typename...> class Queue, typename... InputArgs, typename Callable>
struct thread_worker<Queue, jtc::type_list<InputArgs...>, Callable,  //
    std::enable_if_t<sizeof...(InputArgs) != 0>,                     //
    std::enable_if_t<std::is_same_v<std::invoke_result_t<Callable, InputArgs...>, void>>> {
  using input_t = std::tuple<InputArgs...>;

  Callable _f;
  Queue<input_t>& _input_queue;
  const std::atomic_bool& _stop;

  void operator()() noexcept {
    while (!_stop) {
      auto val = _input_queue.pop_unless([&] { return _stop.load(); });
      if (!val)
        break;
      std::apply(_f, std::move(*val));
    }
  }
};

// Normal output/middle thread
template <template <typename...> class Queue, typename Input, typename Callable>
struct thread_worker<Queue, Input, Callable,                           //
    std::enable_if_t<!util::is_instance_of_v<Input, jtc::type_list>>,  //
    std::enable_if_t<!std::is_same_v<std::invoke_result_t<Callable, Input>, void>>> {
  using output_t = std::invoke_result_t<Callable, Input>;

  Callable _f;
  Queue<Input>& _input_queue;
  Queue<output_t>& _output_queue;
  const std::atomic_bool& _stop;

  void operator()() noexcept {
    while (!_stop) {
      auto val = _input_queue.pop_unless([&] { return _stop.load(); });
      if (!val)
        break;
      auto&& res = std::invoke(_f, std::move(*val));
      _output_queue.push(std::move(res));
    }
    _output_queue.wake();
  }
};

//-------------------------------------------------------------------------------------------------
// Composition of input and output interfaces
//-------------------------------------------------------------------------------------------------

template <template <typename...> class Queue, typename InputType>
struct pipeline_input;

// User input
template <template <typename...> class Queue, typename... InputArgs>
struct pipeline_input<Queue, jtc::type_list<InputArgs...>> {
  using storage_t = std::tuple<InputArgs...>;

  void input(InputArgs... args) { _input_queue.push(storage_t(std::move(args)...)); }
  [[nodiscard]] bool input_is_empty() const noexcept { return _input_queue.empty(); }

 protected:
  Queue<storage_t> _input_queue;
};

// Producer
template <template <typename...> class Queue>
struct pipeline_input<Queue, jtc::type_list<>> {
  [[nodiscard]] bool producing() const noexcept { return !_paused; }
  void pause() noexcept { _paused = true; }
  void resume() noexcept { _paused = false; }

 protected:
  std::atomic_bool _paused = false;  // TODO: Have something like C++20's atomic_flag::wait
};

// Regular output
template <template <typename...> class Queue, typename OutputType>
struct pipeline_output {
  [[nodiscard]] bool available() const noexcept { return !_output_queue.empty(); }
  [[nodiscard]] bool empty() const noexcept { return _output_queue.empty(); }

  [[nodiscard]] OutputType wait_get() noexcept { return _output_queue.pop(); }

  [[nodiscard]] std::optional<OutputType> try_get() noexcept {
    if (_output_queue.empty())
      return std::nullopt;
    return _output_queue.pop();
  }

 protected:
  Queue<OutputType> _output_queue;
};

// Consumer
template <template <typename...> class Queue>
struct pipeline_output<Queue, void> {};

//-------------------------------------------------------------------------------------------------
// Pipeline system
//-------------------------------------------------------------------------------------------------

template <template <typename...> class Queue, typename InputTypes, typename... Stages>
struct pipeline;

template <template <typename...> class Queue, typename... InputArgs, typename... Stages>
struct pipeline<Queue, jtc::type_list<InputArgs...>, Stages...> final
    : pipeline_input<Queue, jtc::type_list<InputArgs...>>,
      pipeline_output<Queue, util::pipeline_return_t<jtc::type_list<InputArgs...>, Stages...>> {
  using input_list_t = jtc::type_list<InputArgs...>;
  using pipeline_input_t = pipeline_input<Queue, input_list_t>;
  using pipeline_output_t = pipeline_output<Queue, util::pipeline_return_t<input_list_t, Stages...>>;
  using tuple_t = util::intermediate_stages_tuple_t<Queue, input_list_t, Stages...>;
  inline static constexpr auto N = sizeof...(Stages);

 public:
  pipeline(std::tuple<Stages...>&& stages) {
    try {
      if constexpr (N > 1) {
        init_output_thread(std::move(std::get<N - 1>(stages)));

        if constexpr (N > 2) {
          init_intermediary_threads<1>(stages);
        }
      }

      init_input_thread(std::move(std::get<0>(stages)));
    } catch (...) {
      stop_threads();
      throw;
    }
  }

  pipeline(const pipeline&) = delete;
  pipeline(pipeline&&) = delete;

  ~pipeline() { stop_threads(); }

  template <std::size_t I>
  void init_intermediary_threads(std::tuple<Stages...>& stages) {
    using inputs = util::result_list_t<input_list_t, Stages...>;
    using input_t = jtc::list_get_t<inputs, I - 1>;
    using callables = jtc::type_list<Stages...>;
    using callable_t = jtc::list_get_t<callables, I>;

    _threads[I] = std::thread(thread_worker<Queue, input_t, callable_t>{
        std::move(std::get<I>(stages)),
        std::get<I - 1>(_queues),
        std::get<I>(_queues),
        _stop,
    });

    if constexpr (I < N - 2) {
      init_intermediary_threads<I + 1>(stages);
    }
  }

  template <typename T>
  void init_output_thread(T&& last) {
    using ret_t = util::pipeline_return_t<input_list_t, Stages...>;
    using inputs = util::result_list_t<input_list_t, Stages...>;
    using input_t = jtc::list_get_t<inputs, inputs::size - 1>;
    using callables = jtc::type_list<Stages...>;
    using callable_t = jtc::list_get_t<callables, N - 1>;

    if constexpr (std::is_same_v<ret_t, void>) {
      // Consumer
      _threads[N - 1] = std::thread(thread_worker<Queue, input_t, callable_t>{
          std::forward<T>(last),
          std::get<N - 2>(_queues),
          _stop,
      });
    } else {
      // User output
      _threads[N - 1] = std::thread(thread_worker<Queue, input_t, callable_t>{
          std::forward<T>(last),
          std::get<N - 2>(_queues),
          pipeline_output_t::_output_queue,
          _stop,
      });
    }
  }

  template <typename T>
  void init_input_thread(T&& first) {
    using input_t = input_list_t;
    using callables = jtc::type_list<Stages...>;
    using callable_t = jtc::list_get_t<callables, 0>;

    if constexpr (sizeof...(InputArgs) == 0) {
      // Producer
      if constexpr (N == 1) {
        // Producing directly to output
        _threads[0] = std::thread(thread_worker<Queue, input_t, callable_t>{
            std::forward<T>(first),
            pipeline_output_t::_output_queue,
            pipeline_input_t::_paused,
            _stop,
        });
      } else {
        // Producing to another thread
        _threads[0] = std::thread(thread_worker<Queue, input_t, callable_t>{
            std::forward<T>(first),
            std::get<0>(_queues),
            pipeline_input_t::_paused,
            _stop,
        });
      }
    } else {
      // User input
      if constexpr (N == 1) {
        using ret_t = util::pipeline_return_t<input_list_t, Stages...>;
        if constexpr (std::is_same_v<ret_t, void>) {
          // Consumer-only pipeline
          _threads[0] = std::thread(thread_worker<Queue, input_t, callable_t>{
              std::forward<T>(first),
              pipeline_input_t::_input_queue,
              _stop,
          });
        } else {
          // Feeding directly to output
          _threads[0] = std::thread(thread_worker<Queue, input_t, callable_t>{
              std::forward<T>(first),
              pipeline_input_t::_input_queue,
              pipeline_output_t::_output_queue,
              _stop,
          });
        }
      } else {
        // Feeding to a second thread
        _threads[0] = std::thread(thread_worker<Queue, input_t, callable_t>{
            std::forward<T>(first),
            pipeline_input_t::_input_queue,
            std::get<0>(_queues),
            _stop,
        });
      }
    }
  }

 private:
  std::atomic_bool _stop = false;
  tuple_t _queues;
  std::array<std::thread, N> _threads;

  void stop_threads() {
    // Set the "stop token" flag
    _stop = true;

    // Wake input thread, if it exists
    if constexpr (sizeof...(InputArgs) != 0) {
      pipeline_input_t::_input_queue.wake();
    }

    // Wake all threads waiting for input
    // (needed in case thread fails during construction)
    util::tuple_foreach([](auto& queue) { queue.wake(); }, _queues);

    // Wait for all unfinished threads to exit
    for (auto& thread : _threads)
      if (thread.joinable())
        thread.join();
  }
};

//-------------------------------------------------------------------------------------------------
// Execution policies
//-------------------------------------------------------------------------------------------------

template <template <typename...> class Queue>
struct policy_type {};

template <typename T>
using default_queue_t = util::blocking_queue<T>;

//-------------------------------------------------------------------------------------------------
// Wrapper Types
//-------------------------------------------------------------------------------------------------

template <template <typename...> class Wrapper>
struct wrapper_type {};

template <typename...>
struct null_wrapper {};

//-------------------------------------------------------------------------------------------------
// Output types
//-------------------------------------------------------------------------------------------------

template <typename OutputType, template <typename...> class Queue, template <typename...> class Wrapper>
struct output_tagged {
  OutputType _data;
};

template <typename OutputType, template <typename...> class Queue>
struct output_with_policy {
  OutputType _data;

  template <template <typename...> class Wrapper>
  [[nodiscard]] constexpr auto operator/(wrapper_type<Wrapper>) &&  //
      noexcept(std::is_nothrow_move_constructible_v<OutputType>) {
    return output_tagged<OutputType, Queue, Wrapper>{std::move(_data)};
  }
};

struct end_type {
  template <template <typename...> class Queue>
  [[nodiscard]] constexpr auto operator/(policy_type<Queue>) const noexcept {
    return output_with_policy<end_type, Queue>{};
  }

  template <template <typename...> class Wrapper>
  [[nodiscard]] constexpr auto operator/(wrapper_type<Wrapper>) const noexcept {
    return output_tagged<end_type, default_queue_t, Wrapper>{};
  }
};

template <typename F>
struct consumer {
  static_assert(std::is_move_constructible_v<F>);

  F _f;

  template <template <typename...> class Queue>
  [[nodiscard]] constexpr auto operator/(policy_type<Queue>) && noexcept(std::is_nothrow_move_constructible_v<F>) {
    return output_with_policy<consumer, Queue>{std::move(*this)};
  }

  template <template <typename...> class Wrapper>
  [[nodiscard]] constexpr auto operator/(wrapper_type<Wrapper>) && noexcept(std::is_nothrow_move_constructible_v<F>) {
    return output_tagged<consumer, default_queue_t, Wrapper>{std::move(*this)};
  }
};

template <typename F>
consumer(F) -> consumer<std::decay_t<F>>;

//-------------------------------------------------------------------------------------------------
// Construction (intermediary) types
//-------------------------------------------------------------------------------------------------

template <typename ParameterTypeList, typename... Stages>
struct partial_pipeline;

template <typename... InputArgs, typename... Stages>
struct partial_pipeline<jtc::type_list<InputArgs...>, Stages...> {
  partial_pipeline(std::tuple<Stages...>&& stages)  //
      noexcept(std::is_nothrow_move_constructible_v<std::tuple<Stages...>>)
      : _stages{std::move(stages)} {}
  partial_pipeline(const partial_pipeline&) = delete;
  partial_pipeline(partial_pipeline&&) = delete;

  std::tuple<Stages...> _stages;

  template <template <typename...> class Queue = default_queue_t, template <typename...> class Wrapper = null_wrapper>
  [[nodiscard]] auto operator>>(end_type) && {
    using pipeline_t = pipeline<Queue, jtc::type_list<InputArgs...>, Stages...>;

    if constexpr (util::is_same_template_v<Wrapper, null_wrapper>) {
      return pipeline_t{
          std::move(_stages),
      };
    } else {
      return Wrapper<pipeline_t>{
          new pipeline_t{
              std::move(_stages),
          },
      };
    }
  }

  template <template <typename...> class Queue = default_queue_t,  //
      template <typename...> class Wrapper = null_wrapper,         //
      typename F>
  [[nodiscard]] auto operator>>(consumer<F>&& s) && {
    using F_ = std::decay_t<F>;
    using arg_t = tdp::util::pipeline_return_t<jtc::type_list<InputArgs...>, Stages...>;
    static_assert(std::is_invocable_v<F_, arg_t>, "The consumer can't be called with the pipeline stage's output");
    static_assert(std::is_same_v<std::invoke_result_t<F_, arg_t>, void>, "A consumer must return void.");

    using pipeline_t = pipeline<Queue, jtc::type_list<InputArgs...>, Stages..., F>;

    if constexpr (util::is_same_template_v<Wrapper, null_wrapper>) {
      return pipeline_t{
          util::tuple_append(std::move(_stages), std::move(s._f)),
      };
    } else {
      return Wrapper<pipeline_t>{
          new pipeline_t{
              util::tuple_append(std::move(_stages), std::move(s._f)),
          },
      };
    }
  }

  template <typename OutputType, template <typename...> class Queue>
  [[nodiscard]] auto operator>>(output_with_policy<OutputType, Queue>&& output) &&  //
      noexcept(util::are_nothrow_move_constructible_v<OutputType, Stages...>) {
    return std::move(*this).template operator>><Queue>(std::move(output._data));
  }

  template <typename OutputType, template <typename...> class Queue, template <typename...> class Wrapper>
  [[nodiscard]] auto operator>>(output_tagged<OutputType, Queue, Wrapper>&& output) &&  //
      noexcept(util::are_nothrow_move_constructible_v<OutputType, Stages...>) {
    return std::move(*this).template operator>><Queue, Wrapper>(std::move(output._data));
  }

  template <typename F>
  [[nodiscard]] constexpr auto operator>>(F&& f) &&  //
      noexcept(util::are_nothrow_move_constructible_v<F, Stages...>) {
    using F_ = std::decay_t<F>;
    static_assert(std::is_move_constructible_v<F_>);
    using arg_t = tdp::util::pipeline_return_t<jtc::type_list<InputArgs...>, Stages...>;
    static_assert(std::is_invocable_v<F_, arg_t>, "The new stage must be callable with the current pipeline output");

    using ret_t = std::invoke_result_t<F_, arg_t>;
    static_assert(!std::is_reference_v<ret_t>, "Pipeline stages can't return references");
    static_assert(!std::is_same_v<ret_t, void>, "To return void, use consumer threads.");

    return partial_pipeline<jtc::type_list<InputArgs...>, Stages..., F_>{
        {tdp::util::tuple_append(std::move(_stages), std::forward<F>(f))},
    };
  }
};

//-------------------------------------------------------------------------------------------------
// Input types
//-------------------------------------------------------------------------------------------------

template <typename... InputArgs>
struct input_type {
  static_assert(sizeof...(InputArgs) > 0, "A pipeline must have input arguments. For one without them, use producers.");
  static_assert(!(std::is_reference_v<InputArgs> || ...), "The input parameters of a pipeline can't be references.");

  constexpr input_type() noexcept {}
  input_type(const input_type&) = delete;
  input_type(input_type&&) = delete;

  template <typename F>
  [[nodiscard]] constexpr auto operator>>(F&& f) const noexcept(std::is_nothrow_move_constructible_v<F>) {
    using F_ = std::decay_t<F>;
    static_assert(std::is_move_constructible_v<F_>);

    static_assert(std::is_invocable_v<F_, InputArgs...>, "The pipeline stage must be callable with the input.");

    using ret_t = std::invoke_result_t<F_, InputArgs...>;
    static_assert(!std::is_reference_v<ret_t>, "Pipeline stages can't return references");
    static_assert(!std::is_same_v<ret_t, void>, "To return void, use consumer threads.");

    return partial_pipeline<jtc::type_list<InputArgs...>, F_>{
        {std::forward<F>(f)},
    };
  }

  template <template <typename...> class Queue = default_queue_t,  //
      template <typename...> class Wrapper = null_wrapper,         //
      typename Fc>
  [[nodiscard]] constexpr auto operator>>(consumer<Fc>&& c) const {
    static_assert(std::is_invocable_v<Fc, InputArgs...>, "The consumer must be callable with the input.");

    using ret_t = std::invoke_result_t<Fc, InputArgs...>;
    static_assert(std::is_same_v<ret_t, void>, "A consumer must return void.");

    using pipeline_t = pipeline<default_queue_t, jtc::type_list<InputArgs...>, Fc>;

    if constexpr (util::is_same_template_v<Wrapper, null_wrapper>) {
      return pipeline_t{
          std::tuple<Fc>{std::move(c._f)},
      };
    } else {
      return Wrapper<pipeline_t>{
          new pipeline_t{
              std::tuple<Fc>{std::move(c._f)},
          },
      };
    }
  }

  template <typename OutputType, template <typename...> class Queue>
  [[nodiscard]] auto operator>>(output_with_policy<OutputType, Queue>&& output) const {
    return std::move(*this).template operator>><Queue>(std::move(output._data));
  }

  template <typename OutputType, template <typename...> class Queue, template <typename...> class Wrapper>
  [[nodiscard]] auto operator>>(output_tagged<OutputType, Queue, Wrapper>&& output) const {
    return std::move(*this).template operator>><Queue, Wrapper>(std::move(output._data));
  }
};

template <typename F>
struct producer {
  static_assert(std::is_move_constructible_v<F>);

  F _f;

  static_assert(std::is_invocable_v<F>, "A producer thread must be invocable without parameters");

  using produced_t = std::invoke_result_t<F>;
  static_assert(!std::is_same_v<produced_t, void>, "A producer can't return void.");
  static_assert(!std::is_reference_v<produced_t>, "A producer's return type can't be a reference.");

  template <typename Fc>
  [[nodiscard]] constexpr auto operator>>(Fc&& f) && noexcept(util::are_nothrow_move_constructible_v<F, Fc>) {
    using F_ = std::decay_t<Fc>;
    static_assert(std::is_move_constructible_v<F_>);
    static_assert(std::is_invocable_v<F_, produced_t>, "The new stage must be callable with the producer's output");

    using ret_t = std::invoke_result_t<F_, produced_t>;
    static_assert(!std::is_reference_v<ret_t>, "Pipeline stages can't return references");
    static_assert(!std::is_same_v<ret_t, void>, "To return void, use consumer threads.");

    return partial_pipeline<jtc::type_list<>, F, F_>{
        {std::move(_f), std::forward<Fc>(f)},
    };
  }

  template <template <typename...> class Queue = default_queue_t,  //
      template <typename...> class Wrapper = null_wrapper,         //
      typename Fc>
  [[nodiscard]] constexpr auto operator>>(consumer<Fc>&& c) && {
    static_assert(std::is_invocable_v<Fc, produced_t>, "The consumer must be callable with the producer's output");

    using ret_t = std::invoke_result_t<Fc, produced_t>;
    static_assert(std::is_same_v<ret_t, void>, "A consumer must return void.");

    using pipeline_t = pipeline<default_queue_t, jtc::type_list<>, F, Fc>;

    if constexpr (util::is_same_template_v<Wrapper, null_wrapper>) {
      return pipeline_t{
          std::tuple<F, Fc>{std::move(_f), std::move(c._f)},
      };
    } else {
      return Wrapper<pipeline_t>{
          new pipeline_t{
              std::tuple<F, Fc>{std::move(_f), std::move(c._f)},
          },
      };
    }
  }

  template <template <typename...> class Queue = default_queue_t,  //
      template <typename...> class Wrapper = null_wrapper>
  [[nodiscard]] constexpr auto operator>>(end_type) && {
    using pipeline_t = pipeline<default_queue_t, jtc::type_list<>, F>;

    if constexpr (util::is_same_template_v<Wrapper, null_wrapper>) {
      return pipeline_t{
          std::tuple<F>{std::move(_f)},
      };
    } else {
      return Wrapper<pipeline_t>{
          new pipeline_t{
              std::tuple<F>{std::move(_f)},
          },
      };
    }
  }

  template <typename OutputType, template <typename...> class Queue>
  [[nodiscard]] auto operator>>(output_with_policy<OutputType, Queue>&& output) && {
    return std::move(*this).template operator>><Queue>(std::move(output._data));
  }

  template <typename OutputType, template <typename...> class Queue, template <typename...> class Wrapper>
  [[nodiscard]] auto operator>>(output_tagged<OutputType, Queue, Wrapper>&& output) && {
    return std::move(*this).template operator>><Queue, Wrapper>(std::move(output._data));
  }
};

template <typename F>
producer(F) -> producer<std::decay_t<F>>;

}  // namespace tdp::detail

#endif