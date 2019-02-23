// -*- mode: c -*-

__kernel void
#if ECL_KERNEL_GLOBAL_WORK_OFFSET_SUPPORTED == 1
saxpy(__global int* in1, __global int* in2, __global int* out, int size, float a)
{
  int idx = get_global_id(0);
#else
saxpy(__global int* in1, __global int* in2, __global int* out, int size, float a, uint offset)
{
  int idx = get_global_id(0) + offset;
#endif

  if (idx >= 0 && idx < size) {
    out[idx] = (a * (float)in1[idx]) + in2[idx];
  }
}
