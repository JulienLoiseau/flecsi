/*
    @@@@@@@@  @@           @@@@@@   @@@@@@@@ @@
   /@@/////  /@@          @@////@@ @@////// /@@
   /@@       /@@  @@@@@  @@    // /@@       /@@
   /@@@@@@@  /@@ @@///@@/@@       /@@@@@@@@@/@@
   /@@////   /@@/@@@@@@@/@@       ////////@@/@@
   /@@       /@@/@@//// //@@    @@       /@@/@@
   /@@       @@@//@@@@@@ //@@@@@@  @@@@@@@@ /@@
   //       ///  //////   //////  ////////  //

   Copyright (c) 2016, Triad National Security, LLC
   All rights reserved.
                                                                              */
#pragma once

/*!
  @file

  This file contains implementations of field accessor types.
 */

#if !defined(__FLECSI_PRIVATE__)
#error Do not include this file directly!
#endif

#include "flecsi/data/reference.hh"
#include "flecsi/exec/launch.hh"
#include "flecsi/util/array_ref.hh"
#include <flecsi/data/field.hh>

#include <algorithm>
#include <memory>
#include <stack>

namespace flecsi {
namespace data {

template<typename DATA_TYPE, size_t PRIVILEGES>
struct accessor<singular, DATA_TYPE, PRIVILEGES> {
  using value_type = DATA_TYPE;
  using base_type = accessor<dense, DATA_TYPE, PRIVILEGES>;
  using element_type = typename base_type::element_type;

  accessor(const base_type & b) : base(b) {}

  element_type & get() const {
    return base(0);
  } // data
  operator element_type &() const {
    return get();
  } // value

  const accessor & operator=(const DATA_TYPE & value) const {
    return const_cast<accessor &>(*this) = value;
  } // operator=
  accessor & operator=(const DATA_TYPE & value) {
    get() = value;
    return *this;
  } // operator=

  base_type & get_base() {
    return base;
  }
  const base_type & get_base() const {
    return base;
  }
  friend base_type * get_null_base(accessor *) { // for task_prologue_t
    return nullptr;
  }

private:
  base_type base;
}; // struct accessor

template<typename DATA_TYPE, size_t PRIVILEGES>
struct accessor<raw, DATA_TYPE, PRIVILEGES> : reference_base {
  using value_type = DATA_TYPE;
  using element_type = std::
    conditional_t<privilege_write(PRIVILEGES), value_type, const value_type>;

  explicit accessor(std::size_t f) : reference_base(f) {}

  auto span() const {
    return s;
  }

  void bind(util::span<element_type> x) { // for bind_accessors
    s = x;
  }

private:
  util::span<element_type> s;
}; // struct accessor

template<class T, std::size_t P>
struct accessor<dense, T, P> : accessor<raw, T, P> {
  using base_type = accessor<raw, T, P>;
  using base_type::base_type;

  accessor(const base_type & b) : base_type(b) {}

  /*!
    Provide logical array-based access to the data referenced by this
    accessor.

    @param index The index of the logical array to access.
   */
  typename accessor::element_type & operator()(std::size_t index) const {
    const auto s = this->span();
    flog_assert(index < s.size(), "index out of range");
    return s[index];
  } // operator()

  base_type & get_base() {
    return *this;
  }
  const base_type & get_base() const {
    return *this;
  }
  friend base_type * get_null_base(accessor *) { // for task_prologue_t
    return nullptr;
  }
};

template<class T, std::size_t P, std::size_t OP = P>
struct ragged_accessor
  : accessor<raw, T, P>,
    util::with_index_iterator<const ragged_accessor<T, P, OP>> {
  using base_type = typename ragged_accessor::accessor;
  using typename base_type::element_type;
  using Offsets = accessor<dense, std::size_t, OP>;
  using row = util::span<element_type>;

  ragged_accessor(const base_type & b)
    : base_type(b), off(this->identifier()) {}

  row operator[](std::size_t i) const {
    // Without an extra element, we must store one endpoint implicitly.
    // Storing the end usefully ignores any overallocation.
    return this->span().first(off(i)).subspan(i ? off(i - 1) : 0);
  }
  std::size_t size() const noexcept { // not total!
    return off.span().size();
  }

  util::span<element_type> span() const {
    const auto s = off.span();
    return get_base().span().first(s.empty() ? 0 : s.back());
  }

  base_type & get_base() {
    return *this;
  }
  const base_type & get_base() const {
    return *this;
  }
  friend base_type * get_null_base(ragged_accessor *) { // for task_prologue_t
    return nullptr;
  }

  Offsets & get_offsets() {
    return off;
  }
  const Offsets & get_offsets() const {
    return off;
  }
  friend Offsets * get_null_offsets(ragged_accessor *) { // for task_prologue_t
    return nullptr;
  }

  template<class Topo, typename Topo::index_space S>
  static ragged_accessor parameter(
    const field_reference<T, data::ragged, Topo, S> & r) {
    return exec::replace_argument<base_type>(r.template cast<data::raw>());
  }

private:
  Offsets off;
};

template<class T, std::size_t P>
struct accessor<ragged, T, P>
  : ragged_accessor<T, P, privilege_repeat(ro, privilege_count(P))> {
  using accessor::ragged_accessor::ragged_accessor;
};

template<class T>
struct mutator<ragged, T>
  : util::with_index_iterator<const mutator<ragged, T>> {
  // NB: Normal accessors might have more than one privilege.
  using base_type = ragged_accessor<T, privilege_pack<rw>>;
  struct Overflow {
    std::size_t del;
    std::vector<T> add;
  };
  using TaskBuffer = std::vector<Overflow>;

private:
  using base_row = typename base_type::row;
  struct raw_row {
    using size_type = typename base_row::size_type;

    base_row s;
    Overflow * o;

    T & operator[](size_type i) const {
      const auto b = brk();
      if(i < b)
        return s[i];
      i -= b;
      flog_assert(i < o->add.size(), "index out of range");
      return o->add[i];
    }

    size_type brk() const noexcept {
      return s.size() - o->del;
    }
    size_type size() const noexcept {
      return brk() + o->add.size();
    }

    void destroy(std::size_t skip = 0) const {
      std::destroy(s.begin() + skip, s.begin() + brk());
    }
  };

public:
  struct row : util::with_index_iterator<const row>, private raw_row {
    using value_type = T;
    using typename raw_row::size_type;
    using difference_type = typename base_row::difference_type;
    using typename row::with_index_iterator::iterator;

    row(const raw_row & r) : raw_row(r) {}

    using raw_row::operator[];
    T & front() const {
      return *this->begin();
    }
    T & back() const {
      return this->end()[-1];
    }

    using raw_row::size;
    bool empty() const noexcept {
      return !this->size();
    }
    size_type max_size() const noexcept {
      return o->add.max_size();
    }
    size_type capacity() const noexcept {
      // We can't count the span, since the client might remove all its
      // elements and then work back up to capacity().  The strange result is
      // that size() can be greater than capacity(), but that just means that
      // every operation might invalidate everything.
      return o->add.capacity();
    }
    void reserve(size_type n) const {
      o->add.reserve(n);
    }
    void shrink_to_fit() const { // repacks into span; only basic guarantee
      const size_type mv = std::min(o->add.size(), o->del);
      const auto b = o->add.begin(), e = b + mv;
      std::uninitialized_move(b, e, s.end());
      o->del -= mv;
      if(mv || o->add.size() < o->add.capacity())
        decltype(o->add)(
          std::move_iterator(e), std::move_iterator(o->add.end()))
          .swap(o->add);
    }

    void clear() const noexcept {
      o->add.clear();
      for(auto n = brk(); n--;)
        pop_span();
    }
    iterator insert(iterator i, const T & t) const {
      return put(i, t);
    }
    iterator insert(iterator i, T && t) const {
      return put(i, std::move(t));
    }
    iterator insert(iterator i, size_type n, const T & t) const {
      const auto b = brki();
      auto & a = o->add;
      if(i < b) {
        const size_type
          // used elements assigned from t:
          filla = std::min(n, b.index() - i.index()),
          fillx = n - filla, // other spaces to fill
          fillc = std::min(fillx, o->del), // slack spaces constructed from t
          fillo = fillx - fillc, // overflow copies of t
          // slack spaces move-constructed from used elements:
          mvc = std::min(o->del - fillc, filla);

        // Perform the moves and fills mostly in reverse order.
        T *const s0 = s.data(), *const ip = s0 + i.index(),
                 *const bp = s0 + b.index();
        // FIXME: Is there a way to do just one shift?
        a.insert(a.begin(), fillo, t); // first to reduce the volume of shifting
        a.insert(a.begin() + fillo, bp - (filla - mvc), bp);
        std::uninitialized_move_n(
          bp - filla, mvc, std::uninitialized_fill_n(bp, fillc, t));
        o->del -= fillc + mvc; // before copies that might throw
        std::fill(ip, std::move_backward(ip, bp - filla, bp), t);
      }
      else {
        if(i == b)
          for(; n && o->del; --n) // fill in the gap
            push_span(t);
        a.insert(a.begin(), n, t);
      }
      return i;
    }
    template<class I, class = std::enable_if_t<!std::is_integral_v<I>>>
    iterator insert(iterator i, I a, I b) const {
      // FIXME: use size when available
      for(iterator j = i; a != b; ++a, ++j)
        insert(j, *a);
      return i;
    }
    iterator insert(iterator i, std::initializer_list<T> l) {
      return insert(i, l.begin(), l.end());
    }
    template<class... AA>
    iterator emplace(iterator i, AA &&... aa) const {
      return put<true>(i, std::forward<AA>(aa)...);
    }
    iterator erase(iterator i) const noexcept {
      const auto b = brki();
      if(i < b) {
        std::move(i + 1, b, i);
        pop_span();
      }
      else
        o->add.erase(o->add.begin() + (i - b));
      return i;
    }
    iterator erase(iterator i, iterator j) const noexcept {
      const auto b = brki();
      if(i < b) {
        if(j == i)
          ;
        else if(j <= b) {
          std::move(j, b, i);
          for(auto n = j - i; n--;)
            pop_span();
        }
        else {
          erase(b, j);
          erase(i, b);
        }
      }
      else {
        const auto ab = o->add.begin();
        o->add.erase(ab + (i - b), ab + (j - b));
      }
      return i;
    }
    void push_back(const T & t) const {
      emplace_back(t);
    }
    void push_back(T && t) const {
      emplace_back(std::move(t));
    }
    template<class... AA>
    T & emplace_back(AA &&... aa) const {
      return !o->del || !o->add.empty()
               ? o->add.emplace_back(std::forward<AA>(aa)...)
               : push_span(std::forward<AA>(aa)...);
    }
    void pop_back() const noexcept {
      if(o->add.empty())
        pop_span();
      else
        o->add.pop_back();
    }
    void resize(size_type n) const {
      extend(n);
    }
    void resize(size_type n, const T & t) const {
      extend(n, t);
    }
    // No swap: it would swap the handles, not the contents

  private:
    using raw_row::brk;
    using raw_row::o;
    using raw_row::s;

    auto brki() const noexcept {
      return this->begin() + brk();
    }
    template<bool Destroy = false, class... AA>
    iterator put(iterator i, AA &&... aa) const {
      static_assert(Destroy || sizeof...(AA) == 1);
      const auto b = brki();
      auto & a = o->add;
      if(i < b) {
        auto & last = s[brk() - 1];
        if(o->del)
          push_span(std::move(last));
        else
          a.insert(a.begin(), std::move(last));
        std::move_backward(i, b - 1, b);
        if constexpr(Destroy) {
          auto * p = &*i;
          p->~T();
          new(p) T(std::forward<AA>(aa)...);
        }
        else
          *i = (std::forward<AA>(aa), ...);
      }
      else if(i == b && o->del)
        push_span(std::forward<AA>(aa)...);
      else {
        const auto j = a.begin() + (i - b);
        if constexpr(Destroy)
          a.emplace(j, std::forward<AA>(aa)...);
        else
          a.insert(j, std::forward<AA>(aa)...);
      }
      return i;
    }
    template<class... U> // {} or {const T&}
    void extend(size_type n, U &&... u) const {
      auto sz = this->size();
      if(n <= sz) {
        while(sz-- > n)
          pop_back();
      }
      else {
        reserve(n - (o->add.empty() ? s.size() : 0));

        struct cleanup {
          const row & r;
          size_type sz0;
          bool fail = true;
          ~cleanup() {
            if(fail)
              r.resize(sz0);
          }
        } guard = {*this, sz};

        while(sz++ < n)
          emplace_back(std::forward<U>(u)...);
        guard.fail = false;
      }
    }
    template<class... AA>
    T & push_span(AA &&... aa) const {
      auto & ret = *new(&s[brk()]) T(std::forward<AA>(aa)...);
      --o->del;
      return ret;
    }
    void pop_span() const noexcept {
      ++o->del;
      s[brk()].~T();
    }
  };

  mutator(const base_type & b) : acc(b) {}

  row operator[](std::size_t i) const {
    return raw_get(i);
  }
  std::size_t size() const noexcept {
    return acc.size();
  }

  base_type & get_base() {
    return acc;
  }
  const base_type & get_base() const {
    return acc;
  }
  void buffer(TaskBuffer & b) { // for unbind_accessors
    over = &b;
  }
  void bind() const { // for bind_accessors
    over->resize(acc.size());
  }
  template<std::size_t N>
  static constexpr ragged_accessor<T, privilege_repeat(rw, N)> * null_base =
    nullptr; // for task_prologue_t

  void commit() const {
    // To move each element before overwriting it, we propagate moves outward
    // from each place where the movement switches from rightward to leftward.
    const auto all = acc.get_base().span();
    const std::size_t n = size();
    // Read and write cursors.  It would be possible, if ugly, to run cursors
    // backwards for the rightward-moving portions and do without the stack.
    std::size_t is = 0, js = 0, id = 0, jd = 0;
    raw_row rs = raw_get(is), rd = raw_get(id);
    struct work {
      void operator()() const {
        if(assign)
          *w = std::move(*r);
        else
          new(w) T(std::move(*r));
      }
      T *r, *w;
      bool assign;
    };
    std::stack<work> stk;
    const auto back = [&stk] {
      for(; !stk.empty(); stk.pop())
        stk.top()();
    };
    for(;; ++js, ++jd) {
      while(js == rs.size()) { // js indexes the split row
        if(++is == n)
          break;
        rs = raw_get(is);
        js = 0;
      }
      if(is == n)
        break;
      while(jd == rd.s.size()) { // jd indexes the span (including slack space)
        if(++id == n)
          break; // we can write off the end of the last
        else
          rd = raw_get(id);
        jd = 0;
      }
      const auto r = &rs[js], w = rd.s.data() + jd;
      flog_assert(w != all.end(),
        "ragged entries for last " << n - is << " rows overrun allocation for "
                                   << all.size() << " entries");
      if(r != w)
        stk.push({r, w, jd < rd.brk()});
      // Perform this move, and any already queued, if it's not to the right.
      // Offset real elements by one position to come after insertions.
      if(rs.s.data() + std::min(js + 1, rs.brk()) > w)
        back();
    }
    back();
    if(jd < rd.brk())
      rd.destroy(jd);
    while(++id < n)
      raw_get(id).destroy();
    auto & off = acc.get_offsets();
    std::size_t delta = 0; // may be "negative"; aliasing is implausible
    for(is = 0; is < n; ++is) {
      auto & ov = (*over)[is];
      off(is) += delta += ov.add.size() - ov.del;
      ov.add.clear();
    }
  }

private:
  raw_row raw_get(std::size_t i) const {
    return {get_base()[i], &(*over)[i]};
  }

  base_type acc;
  TaskBuffer * over = nullptr;
};

// Many compilers incorrectly require the 'template' for a base class.
template<class T, std::size_t P>
struct accessor<sparse, T, P>
  : field<T, sparse>::base_type::template accessor1<P>,
    util::with_index_iterator<const accessor<sparse, T, P>> {
  static_assert(!privilege_write_only(P),
    "sparse accessor requires read permission");

private:
  using FieldBase = typename field<T, sparse>::base_type;

public:
  using value_type = typename FieldBase::value_type;
  using base_type = typename FieldBase::template accessor1<P>;
  using element_type =
    std::tuple_element_t<1, typename base_type::element_type>;

private:
  using base_row = typename base_type::row;

public:
  // Use the ragged interface to obtain the span directly.
  struct row {
    row(base_row s) : s(s) {}
    element_type & operator()(std::size_t c) const {
      return std::partition_point(
        s.begin(), s.end(), [c](const value_type & v) { return v.first < c; })
        ->second;
    }

  private:
    base_row s;
  };

  accessor(const base_type & b) : base_type(b) {}

  row operator[](std::size_t i) const {
    return get_base()[i];
  }

  base_type & get_base() {
    return *this;
  }
  const base_type & get_base() const {
    return *this;
  }
  friend base_type * get_null_base(accessor *) { // for task_prologue_t
    return nullptr;
  }
};

} // namespace data

template<class T, std::size_t P>
struct exec::detail::task_param<data::accessor<data::raw, T, P>> {
  template<class Topo, typename Topo::index_space S>
  static auto replace(const data::field_reference<T, data::raw, Topo, S> & r) {
    return data::accessor<data::raw, T, P>(r.fid());
  }
};
template<class T, std::size_t S>
struct exec::detail::task_param<data::accessor<data::dense, T, S>> {
  using type = data::accessor<data::dense, T, S>;
  template<class Topo, typename Topo::index_space Space>
  static type replace(
    const data::field_reference<T, data::dense, Topo, Space> & r) {
    return exec::replace_argument<typename type::base_type>(
      r.template cast<data::raw>());
  }
};
template<class T, std::size_t S>
struct exec::detail::task_param<data::accessor<data::singular, T, S>> {
  using type = data::accessor<data::singular, T, S>;
  template<class Topo, typename Topo::index_space Space>
  static type replace(
    const data::field_reference<T, data::singular, Topo, Space> & r) {
    return exec::replace_argument<typename type::base_type>(
      r.template cast<data::dense>());
  }
};
template<class T, std::size_t P>
struct exec::detail::task_param<data::accessor<data::ragged, T, P>> {
  using type = data::accessor<data::ragged, T, P>;
  template<class Topo, typename Topo::index_space S>
  static type replace(
    const data::field_reference<T, data::ragged, Topo, S> & r) {
    return type::parameter(r);
  }
};
template<class T>
struct exec::detail::task_param<data::mutator<data::ragged, T>> {
  using type = data::mutator<data::ragged, T>;
  template<class Topo, typename Topo::index_space S>
  static type replace(
    const data::field_reference<T, data::ragged, Topo, S> & r) {
    return type::base_type::parameter(r);
  }
};
template<class T, std::size_t S>
struct exec::detail::task_param<data::accessor<data::sparse, T, S>> {
  using type = data::accessor<data::sparse, T, S>;
  template<class Topo, typename Topo::index_space Space>
  static type replace(
    const data::field_reference<T, data::sparse, Topo, Space> & r) {
    return exec::replace_argument<typename type::base_type>(
      r.template cast<data::ragged,
        typename field<T, data::sparse>::base_type::value_type>());
  }
};

} // namespace flecsi
