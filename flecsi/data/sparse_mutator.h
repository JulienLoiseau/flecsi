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

/*! @file */

#include <algorithm>
#include <iostream>
#include <unordered_set>

#include <flecsi/data/mutator.h>
#include <flecsi/topology/index_space.h>

namespace flecsi {

//----------------------------------------------------------------------------//
//! The mutator_base_t type provides an empty base type for
//! compile-time
//! identification of data handle objects.
//!
//! @ingroup data
//----------------------------------------------------------------------------//

struct sparse_mutator_base_t {};

//----------------------------------------------------------------------------//
//! The mutator type captures information about permissions
//! and specifies a data policy. operator() is used to insert entries at a given
//! index into these buffers, and keeps entries in sorted order per index.
//! Entries may also be deleted with erase().
//!
//! @tparam T                     The data type referenced by the handle.
//!
//! @ingroup data
//----------------------------------------------------------------------------//

template<typename T>
struct mutator_u<data::sparse, T>
  : public mutator_u<data::ragged, data::sparse_entry_value_u<T>>,
    public sparse_mutator_base_t {
  using entry_value_t = data::sparse_entry_value_u<T>;

  using base_t = mutator_u<data::ragged, entry_value_t>;
  using typename base_t::handle_t;

  using index_space_t =
    topology::index_space_u<topology::simple_entry_u<size_t>, true>;

  //--------------------------------------------------------------------------//
  //! Copy constructor.
  //--------------------------------------------------------------------------//

  mutator_u(const handle_t & h) : base_t(h) {}

  T & operator()(size_t index, size_t entry) {
    size_t ragged_idx;
    auto itr = lower_bound(index, entry, &ragged_idx);

    // if we are attempting to create an entry that already exists
    // just over-write the value and exit.
    if(itr && itr->entry == entry) {
      return itr->value;
    }

    // otherwise, create a new entry
    auto & ragged = static_cast<base_t &>(*this);
    auto ritr = ragged.insert(index, ragged_idx, {entry, T()});
    return ritr->value;

  } // operator ()

  void dump() {
    auto & h_ = base_t::handle;
    std::size_t i = 0;
    const auto f = [&](const char * l, std::size_t n) {
      std::cout << l << ": \n";

      for(std::size_t end = i + n; i < end; ++i) {
        std::cout << "  index: " << i << std::endl;
        const auto & row = h_.new_entries[i];
        for(const auto & ev : row) {
          std::cout << "    +" << ev.entry << " = " << ev.value << std::endl;
        }
      }
    };
    f("exclusive", h_.num_exclusive);
    f("shared", h_.num_shared);
    f("ghost", h_.num_ghost);
  } // dump

  void erase(size_t index, size_t entry) {
    size_t ragged_idx;
    auto itr = lower_bound(index, entry, &ragged_idx);

    // if we are attempting to erase an entry that doesn't exist,
    // then just return
    if(!itr || itr->entry != entry) {
      return;
    }

    // otherwise, erase
    auto & ragged = static_cast<base_t &>(*this);
    ragged.erase(index, ragged_idx);

  } // erase

  // for row 'index', return pointer to first entry not less
  // than 'entry'
  entry_value_t *
  lower_bound(size_t index, size_t entry, size_t * pos = nullptr) {
    auto & h_ = base_t::handle;
    assert(h_.new_entries && "uninitialized mutator");
    assert(index < h_.num_total_);

    auto & row = h_.new_entries[index];
    auto start = row.begin();
    auto end = row.end();

    // find where entry should be
    auto itr = std::lower_bound(start, end, entry_value_t(entry),
      [](const entry_value_t & e1, const entry_value_t & e2) -> bool {
        return e1.entry < e2.entry;
      });

    if(pos) {
      size_t ragged_idx = itr - start;
      *pos = ragged_idx;
    }

    return (itr == end ? nullptr : &(*itr));

  } // lower_bound

  // for row 'index', return pointer to first entry not less
  // than 'entry'
  const entry_value_t * lower_bound(size_t index, size_t entry) const {
    auto & h_ = base_t::handle;
    assert(h_.new_entries && "uninitialized mutator");
    assert(index < h_.num_total_);

    auto & row = h_.new_entries[index];
    auto start = row.begin();
    auto end = row.end();

    // find where entry should be
    auto itr = std::lower_bound(start, end, entry_value_t(entry),
      [](const entry_value_t & e1, const entry_value_t & e2) -> bool {
        return e1.entry < e2.entry;
      });

    return (itr == end ? nullptr : &(*itr));

  } // lower_bound

  // for row 'index', test whether entry 'entry' is present
  bool contains(size_t index, size_t entry) const {
    auto itr = lower_bound(index, entry);
    return (itr && itr->entry == entry);
  } // contains

  //-------------------------------------------------------------------------//
  //! Return all entries used over all indices.
  //-------------------------------------------------------------------------//
  index_space_t entries() const {
    auto & h_ = base_t::handle;
    size_t id = 0;
    index_space_t is;
    std::unordered_set<size_t> found;

    for(size_t index = 0; index < h_.num_total_; ++index) {
      const auto & row = h_.new_entries[index];
      for(const auto & ev : row) {
        size_t entry = ev.entry;
        if(found.find(entry) == found.end()) {
          is.push_back({id++, entry});
          found.insert(entry);
        }
      }
    }

    return is;
  } // entries()

  //-------------------------------------------------------------------------//
  //! Return all entries used over the specified index.
  //-------------------------------------------------------------------------//
  index_space_t entries(size_t index) const {
    auto & h_ = base_t::handle;
    clog_assert(index < h_.num_total_, "sparse mutator: index out of bounds");

    size_t id = 0;
    index_space_t is;

    const auto & row = h_.new_entries[index];
    for(const auto & ev : row) {
      is.push_back({id++, ev.entry});
    }

    return is;
  } // entries(index)

  //-------------------------------------------------------------------------//
  //! Return all indices allocated.
  //-------------------------------------------------------------------------//
  index_space_t indices() const {
    auto & h_ = base_t::handle;
    index_space_t is;
    size_t id = 0;

    for(size_t index = 0; index < h_.num_total_; ++index) {
      const size_t count = h_.new_count(index);

      if(count != 0) {
        is.push_back({id++, index});
      }
    }

    return is;
  } // indices()

  //-------------------------------------------------------------------------//
  //! Return all indices allocated for a given entry.
  //-------------------------------------------------------------------------//
  index_space_t indices(size_t entry) const {
    auto & h_ = base_t::handle;
    index_space_t is;
    size_t id = 0;

    for(size_t index = 0; index < h_.num_total_; ++index) {
      auto itr = lower_bound(index, entry);
      if(itr && itr->entry == entry) {
        is.push_back({id++, index});
      }
    }

    return is;
  } // indices(index)

}; // mutator_u

template<typename T>
using sparse_mutator_u = mutator_u<data::sparse, T>;

template<typename T>
using sparse_mutator = sparse_mutator_u<T>;

} // namespace flecsi
