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

#include <flecsi/utils/ftest.h>

#define __FLECSI_PRIVATE__
#include <flecsi/execution/common/launch.h>
#include <flecsi/execution/execution.h>

#define THREADS_PER_BLOCK 128
#define MIN_CTAS_PER_SM 4

#if 0
using namespace flecsi::execution;

flog_register_tag(gpu_task);

namespace gpu_task {

/*
  A simple task with no arguments.
 */

__global__ void
__launch_bounds__(THREADS_PER_BLOCK,MIN_CTAS_PER_SM)
gpu_calc_smthng( )
{

}

__host__
void
simple(int value) {

const size_t blocks = (volume + THREADS_PER_BLOCK - 1) / THREADS_PER_BLOCK;
    gpu_calc_smthng<<<blocks,THREADS_PER_BLOCK>>>();

} // simple

flecsi_register_task(simple, task, toc, index);

} // namespace task

/*
  Test driver.
 */

int
gpu_task(int argc, char ** argv) {

  FTEST();

  flecsi_execute_task(simple, task, single, 10);

  flecsi_execute_task(simple, task, index, 8);

  return FTEST_RESULT();
}

ftest_register_test(gpu_task);
#endif
