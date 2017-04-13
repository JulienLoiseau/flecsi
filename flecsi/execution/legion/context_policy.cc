/*~-------------------------------------------------------------------------~~*
 * Copyright (c) 2014 Los Alamos National Security, LLC
 * All rights reserved.
 *~-------------------------------------------------------------------------~~*/

///
/// \file
/// \date Initial file creation: Jul 26, 2016
///

#include <iostream>

#include "flecsi/execution/legion/context_policy.h"
#include "flecsi/execution/legion/legion_tasks.h"
//#include "flecsi/execution/legion/mapper.h"
#include "flecsi/data/storage.h"

namespace flecsi {
namespace execution {

thread_local std::unordered_map<size_t,
  std::stack<std::shared_ptr<legion_runtime_state_t>>> state_;  

///
///
///
int
legion_context_policy_t::initialize(
  int argc,
  char ** argv
)
{
  using namespace LegionRuntime::HighLevel;

  {
  clog_tag_guard(context);
  clog(info) << "Initializing..." << std::endl;
  }

  // Register top-level task
  HighLevelRuntime::set_top_level_task_id(TOP_LEVEL_TASK_ID);
  HighLevelRuntime::register_legion_task<runtime_driver>(
    TOP_LEVEL_TASK_ID, lr_loc, true, false);

  // Register user tasks
  for(auto & t: task_registry_) {

    // FIXME: The casts in this section need to be cleaned up...
    task_hash_key_t key = static_cast<task_hash_key_t>(t.first);

    // Iterate over task variants
    for(auto & v: t.second) {
      auto & value = std::get<1>(v);
      std::get<1>(value)(std::get<0>(value),
        mask_to_type(static_cast<processor_mask_t>(
          key.processor().to_ulong())),
        key.launch(), std::get<2>(value));
    } // for
  } // for

  // Intialize MPI/Legion interoperability layer.
  handshake_ = Legion::Runtime::create_handshake(
    true, // MPI has initial control
    1,    // MPI partiticipants
    1     // Legion participants
  );

  int rank;
  MPI_Comm_size(MPI_COMM_WORLD, &rank);
  Legion::Runtime::configure_MPI_interoperability(rank);

  // Start the Legion runtime
  HighLevelRuntime::start(argc, argv, true);

  do {
    handoff_to_legion();
    wait_on_legion();

    // execute mpi task
  } while(mpi_active_);

  handoff_to_legion();

  int version, subversion;
  MPI_Get_version(&version, &subversion);
  if(version==3 && subversion>0) {
    Legion::Runtime::wait_for_shutdown();
  } // if

  return 0;
} // legion_context_policy_t::initialize

///
///
///
void
legion_context_policy_t::unset_call_mpi(
  Legion::Context ctx,
  Legion::HighLevelRuntime * runtime
)
{
#if 0
  // Get a key to look up the task id that was assigned by the runtime.
  auto key = __flecsi_task_key(unset_call_mpi_task, loc);

  Legion::ArgumentMap arg_map;
  Legion::IndexLauncher launcher(
    context_t::instance().task_id(key),
    Legion::Domain::from_rect<1>(all_processes_),
    Legion::TaskArgument(0, 0),
    arg_map
  );

  launcher.tag = MAPPER_FORCE_RANK_MATCH;

  auto fm = runtime->execute_index_space(ctx, launcher);

  fm.wait_all_results();
#endif
} // legion_context_policy_t::unset_call_mpi

///
///
///
void
legion_context_policy_t::handoff_to_mpi(
  Legion::Context ctx,
  Legion::HighLevelRuntime * runtime
)
{
} // legion_context_policy_t::handoff_to_mpi

///
///
///
Legion::FutureMap
legion_context_policy_t::wait_on_mpi(
  Legion::Context ctx,
  Legion::HighLevelRuntime * runtime
)
{
#if 0
  // Get a key to look up the task id that was assigned by the runtime.
  auto key = __flecsi_task_key(wait_on_mpi_task, loc);

  Legion::ArgumentMap arg_map;
  Legion::IndexLauncher wait_on_mpi_launcher(
    context_t::instance().task_id(key),
    Legion::Domain::from_rect<1>(all_processes_),
    Legion::TaskArgument(0, 0),
    arg_map
  );

  auto fm = runtime->execute_index_space(ctx, wait_on_mpi_launcher);

  fm.wait_all_results();

  return fm;    
#endif
} // legion_context_policy_t::wait_on_legion

///
///
///
void
legion_context_policy_t::connect_with_mpi(
  Legion::Context ctx,
  Legion::HighLevelRuntime * runtime
)
{
  int size;
  MPI_Comm_size(MPI_COMM_WORLD, &size);
  all_processes_ = LegionRuntime::Arrays::Rect<1>(0, size-1);

  // FIXME: Does this do anything?
  // Both the application and Legion mappers have access to
  // the mappings between MPI Ranks and Legion address spaces
  // The reverse mapping goes the other way
  const std::map<int, Legion::AddressSpace> & forward_mapping =
    runtime->find_forward_MPI_mapping();

  for(auto it: forward_mapping) {
    clog(info) << "MPI rank " << it.first <<
      " maps to Legion address space " << it.second;
  } // for
} // legion_context_policy_t::connect_with_mpi

} // namespace execution 
} // namespace flecsi

/*~------------------------------------------------------------------------~--*
 * Formatting options for vim.
 * vim: set tabstop=2 shiftwidth=2 expandtab :
 *~------------------------------------------------------------------------~--*/