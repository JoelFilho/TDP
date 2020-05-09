// Joel's Template Collection (JTC) - https://github.com/JoelFilho/JTC
// type_list.hpp: A list of types and operations.

// Copyright Joel P. C. Filho 2019 - 2020
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE.md or copy at https://www.boost.org/LICENSE_1_0.txt)

#ifndef JTC_TEMPLATES_TYPE_LIST_HPP
#define JTC_TEMPLATES_TYPE_LIST_HPP

#include <type_traits>
#include <cstddef>

namespace jtc {

//-------------------------------------------------------------------------------------------------
// Imported from JTC's make_type.hpp
//-------------------------------------------------------------------------------------------------
template<typename T> struct make_type { using type = T; };

//-------------------------------------------------------------------------------------------------
// Imported content from JTC's result_type.hpp
//-------------------------------------------------------------------------------------------------

/// A type for compile-time search query results.
/// See the specializations for details.
/// For usage examples, see 'type_list' and 'type_map' search implementations.
template <bool B, typename T = void>
struct result_type;

/// Result type specialization for unsuccesful queries
/// Member 'value' indicates the unsuccessful result.
/// Trying to access the 'type' member causes a substitution error and can be used in SFINAE.
template <typename T>
struct result_type<false, T> {
  constexpr static bool value = false;
};

/// "Found" result type specialization.
/// Member 'value' indicates it was found.
/// Member 'type' indicates the type found.
template <typename T>
struct result_type<true, T> {
  constexpr static bool value = true;
  using type = T;
};

//-------------------------------------------------------------------------------------------------
// type_list definition
//
// Usage:
//     using my_list = type_list<char, short, int>;
//     static_assert(my_list::size == 3, "List size should be 3");
//-------------------------------------------------------------------------------------------------

template <typename... Types>
struct type_list { constexpr static auto size = sizeof...(Types); };

//-------------------------------------------------------------------------------------------------
// Item Indexing
//
// Gets the nth type in the type_list, indexed from left to right.
//
// Usage:
//     using my_list = type_list<char, short, int>;
//     using found_type = list_get_t<my_list, 0>;
//     static_assert(std::is_same_v<found_type, char>, "Should be char.");
//-------------------------------------------------------------------------------------------------

template <typename List, std::size_t Index>
struct list_get;

template <typename List, std::size_t Index>
using list_get_t = typename list_get<List, Index>::type;

// Recursive implementation: Decrement index until it reaches 0
template <typename Node, typename... Nodes, std::size_t Index>
struct list_get<type_list<Node, Nodes...>, Index>
    : std::conditional<Index == 0, Node, list_get_t<type_list<Nodes...>, Index - 1>> {};

// Recursion stop condition: Index == 0 and there's at least one item in the list
template <typename Node, typename... Nodes>
struct list_get<type_list<Node, Nodes...>, 0> : make_type<Node> {};

// Error case: Index outside range (empty list)
template <std::size_t Index>
struct list_get<type_list<>, Index> { static_assert(Index < 0, "Index out of Bounds."); };

//-------------------------------------------------------------------------------------------------
// Item Search
//
// Returns a result_type that indicates the first match where std::is_same_v<Node, Query>() == true
//
// Usage:
//     using my_list = type_list<char, short, int>;
//     static_assert(list_has_type<my_list, int>(), "List should have 'int'");
//     using my_result = list_find_t<my_list, int>;
//     static_assert(my_result::value, "Int should have been found");
//     typename my_result::type x = 1; // int x = 1;
//-------------------------------------------------------------------------------------------------

// Forward declaration of the implementation
namespace detail { template <typename List, typename Query> struct list_find_impl; }

/// Tries finding the Query type in the List using 'is_same'
template <typename List, typename Query>
struct list_find : detail::list_find_impl<List, Query> {};

template <typename List, typename Query>
using list_find_t = typename list_find<List, Query>::type;

/// Helper function that returns whether List contains the type Query
template <typename List, typename Query>
inline constexpr bool list_has_type() { return list_find<List, Query>::value; }

//-------------------------------------------------------------------------------------------------
// Item Search by Operator
//
// Returns a result_type that indicates the first match where Operator<Node>::value == true
//
// Usage:
//     using my_list = type_list<char, short, int>;
//     template<typename T> using is_big_enough = bool_constant<(sizeof(T) > 1)>;
//     static_assert(list_has_match<my_list, is_big_enough>(), "Weird compiler detected?");
//     using big_enough_type = list_find_where_t<my_list, is_big_enough>;
//     big_enough_type val = 256; // Possibly short val = 256;
//-------------------------------------------------------------------------------------------------

// Forward declaration of the implementation
namespace detail { template <typename List, template <typename...> class BoolOperator> struct list_find_where_impl; }

template <typename List, template <typename...> class BoolOperator>
struct list_find_where : detail::list_find_where_impl<List, BoolOperator> {};

template <typename List, template <typename...> class BoolOperator>
using list_find_where_t = typename list_find_where<List, BoolOperator>::type;

template <typename List, template <typename...> class BoolOperator>
inline constexpr bool list_has_match() { return list_find_where<List, BoolOperator>::value; }

//-------------------------------------------------------------------------------------------------
// Transform (Map)
//
// Creates a new list by applying Operator to each member of List
//
// Usage:
//     using my_list = type_list<true_type, false_type>;
//     template <typename Bool> using not_op = bool_constant<!Bool::value>;
//     using other_list = list_transform_t<my_list, not_op>;
//     static_assert(std::is_same_v<list_get_t<other_list, 0>, false_type>(), "...");
//-------------------------------------------------------------------------------------------------

template <typename List, template <typename...> class Operator>
struct list_transform;

template <typename List, template <typename...> class Operator>
using list_transform_t = typename list_transform<List, Operator>::type;

template <typename... Nodes, template <typename...> class Operator>
struct list_transform<type_list<Nodes...>, Operator> : make_type<type_list<typename Operator<Nodes>::type...>> {};

//-------------------------------------------------------------------------------------------------
// Reduction Operations : Accumulate
//
// Applies reduction of List using InitialValue, from left to right,
//   using binary operator template Operator.
//
// Example of template expansion:
//    list_accumulate_t<T1, T2, T3, bin_op, InitialValue> becomes:
//    bin_op<bin_op<bin_op<InitialValue, T1>, T2>, T3>;
//
// For usage examples of reduction operations, see the n-ary operators in bool_operations.hpp.
//-------------------------------------------------------------------------------------------------

template <typename List, template <typename...> class Operator, typename InitialValue>
struct list_accumulate;

template <typename List, template <typename...> class Operator, typename InitialValue>
using list_accumulate_t = typename list_accumulate<List, Operator, InitialValue>::type;

template <typename Node, typename... Nodes, template <typename...> class Operator, typename InitialValue>
struct list_accumulate<type_list<Node, Nodes...>, Operator, InitialValue>
    : list_accumulate<type_list<Nodes...>, Operator, typename Operator<InitialValue, Node>::type> {};

template <template <typename...> class Operator, typename InitialValue>
struct list_accumulate<type_list<>, Operator, InitialValue> : make_type<InitialValue> {};

//-------------------------------------------------------------------------------------------------
// Reduction Operations : Reduce (left fold)
//
// Applies reduction of List using list_accumulate, but with the first list node as InitialValue.
//-------------------------------------------------------------------------------------------------

template <typename List, template <typename...> class Operator>
struct list_reduce;

template <typename List, template <typename...> class Operator>
using list_reduce_t = typename list_reduce<List, Operator>::type;

template <typename Node, typename... Nodes, template <typename...> class Operator>
struct list_reduce<type_list<Node, Nodes...>, Operator> : list_accumulate<type_list<Nodes...>, Operator, Node> {};

//-------------------------------------------------------------------------------------------------
// Reduction Operations : Right Fold Reduce
//
// Similar to list_reduce, but the operator is applied left-to-right:
//    list_reduce_right_t<T0, T1, T2, T3, bin_op> becomes:
//    bin_op<T0, bin_op<T1, bin_op<T2,T3>>>
//-------------------------------------------------------------------------------------------------

template <typename List, template <typename...> class Operator>
struct list_reduce_right;

template <typename List, template <typename...> class Operator>
using list_reduce_right_t = typename list_reduce_right<List, Operator>::type;

template <typename Node, typename... Nodes, template <typename...> class Operator>
struct list_reduce_right<type_list<Node, Nodes...>, Operator>
    : make_type<typename Operator<Node, list_reduce_right_t<type_list<Nodes...>, Operator>>::type> {};

template <typename Node, template <typename...> class Operator>
struct list_reduce_right<type_list<Node>, Operator> : make_type<Node> {};

//-------------------------------------------------------------------------------------------------
// Concatenate Lists
//
// Concatenate any number of type_list's into a single type list.
//
// Usage example:
//     using list1 = type_list<T0, T1, T2>;
//     using list2 = type_list<T3, T4>;
//     using list3 = type_list<T5, T6>;
//     using list123 = list_concat_t<list1, list2, list3>;
//     (List123 becomes type_list<T0, T1, T2, T3, T4, T5, T6>)
//-------------------------------------------------------------------------------------------------

template <typename... Lists>
struct list_concat;

template <typename... Lists>
using list_concat_t = typename list_concat<Lists...>::type;

template <typename... NodesA, typename... NodesB, typename... Lists>
struct list_concat<type_list<NodesA...>, type_list<NodesB...>, Lists...>
    : list_concat<type_list<NodesA..., NodesB...>, Lists...> {};

template <typename... Nodes>
struct list_concat<type_list<Nodes...>> : make_type<type_list<Nodes...>> {};

//-------------------------------------------------------------------------------------------------
// Implementation of List Search Algorithms, O(n), left-to-right linear search.
//-------------------------------------------------------------------------------------------------

namespace detail {

//-------------------------------------------------------------------------------------------------
// list_find Implementation
//
// TODO: implement using list_find_where and a custom is_same_as operator.
//-------------------------------------------------------------------------------------------------

// Recursive implementation:
template <typename Query, typename Node, typename... Nodes>
struct find_param_type : std::conditional_t<std::is_same_v<Query, Node>,              // Has the type been found?
                                       result_type<true, Node>,               // If yes, return the result.
                                       find_param_type<Query, Nodes...>> {};  // Otherwise, continue searching.

// Recursion stop condition: Single item in the list determines whether it was found by itself.
template <typename Query, typename Node>
struct find_param_type<Query, Node> : result_type<std::is_same_v<Query, Node>, Node> {};

/// Implementation specialization for type_list<Nodes...>.
/// "Calls" find_param_type to start search.
template <typename... Nodes, typename Query>
struct list_find_impl<type_list<Nodes...>, Query> : find_param_type<Query, Nodes...> {};

/// Special case of the implementation: empty list -> not found
template <typename Query>
struct list_find_impl<type_list<>, Query> : result_type<false, Query> {};

//-------------------------------------------------------------------------------------------------
// list_find_where Implementation
// Follows the same algorithm as list_find, except it applies the custom BoolOperator.
//-------------------------------------------------------------------------------------------------

template <template <typename...> class BoolOperator, typename Node, typename... Nodes>
struct find_param_operator : std::conditional_t<BoolOperator<Node>::value,                        // Does Node match the operator?
                                           result_type<true, Node>,                          // If so, return the result.
                                           find_param_operator<BoolOperator, Nodes...>> {};  // Otherwise, continue searching.

template <template <typename...> class BoolOperator, typename Node>
struct find_param_operator<BoolOperator, Node> : result_type<BoolOperator<Node>::value, Node> {};

template <typename... Nodes, template <typename...> class BoolOperator>
struct list_find_where_impl<type_list<Nodes...>, BoolOperator> : find_param_operator<BoolOperator, Nodes...> {};

template <template <typename...> class BoolOperator>
struct list_find_where_impl<type_list<>, BoolOperator> : result_type<false> {};

}  // namespace detail
}  // namespace jtc

#endif