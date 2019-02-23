/**
 * Copyright (c) 2018    ATC (University of Cantabria) <nozalr@unican.es>
 * This file is part of EngineCL which is released under MIT License.
 * See file LICENSE for full license details.
 */
#ifndef ENGINECL_RUNTIME_HPP
#define ENGINECL_RUNTIME_HPP 1

#include <chrono>
#include <memory>
#include <mutex>

#include "CLUtils.hpp"
#include "Device.hpp"
#include "NDRange.hpp"
#include "Semaphore.hpp"

using std::lock_guard;
using std::make_shared;
using std::make_tuple;
using std::make_unique;
using std::shared_ptr;
using std::string;
using std::vector;

namespace ecl {
class Scheduler;

class Runtime
{
public:
  void run();

  template<typename T>
  void setInBuffer(shared_ptr<vector<T>> array)
  {
    for (auto& device : mDevices) {
      device.setInBuffer(array);
    }
  }
  template<typename T>
  void setOutBuffer(shared_ptr<vector<T>> array)
  {
    for (auto& device : mDevices) {
      device.setOutBuffer(array);
    }
  }

  void setKernel(const string& source, const string& kernel);

  template<typename T>
  void setKernelArg(cl_uint index, const T& value)
  {
    for (auto& device : mDevices) {
      device.setKernelArg(index, value);
    }
  }
  template<typename T>
  void setKernelArg(cl_uint index, const uint bytes, ArgType arg)
  {
    for (auto& device : mDevices) {
      device.setKernelArg(index, bytes, arg);
    }
  }
  void setKernelArgLocalAlloc(cl_uint index, const uint bytes);

  void discoverDevices();

  void saveDuration(ActionType action);
  void saveDurationOffset(ActionType action);

  cl::Platform usePlatformDiscovery(uint selPlatform);
  cl::Device useDeviceDiscovery(uint selPlatform, uint selDevice);

  void printStats();

  void notifyAllReady();
  void waitAllReady();
  void notifyReady();
  void waitReady();

  void setScheduler(Scheduler* scheduler);

  Runtime(vector<Device>&& devices,
          NDRange gws,
          size_t lws = CL_LWS,
          uint out_workitems = 1,
          uint out_positions = 1);

private:
  void configDevices();

  vector<cl::Platform> mPlatforms;
  vector<vector<cl::Device>> mPlatformDevices;

  shared_ptr<Semaphore> mBarrier;
  vector<Device> mDevices;
  Scheduler* mScheduler;

  NDRange mGws;
  size_t mLws;
  uint mOutWorkitems;
  uint mOutPositions;

  shared_ptr<Semaphore> mSemaReady;
  Semaphore mSemaAllReady;

  mutex* mMutexDuration;
  std::chrono::duration<double> mTimeInit;
  std::chrono::duration<double> mTime;
  vector<tuple<size_t, ActionType>> mDurationActions;
  vector<tuple<size_t, ActionType>> mDurationOffsetActions;

  string mKernel;
};

} // namespace ecl

#endif /* ENGINECL_RUNTIME_HPP */
