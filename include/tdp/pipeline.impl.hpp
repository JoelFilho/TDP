#ifndef TDP_PIPELINE_IMPL_HPP
#define TDP_PIPELINE_IMPL_HPP

#include <array>
#include <atomic>
#include <thread>
#include <tuple>
#include <type_traits>

#include "util/blocking_queue.hpp"
#include "util/blocking_triple_buffer.hpp"
#include "util/helpers.hpp"
#include "util/type_list.hpp"

namespace tdp::detail {

//-------------------------------------------------------------------------------------------------
// Processing threads
//-------------------------------------------------------------------------------------------------

template <template <typename...> class Queue, typename Input, typename Callable, typename = void>
struct thread_worker;

// Normal input
template <template <typename...> class Queue, typename... InputArgs, typename Callable>
struct thread_worker<Queue, jtc::type_list<InputArgs...>, Callable, void> {
  using input_t = std::tuple<InputArgs...>;
  using output_t = std::invoke_result_t<Callable, InputArgs...>;

  Callable _f;
  Queue<input_t>& _input_queue;
  Queue<output_t>& _output_queue;
  const std::atomic_bool& _stop;

  void operator()() {
    while (!_stop) {
      auto val = _input_queue.pop_unless([&] { return _stop.load(); });
      if (!val)
        break;
      auto res = std::apply(_f, std::move(*val));
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

  void operator()() {
    while (!_stop) {
      if (!_pause)
        _output_queue.push(_f());
    }
    _output_queue.wake();
  }
};

// Consumer thread
template <template <typename...> class Queue, typename Input, typename Callable>
struct thread_worker<Queue, Input, Callable,
    std::enable_if_t<std::is_same_v<std::invoke_result_t<Callable, Input>, void>>> {
  Callable _f;
  Queue<Input>& _input_queue;
  const std::atomic_bool& _stop;

  void operator()() {
    while (!_stop) {
      auto val = _input_queue.pop_unless([&] { return _stop.load(); });
      if (!val)
        break;
      _f(std::move(*val));
    }
  }
};

// Normal output/middle thread
template <template <typename...> class Queue, typename Input, typename Callable>
struct thread_worker<Queue, Input, Callable,
    std::enable_if_t<!std::is_same_v<std::invoke_result_t<Callable, Input>, void>>> {
  using output_t = std::invoke_result_t<Callable, Input>;

  Callable _f;
  Queue<Input>& _input_queue;
  Queue<output_t>& _output_queue;
  const std::atomic_bool& _stop;

  void operator()() {
    while (!_stop) {
      auto val = _input_queue.pop_unless([&] { return _stop.load(); });
      if (!val)
        break;
      auto res = _f(std::move(*val));
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

 protected:
  Queue<storage_t> _input_queue;
};

// Producer
template <template <typename...> class Queue>
struct pipeline_input<Queue, jtc::type_list<>> {
  bool running() const noexcept { return !_paused; }
  void pause() noexcept { _paused = true; }
  void resume() noexcept { _paused = false; }

 protected:
  std::atomic_bool _paused = false;  // TODO: Have something like C++20's atomic_flag::wait
};

// Regular output
template <template <typename...> class Queue, typename OutputType>
struct pipeline_output {
  bool available() const noexcept { return !_output_queue.empty(); }

  OutputType get() noexcept { return _output_queue.pop(); }

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
struct pipeline<Queue, jtc::type_list<InputArgs...>, Stages...>
    : pipeline_input<Queue, jtc::type_list<InputArgs...>>,
      pipeline_output<Queue, util::pipeline_return_t<jtc::type_list<InputArgs...>, Stages...>> {
  using input_list_t = jtc::type_list<InputArgs...>;
  using pipeline_input_t = pipeline_input<Queue, input_list_t>;
  using pipeline_output_t = pipeline_output<Queue, util::pipeline_return_t<input_list_t, Stages...>>;
  using tuple_t = util::intermediate_stages_tuple_t<Queue, input_list_t, Stages...>;
  inline static constexpr auto N = sizeof...(Stages);

 public:
  pipeline(std::tuple<Stages...>&& stages) {
    if constexpr (N > 1) {
      init_output_thread(std::move(std::get<N - 1>(stages)));

      if constexpr (N > 2) {
        init_intermediary_threads<1>(stages);
      }
    }

    init_input_thread(std::move(std::get<0>(stages)));
  }

  pipeline(const pipeline&) = delete;
  pipeline(pipeline&&) = default;
  ~pipeline() {
    _stop = true;
    if constexpr (sizeof...(InputArgs) != 0) {
      pipeline_input_t::_input_queue.wake();
    }
    for (auto& t : _threads)
      t.join();
  }

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
      _threads[0] = std::thread(thread_worker<Queue, input_t, callable_t>{
          std::forward<T>(first),
          pipeline_input_t::_input_queue,
          std::get<0>(_queues),
          _stop,
      });
    }
  }

 private:
  std::atomic_bool _stop = false;
  tuple_t _queues;
  std::array<std::thread, N> _threads;
};

//-------------------------------------------------------------------------------------------------
// Output types
//-------------------------------------------------------------------------------------------------

struct end_type {
  end_type(const end_type&) = delete;
  end_type(end_type&&) = delete;
};

template <typename F>
struct consumer {
  F _f;
};

template <typename F>
consumer(F) -> consumer<std::decay_t<F>>;

//-------------------------------------------------------------------------------------------------
// Construction (intermediary) types
//-------------------------------------------------------------------------------------------------

template <template <typename...> class Queue, typename ParameterTypeList, typename... Stages>
struct partial_pipeline;

template <template <typename...> class Queue, typename... InputArgs, typename... Stages>
struct partial_pipeline<Queue, jtc::type_list<InputArgs...>, Stages...> {
  partial_pipeline(const partial_pipeline&) = delete;
  partial_pipeline(partial_pipeline&&) = delete;

  std::tuple<Stages...> _stages;

  auto operator>>(const end_type&) && noexcept {
    return pipeline<Queue, jtc::type_list<InputArgs...>, Stages...>{
        std::move(_stages),
    };
  }

  template <typename F>
  auto operator>>(consumer<F>&& s) && noexcept {
    using F_ = std::decay_t<F>;
    using arg_t = tdp::util::pipeline_return_t<jtc::type_list<InputArgs...>, Stages...>;
    static_assert(std::is_invocable_v<F_, arg_t>);
    static_assert(std::is_same_v<std::invoke_result_t<F_, arg_t>, void>);

    return pipeline<Queue, jtc::type_list<InputArgs...>, Stages..., F>{
        util::tuple_append(std::move(_stages), std::move(s._f)),
    };
  }

  template <typename F>
  constexpr auto operator>>(F&& f) && noexcept {
    using F_ = std::decay_t<F>;
    using arg_t = tdp::util::pipeline_return_t<jtc::type_list<InputArgs...>, Stages...>;
    static_assert(std::is_invocable_v<F_, arg_t>);

    return partial_pipeline<Queue, jtc::type_list<InputArgs...>, Stages..., F_>{
        {tdp::util::tuple_append(std::move(_stages), std::forward<F>(f))},
    };
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
// Input types
//-------------------------------------------------------------------------------------------------

template <template <typename...> class Queue, typename... InputArgs>
struct input_type_base {
  static_assert(sizeof...(InputArgs) > 0);

  template <typename F>
  constexpr auto operator>>(F&& f) const noexcept {
    using F_ = std::decay_t<F>;

    static_assert(std::is_invocable_v<F_, InputArgs...>);

    return partial_pipeline<Queue, jtc::type_list<InputArgs...>, F_>{
        {std::forward<F>(f)},
    };
  }
};

template <template <typename...> class Queue, typename... InputArgs>
struct input_type_tagged : input_type_base<Queue, InputArgs...> {
  constexpr input_type_tagged() noexcept {}
  input_type_tagged(const input_type_tagged&) = delete;
  input_type_tagged(input_type_tagged&&) = delete;
};

template <typename... InputArgs>
struct input_type : input_type_base<default_queue_t, InputArgs...> {
  constexpr input_type() noexcept {}
  input_type(const input_type&) = delete;
  input_type(input_type&&) = delete;

  template <template <typename...> class Queue>
  constexpr auto operator()(const policy_type<Queue>&) const noexcept {
    return input_type_tagged<Queue, InputArgs...>{};
  }
};

template <template <typename...> class Queue, typename F>
struct producer_base {
  F _f;

  static_assert(std::is_invocable_v<F>);
  static_assert(!std::is_same_v<std::invoke_result_t<F>, void>);

  template <typename Fc>
  constexpr auto operator>>(Fc&& f) && noexcept {
    using F_ = std::decay_t<Fc>;
    static_assert(std::is_invocable_v<F_, std::invoke_result_t<F>>);

    return partial_pipeline<Queue, jtc::type_list<>, F, F_>{
        {std::move(_f), std::forward<Fc>(f)},
    };
  }

  template <typename Fc>
  constexpr auto operator>>(consumer<Fc>&& c) && noexcept {
    static_assert(std::is_invocable_v<Fc, std::invoke_result_t<F>>);
    return pipeline<Queue, jtc::type_list<>, F, Fc>{
        std::tuple<F, Fc>{std::move(_f), std::move(c._f)},
    };
  }

  constexpr auto operator>>(const end_type&) && noexcept {
    return pipeline<Queue, jtc::type_list<>, F>{
        std::tuple<F>{std::move(_f)},
    };
  }
};

template <typename F>
struct producer : producer_base<default_queue_t, F> {
  template <template <typename...> class Queue>
  constexpr auto operator()(const policy_type<Queue>&) && noexcept {
    return producer_base<Queue, F>{std::move(producer_base<default_queue_t, F>::_f)};
  }
};

template <typename F>
producer(F) -> producer<std::decay_t<F>>;

}  // namespace tdp::detail

#endif