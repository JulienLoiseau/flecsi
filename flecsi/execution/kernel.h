/*
    @@@@@@@@  @@           @@@@@@   @@@@@@@@ @@
   /@@/////  /@@          @@////@@ @@////// /@@
   /@@       /@@  @@@@@  @@    // /@@       /@@
   /@@@@@@@  /@@ @@///@@/@@       /@@@@@@@@@/@@
   /@@////   /@@/@@@@@@@/@@       ////////@@/@@
   /@@       /@@/@@//// //@@    @@       /@@/@@
   /@@       @@@//@@@@@@ //@@@@@@  @@@@@@@@ /@@
   //       ///  //////   //////  ////////  //

   Copyright (c) 2016, Los Alamos National Security, LLC
   All rights reserved.
                                                                              */
#pragma once

/*!
  @file
 */

#include <algorithm>

#ifdef FLECSI_ENABLE_KOKKOS

#include <Kokkos_Core.hpp>

namespace flecsi {

/*!
  This function is a wrapper for Kokkos::parallel_for that has been adapted to
  work with FleCSI's topology iterator types. In particular, this function
  invokes a map from the normal kernel index space to the FleCSI index space,
  which may require indirection.
 */

template<typename ITERATOR, typename LAMBDA>
void
parallel_for(ITERATOR iterator, LAMBDA lambda, std::string const & name = "") {

  struct functor_t {

    functor_t(ITERATOR & iterator, LAMBDA & lambda)
      : iterator_(iterator), lambda_(lambda) {}

    KOKKOS_INLINE_FUNCTION void operator()(int i) const {
      lambda_(iterator_[i]);
    } // operator()

  private:
    ITERATOR & iterator_;
    LAMBDA & lambda_;

  }; // struct functor_t

  Kokkos::parallel_for(name, iterator.size(), functor_t{iterator, lambda});

} // parallel_for

template<typename ITERATOR>
struct forall_t {

  forall_t(ITERATOR iterator, std::string const & name = "")
    : iterator_(iterator) {}

  template<typename LAMBDA>
  struct functor_u {

    functor_u(ITERATOR & iterator, LAMBDA & lambda)
      : iterator_(iterator), lambda_(lambda) {}

    KOKKOS_INLINE_FUNCTION void operator()(int i) const {
      lambda_(iterator_[i]);
    } // operator()

  private:
    ITERATOR iterator_;
    LAMBDA lambda_;

  }; // struct functor_u

  template<typename LAMBDA>
  void operator+(LAMBDA lambda) {
    Kokkos::parallel_for(
      "  ", iterator_.size(), functor_u<LAMBDA>{iterator_, lambda});
  } // operator+

private:
  ITERATOR iterator_;

}; // forall_t

#define forall(it, iterator, name)                                             \
  forall_t{iterator, name} + KOKKOS_LAMBDA(auto it)

template<typename ITERATOR, typename LAMBDA, typename REDUCER>
void
parallel_reduce(ITERATOR iterator,
  LAMBDA lambda,
  REDUCER result,
  std::string const & name = "") {

  using value_type = typename REDUCER::value_type;

  struct functor_t {

    functor_t(ITERATOR & iterator, LAMBDA & lambda)
      : iterator_(iterator), lambda_(lambda) {}

    KOKKOS_INLINE_FUNCTION void operator()(int i, value_type & tmp) const {
      lambda_(iterator_[i], tmp);
    } // operator()

  private:
    ITERATOR & iterator_;
    LAMBDA & lambda_;

  }; // struct functor_t

  Kokkos::parallel_reduce(
    name, iterator.size(), functor_t{iterator, lambda}, result);

} // parallel_reduce

template<typename ITERATOR, typename REDUCER>
struct reduceall_t {

  reduceall_t(ITERATOR iterator, REDUCER reducer, std::string const & name = "")
    : iterator_(iterator), reducer_(reducer) {}

  using value_type = typename REDUCER::value_type;

  template<typename LAMBDA>
  struct functor_u {

    functor_u(ITERATOR & iterator, LAMBDA & lambda)
      : iterator_(iterator), lambda_(lambda) {}

    KOKKOS_INLINE_FUNCTION void operator()(int i, value_type & tmp) const {
      lambda_(iterator_[i], tmp);
    } // operator()

  private:
    ITERATOR iterator_;
    LAMBDA lambda_;

  }; // struct functor_u

  template<typename LAMBDA>
  void operator+(LAMBDA lambda) {
    Kokkos::parallel_reduce(
      "  ", iterator_.size(), functor_u<LAMBDA>{iterator_, lambda}, reducer_);
  } // operator+

private:
  ITERATOR iterator_;
  REDUCER reducer_;

}; // forall_t

#define reduceall(it, tmp, iterator, reducer, name)                            \
  reduceall_t{iterator, reducer, name} + KOKKOS_LAMBDA(auto it, auto & tmp)

} // namespace flecsi
#endif

namespace flecsi {

//----------------------------------------------------------------------------//
//! Abstraction function for fine-grained, data-parallel interface.
//!
//! @tparam R range type
//! @tparam FUNCTION    The calleable object type.
//!
//! @param r range over which to execute \a function
//! @param function     The calleable object instance.
//!
//! @ingroup execution
//----------------------------------------------------------------------------//

template<class R, typename FUNCTION>
inline void
for_each_u(R && r, FUNCTION && function) {
  std::for_each(r.begin(), r.end(), std::forward<FUNCTION>(function));
} // for_each_u

//----------------------------------------------------------------------------//
//! Abstraction function for fine-grained, data-parallel interface.
//!
//! @tparam R range type
//! @tparam FUNCTION    The calleable object type.
//! @tparam REDUCTION   The reduction variabel type.
//!
//! @param r range over which to execute \a function
//! @param function     The calleable object instance.
//!
//! @ingroup execution
//----------------------------------------------------------------------------//

template<class R, typename FUNCTION, typename REDUCTION>
inline void
reduce_each_u(R && r, REDUCTION & reduction, FUNCTION && function) {
  for(const auto & e : r)
    function(e, reduction);
} // reduce_each_u

} // namespace flecsi

#ifdef FLECSI_ENABLE_KOKKOS
// Reducers extracted from Kokkos
namespace flecsi {
namespace reducer {
template<typename TYPE>
using sum = Kokkos::Sum<TYPE>;
template<typename TYPE>
using prod = Kokkos::Prod<TYPE>;
template<typename TYPE>
using max = Kokkos::Max<TYPE>;
template<typename TYPE>
using min = Kokkos::Min<TYPE>;
template<typename TYPE>
using land = Kokkos::LAnd<TYPE>;
template<typename TYPE>
using lor = Kokkos::LOr<TYPE>;
template<typename TYPE>
using band = Kokkos::BAnd<TYPE>;
template<typename TYPE>
using bor = Kokkos::BOr<TYPE>;
template<typename TYPE>
using minmax = Kokkos::MinMax<TYPE>;
template<typename TYPE, typename IDX, typename SPACE>
using minloc = Kokkos::MinLoc<TYPE, IDX, SPACE>;
template<typename TYPE, typename IDX, typename SPACE>
using maxloc = Kokkos::MaxLoc<TYPE, IDX, SPACE>;
template<typename TYPE, typename IDX, typename SPACE>
using minmaxloc = Kokkos::MinMaxLoc<TYPE, IDX, SPACE>;
} // namespace reducer
} // namespace flecsi
#endif // FLECSI_ENABLE_KOKKOS
