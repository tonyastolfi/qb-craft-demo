// Utilities for dealing with tuples.
//
#pragma once

#include <tuple>
#include <utility>

//------------------------------------------------------------------------------
// Helpers and implementation detail for `visit_tuple_element`.
//
namespace detail {

template <int Index, typename Tuple, typename Visitor>
void dispatch_fn_impl(Tuple &&t, Visitor &&v) {
  v(std::get<Index>(std::forward<Tuple>(t)));
}

template <typename Tuple, typename Visitor, int... Indices>
void visit_tuple_element_impl(int i, Tuple &&t, Visitor &&v,
                              std::integer_sequence<int, Indices...>) {

  using dispatch_fn_type = void(Tuple &&, Visitor &&);

  static dispatch_fn_type *const dispatch_[sizeof...(Indices)] = {
      &dispatch_fn_impl<Indices, Tuple, Visitor>...};

  assert(i >= 0);
  assert(i < sizeof...(Ts));

  return dispatch_[i](t, std::forward<Visitor>(v));
}

} // namespace detail
//------------------------------------------------------------------------------

// Invokes `v`, an overloaded callable accepting any of the types in tuple `t`,
// on the i-th element of `t`.
//
template <typename Tuple, typename Visitor /*void(decltype(std::get<N>(t)))*/,
          // Generate a compile-time sequence of tuple indices to generate the
          // dispatch function pointer table (see
          // `detail::visit_tuple_element_impl`).
          typename Indices = std::make_integer_sequence<
              int, std::tuple_size<std::decay_t<Tuple>>::value> //
          >
void visit_tuple_element(int i, Tuple &&t, Visitor &&v) {
  detail::visit_tuple_element_impl(i, std::forward<Tuple>(t),
                                   std::forward<Visitor>(v), Indices{});
}

template <typename T> struct TypeOf { using type = T; };

//------------------------------------------------------------------------------
namespace detail {

template <typename Tuple, int... TailIndices>
auto drop_first_impl(Tuple &&t, std::integer_sequence<int, TailIndices...>) {
  return std::make_tuple(std::get<TailIndices + 1>(std::forward<Tuple>(t))...);
}

} // namespace detail
//------------------------------------------------------------------------------

template <typename Tuple,
          typename TailIndices = std::make_integer_sequence<
              int, std::tuple_size<std::decay_t<Tuple>>::value - 1>>
auto drop_first(Tuple &&t) {
  return detail::drop_first_impl(std::forward<Tuple>(t), TailIndices{});
}

//------------------------------------------------------------------------------
namespace detail {

template <typename Tuple, typename MapFn, int... Indices>
auto tuple_transform_impl(Tuple &&t, MapFn &&fn,
                          std::integer_sequence<int, Indices...>) {
  return std::make_tuple(fn(std::get<Indices>(std::forward<Tuple>(t)))...);
}

} // namespace detail
//------------------------------------------------------------------------------

// Returns a new tuple by applying an element-wise transformation (`fn`) on each
// member of the input tuple `t`; i.e.,
//
//   std::make_tuple(fn(std::get<0>(t)),
//                   fn(std::get<1>(t)),
//                   ...,
//                   fn(std::get<N>(t)))
//
template <typename Tuple,
          typename MapFn /* MappedT_N(decltype(std::get<N>(t))) */,
          typename Indices = std::make_integer_sequence<
              int, std::tuple_size<std::decay_t<Tuple>>::value> //
          >
auto tuple_transform(Tuple &&t, MapFn &&fn) {
  return detail::tuple_transform_impl(std::forward<Tuple>(t),
                                      std::forward<MapFn>(fn), Indices{});
}

//------------------------------------------------------------------------------

// Applies the given function `fn` to each element of the passed tuple, in
// ascending index order.
//
template <typename Tuple, typename Fn> void tuple_for_each(Tuple &&t, Fn &&fn) {
  constexpr auto nelems = std::tuple_size<std::decay_t<Tuple>>::value;
  for (std::size_t i = 0; i < nelems; ++i) {
    visit_tuple_element(i, t, fn);
  }
}

//------------------------------------------------------------------------------

template <typename Fn>
void sequence_for_each(std::integer_sequence<int>, Fn &&) noexcept {}

template <int First, int... Rest, typename Fn>
void sequence_for_each(std::integer_sequence<int, First, Rest...>,
                       Fn &&fn) noexcept {
  fn(std::integral_constant<int, First>{});
  sequence_for_each(std::integer_sequence<int, Rest...>{},
                    std::forward<Fn>(fn));
}

template <int N, typename Fn> void for_each_upto(Fn &&fn) noexcept {
  sequence_for_each(std::make_integer_sequence<int, N>{}, std::forward<Fn>(fn));
}
