#ifndef ENGINECL_CONFIG_HPP
#define ENGINECL_CONFIG_HPP

#include <CL/cl.h>

#define CL_LWS 128

/* CONFIGURATION Begin */

#define ECL_OPERATION_BLOCKING_READ 1
// #define ECL_KERNEL_GLOBAL_WORK_OFFSET_SUPPORTED 0
// #define ECL_LOGGING 1
// #define ECL_RUNTIME_WAIT_ALL_READY 1
#define ECL_SAVE_CHUNKS 1

/* CONFIGURATION End */

#ifndef ECL_OPERATION_BLOCKING_READ
#define ECL_OPERATION_BLOCKING_READ 0
#endif

#ifndef ECL_KERNEL_GLOBAL_WORK_OFFSET_SUPPORTED
// enable if CL_VERSION_1_1 or higher
#if CL_VERSION_1_1 == 1
#define ECL_KERNEL_GLOBAL_WORK_OFFSET_SUPPORTED 1
#else // CL_VERSION_1_0 == 1
#define ECL_KERNEL_GLOBAL_WORK_OFFSET_SUPPORTED 0
#endif
#endif // ECL_KERNEL_GLOBAL_WORK_OFFSET_SUPPORTED defined

#if ECL_KERNEL_GLOBAL_WORK_OFFSET_SUPPORTED == 0
#warning                                                                                           \
  "[ECL_KERNEL_GLOBAL_WORK_OFFSET_SUPPORTED == 0] Kernel should receive the last argument as 'uint mOffset'"
#endif

#ifndef ECL_LOGGING
#define ECL_LOGGING 0
#endif
#if ECL_LOGGING
#define IF_LOGGING(x) (x)
#else
#define IF_LOGGING(x)
#endif

#ifndef ECL_RUNTIME_WAIT_ALL_READY
#define ECL_RUNTIME_WAIT_ALL_READY 0
#endif // ECL_RUNTIME_WAIT_ALL_READY

// device package tracking offsets
#ifndef ECL_SAVE_CHUNKS
#define ECL_SAVE_CHUNKS 0
#endif // ECL_SAVE_CHUNKS

#endif /* ENGINECL_CONFIG_HPP */
