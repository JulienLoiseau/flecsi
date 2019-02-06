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

#define POLICY_NAMESPACE legion
#include <flecsi/data/common/client_handler.h>
#undef POLICY_NAMESPACE

#include <flecsi/data/common/client_handle.h>

namespace flecsi {

/*----------------------------------------------------------------------------*
  Forward topology types.
 *----------------------------------------------------------------------------*/

namespace topology {
struct global_topology_t;
struct color_topology_t;
template<typename>
class mesh_topology_u;
} // namespace topology

namespace data {
namespace legion {

/*----------------------------------------------------------------------------*
  Global Topology.
 *----------------------------------------------------------------------------*/

template<>
struct client_handler_u<topology::global_topology_t> {

  using client_t = topology::global_topology_t;

  template<size_t NAMESPACE_HASH, size_t NAME_HASH>
  static client_handle_u<client_t, 0> get_client_handle() {
    client_handle_u<client_t, 0> h;
    return h;
  } // get_client_handle

}; // client_handler_u<topology::global_topology_t>

/*----------------------------------------------------------------------------*
  Color Topology.
 *----------------------------------------------------------------------------*/

template<>
struct client_handler_u<topology::color_topology_t> {

  using client_t = topology::color_topology_t;

  template<size_t NAMESPACE_HASH, size_t NAME_HASH>
  static client_handle_u<client_t, 0> get_client_handle() {
    client_handle_u<client_t, 0> h;
    return h;
  } // get_client_handle

}; // client_handler_u<topology::color_topology_t>

/*----------------------------------------------------------------------------*
  Mesh Topology.
 *----------------------------------------------------------------------------*/

#if 0
template<typename MESH_POLICY>
struct client_handler_u<topology::mesh_topology_t<MESH_POLICY>> {

  using client_t = topology::mesh_topology_t<MESH_POLICY>;

  template<size_t NAMESPACE_HASH, size_t NAME_HASH>
  static client_handle_u<client_t, 0> get_client_handle() {
    client_handle_u<client_t, 0> h;
    return h;
  } // get_client_handle

}; // client_handler_u<topology::mesh_topology_t<MESH_POLICY>>
#endif

} // namespace legion
} // namespace data
} // namespace flecsi
