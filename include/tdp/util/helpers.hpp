#ifndef TDP_HELPERS_HPP
#define TDP_HELPERS_HPP

#include <tuple>
#include <type_traits>

#include "type_list.hpp"

namespace tdp::util {

namespace detail {

/// Implementation of tuple_append
template <typename... Ts, typename T, std::size_t... Is>
constexpr auto tuple_append_impl(std::tuple<Ts...>&& tuple, T&& value, std::index_sequence<Is...>)
    -> std::tuple<Ts..., std::decay_t<T>> {
  return {std::move(std::get<Is>(tuple))..., std::forward<T>(value)};
}

// Hidden implementation for pipeline_return_t
template <typename Input, typename... Callables>
struct pipeline_return;

template <typename... InputArgs, typename Callable, typename... Callables>
struct pipeline_return<jtc::type_list<InputArgs...>, Callable, Callables...>
    : pipeline_return<jtc::type_list<std::invoke_result_t<Callable, InputArgs...>>, Callables...> {};

template <typename... InputArgs, typename Callable>
struct pipeline_return<jtc::type_list<InputArgs...>, Callable> {
  using type = std::invoke_result_t<Callable, InputArgs...>;
};

// Hidden implementation for intermediate_stages_tuple_t

template <typename Input, typename... Callables>
struct result_list;

template <typename Input, typename... Callables>
using result_list_t = typename result_list<Input, Callables...>::type;

template <typename... InputArgs, typename Callable>
struct result_list<jtc::type_list<InputArgs...>, Callable> {
  using type = jtc::type_list<>;
};

template <typename... InputArgs, typename Callable, typename... Callables>
struct result_list<jtc::type_list<InputArgs...>, Callable, Callables...> {
  using first_t = std::invoke_result_t<Callable, InputArgs...>;
  using type = jtc::list_concat_t<jtc::type_list<first_t>, result_list_t<first_t, Callables...>>;
};

template <typename Input, typename Callable, typename... Callables>
struct result_list<Input, Callable, Callables...> {
  using res_t = std::invoke_result_t<Callable, Input>;
  using type = jtc::list_concat_t<jtc::type_list<res_t>, result_list_t<res_t, Callables...>>;
};

template <typename Input, typename Callable>
struct result_list<Input, Callable> {
  using type = jtc::type_list<>;
};

template <template <typename...> typename Queue, typename ResultList>
struct tuple_instance;

template <template <typename...> typename Queue, typename... Results>
struct tuple_instance<Queue, jtc::type_list<Results...>> {
  using type = std::tuple<Queue<Results>...>;
};

template <template <typename...> typename Queue, typename Input, typename... Callables>
struct intermediate_stages_tuple {
  using type = typename tuple_instance<Queue, result_list_t<Input, Callables...>>::type;
};

}  // namespace detail

//---------------------------------------------------------------------------------------------------------------------
// tuple_append(std::tuple<...> tuple, value)
//
// Creates a new tuple, appending to the content of 'tuple' the value type of 'value'.
//
// Example:
//
//   std::tuple<int, char> t {10, 'X'};
//   auto t2 = tuple_append(t, 22.0);
//   static_assert(std::is_same_v<decltype(t2), std::tuple<int, char, double>>);
//---------------------------------------------------------------------------------------------------------------------

template <typename... Ts, typename T>
constexpr auto tuple_append(std::tuple<Ts...> tuple, T&& value) {
  return detail::tuple_append_impl(std::move(tuple), std::forward<T>(value), std::index_sequence_for<Ts...>());
}

//---------------------------------------------------------------------------------------------------------------------
// pipeline_return_t<Input, Callables...>
//
// Determines the return type of a series of callables, called left-to-right.
//
// The input must have the type jtc::type_list<InputTypes...>,
// where InputTypes are the types of parameters of the first callable.
//
// pipeline_return_t<Input, Callable1, Callable2>
// is the return of
// Callable2{}(Callable1{}( input... ))
//
// Example:
//
//   auto sum = [] (int x, int y) { return x + y; };
//   auto square [] (int x) { return long long {x} *x; };
//
//   using input_t = jtc::type_list<int,int>;
//   using sum_t = decltype(sum);
//   using square_t = decltype(square);
//
//   using return_t = pipeline_return_t<input_t, sum_t, square_t>;
//   static_assert(std::is_same_v<return_t, long long>);
//
//---------------------------------------------------------------------------------------------------------------------

template <typename Input, typename... Callables>
using pipeline_return_t = typename detail::pipeline_return<Input, Callables...>::type;

//---------------------------------------------------------------------------------------------------------------------
// Returns the tuple type with Queues
//---------------------------------------------------------------------------------------------------------------------

template <template <typename...> typename Queue, typename Input, typename... Callables>
using intermediate_stages_tuple_t = typename detail::intermediate_stages_tuple<Queue, Input, Callables...>::type;

using detail::result_list_t;

//---------------------------------------------------------------------------------------------------------------------
// Determines if a class is a specialization of a template
//
// Due to how C++ works, it will only work with templates that only accept typenames.
//---------------------------------------------------------------------------------------------------------------------

template <typename Typename, template <typename...> typename Template>
struct is_instance_of : std::bool_constant<false> {};

template <template <typename...> typename Template, typename... Args>
struct is_instance_of<Template<Args...>, Template> : std::bool_constant<true> {};

template <typename Typename, template <typename...> typename Template>
inline constexpr bool is_instance_of_v = is_instance_of<Typename, Template>::value;

}  // namespace tdp::util

#endif