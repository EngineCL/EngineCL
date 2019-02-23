# ChangeLog

## v0.4.0 (2019-02-23)

**News**

- Core extracted as a library, supporting Tier-2.
- GitHub publication.

## v0.3.1 (2018-03-20)

**Bugfix**

- Static: needed mutex.
- Dynamic: needed atomic + each device should execute the package given to it.

**Acknowledgements**

- Angélica Dávila (UNIZAR)

## v0.3.0 (2018-03-09)

**Enhancements**

- Named: EngineCL (with `ecl` as namespace)
- Complete functional benchs: saxpy, gaussian, ray, binomial, mandelbrot and nbody.
- Every bench with a OpenCL C++ base case. All but ray with checkers.
- Conditional compilation for logging.
- Improved API for kernel argument (LocalAlloc).
- Output write pattern (relation between gws and index of out buffers).
- Version focused on usability and performance (overhead). Schedulers should be reviewed.
- New reviewed kernels (ray without artifacts).

**Stats**

- C++ LOC: 2219 `src`, 989 `include` (headers)

## v0.2.4 (2018-02-05)

**Enhancements**

- New bench: raytracer.
- Runtime with support of arguments of vector type without buffer initialization (memory allocation for `__local`).
- Buffer simplified and improved with template-based support of data types.
- Flags (`define`) renamed and organized into `config.hpp` file.
- Operations allow blocking reads.

**Stats**

- 2918 C++ lines of code in `src` (852 C/C++ header)

## v0.2.3 (2018-01-29)

**Enhancements**:

- Benchsuite with file reader (source and binary) via `io` compile unit.
- Device supports custom kernels (source and binary).
- Dynamic scheduler with ATOMIC support (`callbacks / get_next_request`).

**News**

- New tool to build kernels: clkernel.

## v0.2.2 (2018-01-22)

**Enhancements**:

- Added support for mOffset in kernel via last args.
- Removed `using namespace std`.
- Added scheduler durations.
- Member vars `size_given` and `size_rem` inside the mutex.

## v0.2.1 (2018-01-17)

**Enhancements**:

- Comments cleaned.
- Inspector created with printActionTypeDuration.
- No warnings in compilation (debug and release).

**News**

- MIT Licensed.
- Clang-format style changed to Mozilla.
- Tested in AMD APU, Intel CPU+IGPU, NVIDIA GPU, AMD CPU+GPU and Xeon Phi.
- Static and Dynamic working.
- HGuided prototype working.
- Makefile improved with clang-tidy and targets refactored.

**Stats**

- 2498 C++ lines of code in `src` (744 C/C++ header)

## v0.2.0 (2018-01-14)

**Enhancements**:

- HGuided load balancer created as prototype.
- Callbacks contain connection with the Work package: `callback` receives the queue index (not the `Device*`).
- Assign, Vecadd and Gaussian benchsuite with an unified structure.

## v0.1.10 (2018-01-03)

**Enhancements**:

- Dynamic load balancer created.
- New library-type API design focused on ease of use.
- Completely new architecture focusing library-runtime reuse.
- CMake organization and library-like building blocks.
- Concurrency mechanism based on callbacks (but easily portable to a direct API-call system).

## v0.1.0 (2017-12-13)

- Static load balancer created.
- Concurrency mechanism based on callbacks.
