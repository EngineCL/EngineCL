# EngineCL

Usability and Performance in Heterogeneous Computing.

* [Core](#core)
  + [Targets](#targets)
  + [Examples](#examples)
    - [Static](#static)
    - [Dynamic](#dynamic)
* [Citing](#citing)
* [Research](#research)
* [Contributions](#contributions)
* [License](#license)

## Core

The core is extracted as a library, exposing the Tier-2.

### Targets

Building targets for debug, release or debug-test.

```
make build/debug
make build/release
make build/debug-test
```

### Examples

EngineCL-tier2 is provided to be able to execute Saxpy benchmark. It can use two load balancing algorithms.

#### Static

Static algorithm with devices 0.0 and 1.0 (platform.device), problem size of 1024, chunksize of 128, results are checked. The work is divided in half (0.5 is given to first device).

The static props are the proportions given to the devices. Eg. if using 3 devices, we can provide 2 or 3 static props. Therefore, `0.5:0.2` or `0.5:0.2:0.3` are the same: 50% for the first, 20% for the second, 30% (rest) for the third.

```
 ❯ ./build/debug/EngineCL-tier2 1024 128 3.14 --static 0.5 --check --devices 0.0,1.0
Config:
  scheduler: static
  size: 1024
  chunksize: 128
  constant: 3.14
  check: yes
  kernel path:examples/tier-2/saxpy.cl
  platform.device list: 0.0 1.0
  static props: 0.5
  dynamic chunks: 1
time: 510
Kernel: saxpy
Runtime init timestamp: 1550921407
Runtime durations:
 initDiscovery: 117 ms.
Device id: 0
Selected platform: Clover
Selected device: AMD Radeon (TM) R7 300 Series (PITCAIRN, DRM 3.27.0, 4.20.10-arch1-1-ARCH, LLVM 7.0.1)
program type: source
kernel: saxpy
works: 1 works_size: 512
duration increments:
 init: 0 ms.
 useDiscovery: 0 ms.
 initContext: 0 ms.
 initQueue: 3 ms.
 initBuffers: 0 ms.
 initKernel: 387 ms.
 writeBuffers: 0 ms.
 deviceStart: 0 ms.
 deviceReady: 0 ms.
 deviceRun: 0 ms.
 deviceEnd: 0 ms.
 completeWork: 0 ms.
 total: 390 ms.
duration offsets from init:
 init: 118 ms.
 useDiscovery: 118 ms.
 initBuffers: 122 ms.
 initKernel: 509 ms.
 writeBuffers: 509 ms.
 deviceStart: 509 ms.
 deviceReady: 509 ms.
 deviceRun: 509 ms.
 deviceEnd: 510 ms.
use events: yes
chunks (mOffset+mSize:ts_ms+duration_ms)type-chunks,0+512:509+1,
Device id: 1
Selected platform: AMD Accelerated Parallel Processing
Selected device: AMD FX-8320E Eight-Core Processor
program type: source
kernel: saxpy
works: 1 works_size: 512
duration increments:
 init: 0 ms.
 useDiscovery: 0 ms.
 initContext: 0 ms.
 initQueue: 2 ms.
 initBuffers: 0 ms.
 initKernel: 40 ms.
 writeBuffers: 0 ms.
 deviceStart: 0 ms.
 deviceReady: 0 ms.
 deviceRun: 0 ms.
 deviceEnd: 0 ms.
 completeWork: 0 ms.
 total: 42 ms.
duration offsets from init:
 init: 118 ms.
 useDiscovery: 118 ms.
 initBuffers: 120 ms.
 initKernel: 161 ms.
 writeBuffers: 161 ms.
 deviceStart: 161 ms.
 deviceReady: 161 ms.
 deviceRun: 161 ms.
 deviceEnd: 161 ms.
use events: yes
chunks (mOffset+mSize:ts_ms+duration_ms)type-chunks,512+512:161+0,
StaticScheduler:
chunks: 2
duration offsets from init:
 schedulerStart: 1 ms.
 schedulerEnd: 510 ms.
Success
```

### Dynamic

Dynamic algorithm with devices 0.0 and 1.0 (platform.device), problem size of 102400000, chunksize of 128, results are checked. The work is divided dynamically in 8 chunks, each one given to the first device that is free.

```
 ❯ ./build/debug/EngineCL-tier2 102400000 128 3.14 --dynamic 8 --check --devices 0.0,1.0
Config:
  scheduler: dynamic
  size: 102400000
  chunksize: 128
  constant: 3.14
  check: yes
  kernel path:examples/tier-2/saxpy.cl
  platform.device list: 0.0 1.0
  static props:
  dynamic chunks: 8
time: 1306
Kernel: saxpy
Runtime init timestamp: 1550921866
Runtime durations:
 initDiscovery: 102 ms.
Device id: 0
Selected platform: Clover
Selected device: AMD Radeon (TM) R7 300 Series (PITCAIRN, DRM 3.27.0, 4.20.10-arch1-1-ARCH, LLVM 7.0.1)
program type: source
kernel: saxpy
works: 2 works_size: 25600000
duration increments:
 init: 0 ms.
 useDiscovery: 0 ms.
 initContext: 0 ms.
 initQueue: 4 ms.
 initBuffers: 0 ms.
 initKernel: 413 ms.
 writeBuffers: 596 ms.
 deviceStart: 0 ms.
 deviceReady: 0 ms.
 deviceRun: 0 ms.
 deviceEnd: 0 ms.
 completeWork: 139 ms.
 total: 1152 ms.
duration offsets from init:
 init: 103 ms.
 useDiscovery: 103 ms.
 initBuffers: 107 ms.
 initKernel: 521 ms.
 writeBuffers: 1117 ms.
 deviceStart: 1117 ms.
 deviceReady: 1117 ms.
 deviceRun: 1117 ms.
 deviceEnd: 1257 ms.
use events: yes
chunks (mOffset+mSize:ts_ms+duration_ms)type-chunks,51200000+12800000:1117+102,76800000+12800000:1219+38,
Device id: 1
Selected platform: AMD Accelerated Parallel Processing
Selected device: AMD FX-8320E Eight-Core Processor
program type: source
kernel: saxpy
works: 6 works_size: 76800000
duration increments:
 init: 0 ms.
 useDiscovery: 0 ms.
 initContext: 0 ms.
 initQueue: 3 ms.
 initBuffers: 0 ms.
 initKernel: 43 ms.
 writeBuffers: 0 ms.
 deviceStart: 0 ms.
 deviceReady: 0 ms.
 deviceRun: 0 ms.
 deviceEnd: 0 ms.
 completeWork: 1153 ms.
 total: 1199 ms.
duration offsets from init:
 init: 102 ms.
 useDiscovery: 103 ms.
 initBuffers: 106 ms.
 initKernel: 150 ms.
 writeBuffers: 150 ms.
 deviceStart: 150 ms.
 deviceReady: 150 ms.
 deviceRun: 150 ms.
 deviceEnd: 1306 ms.
use events: yes
chunks (mOffset+mSize:ts_ms+duration_ms)type-chunks,0+12800000:150+714,12800000+12800000:864+90,25600000+12800000:954+90,38400000+12800000:1045+86,64000000+12800000:1131+88,89600000+12800000:1219+87,
DynamicScheduler:
chunks: 8
duration offsets from init:
 schedulerStart: 0 ms.
 schedulerEnd: 1306 ms.
Success
```

## Citing

If you use anything from this project, please, cite the following [paper](https://arxiv.org/abs/1805.02755):

```
Nozal, R., Bosque, J.L., Beivide, R.: EngineCL: Usability and Performance in Heterogeneous Computing. arXiv e-prints arXiv:1805.02755 (May 2018).
```

Current BibTeX citation details are:

```
@ARTICLE{EngineCL:2018,
       author = {{Nozal}, Ra{\'u}l and {Bosque}, Jose Luis and {Beivide}, Ram{\'o}n},
        title = {{EngineCL: Usability and Performance in Heterogeneous Computing}},
      journal = {arXiv e-prints},
     keywords = {Computer Science - Distributed, Parallel, and Cluster Computing, C.1.2, C.1.4, C.1.3, D.1.3, D.2.0, D.2.3, D.2.11, D.2.13, D.4.7, D.4.9, E.1},
         year = {2018},
        month = {May},
          eid = {arXiv:1805.02755},
        pages = {arXiv:1805.02755},
archivePrefix = {arXiv},
       eprint = {1805.02755},
 primaryClass = {cs.DC}
}
```

## Research

EngineCL is applied to different fields by well-known research groups, like the Department of Computing and Systems Engineering, from Uniersity of Zaragoza (UNIZAR). 

Some works involve co-execution using complex architectures like FPGAs, and optimizations under time-constrained scenarios, which novelty ideas are developed far from typical OpenCL/HPC applications.

- [GUZ19] Maria Angelica Davila Guzman et al. Cooperative CPU, GPU, and FPGA heterogeneous execution with EngineCL. 10.1007/s11227-019-02768-y. Feb 2019.
- [GUZ18] Maria Angelica Davila Guzman et al. First Steps Towards CPU, GPU, and FPGA Parallel Execution with EngineCL. CMMSE. Cadiz, Spain. Jul 2018.
- [NOZ19] Raúl Nozal et al. Towards Co-execution on Commodity Heterogeneous Systems: Optimizations for Time-Constrained Scenarios. WEHA HPCS 2019. Dublin, Ireland. Jul 2019.

## Contributions

Feel free to contact the maintainer, open issues or do pull requests. We encourage you to request, fix or add functionalities.

## License

Copyright (c) 2018    ATC (University of Cantabria) <nozalr@unican.es>

EngineCL is released under MIT License.

See file LICENSE for full license details.
