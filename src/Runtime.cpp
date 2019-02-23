/**
 * Copyright (c) 2018    ATC (University of Cantabria) <nozalr@unican.es>
 * This file is part of EngineCL which is released under MIT License.
 * See file LICENSE for full license details.
 */
#include "Runtime.hpp"

#include "Device.hpp"
#include "Inspector.hpp"
#include "Scheduler.hpp"

namespace ecl {

Runtime::Runtime(vector<Device>&& devices,
                 NDRange gws,
                 size_t lws,
                 uint out_workitems,
                 uint out_positions)
  : mDevices(move(devices))
  , mGws(gws)
  , mLws(lws)
  , mOutWorkitems(out_workitems)
  , mOutPositions(out_positions)
  , mSemaAllReady(mDevices.size())
{
  mBarrier = make_shared<Semaphore>(mDevices.size());
  mSemaReady = make_unique<Semaphore>(1);

  mMutexDuration = new mutex();
  mTimeInit = std::chrono::system_clock::now().time_since_epoch();
  mTime = std::chrono::system_clock::now().time_since_epoch();
  mDurationActions.reserve(2);       // NOTE: improve
  mDurationOffsetActions.reserve(2); // the reserve
  for (auto& device : devices) {
    device.setTimeInit(mTimeInit);
  }
}

void
Runtime::saveDuration(ActionType action)
{
  lock_guard<mutex> lock(*mMutexDuration);
  auto t2 = std::chrono::system_clock::now().time_since_epoch();
  size_t diff_ms = std::chrono::duration_cast<std::chrono::milliseconds>(t2 - mTime).count();
  mDurationActions.push_back(make_tuple(diff_ms, action));
  mTime = t2;
}

void
Runtime::saveDurationOffset(ActionType action)
{
  lock_guard<mutex> lock(*mMutexDuration);
  auto t2 = std::chrono::system_clock::now().time_since_epoch();
  size_t diff_ms = std::chrono::duration_cast<std::chrono::milliseconds>(t2 - mTimeInit).count();
  mDurationOffsetActions.push_back(make_tuple(diff_ms, action));
}

void
Runtime::printStats()
{
  cout << "Kernel: " << mKernel << "\n";
  cout << "Runtime init timestamp: " << std::fixed << (long)mTimeInit.count() << "\n";
  cout << "Runtime durations:\n";
  for (auto& t : mDurationActions) {
    auto d = std::get<0>(t);
    auto action = std::get<1>(t);
    if (action == ActionType::initDiscovery) {
      cout << " initDiscovery: " << d << " ms.\n";
    }
  }
  for (auto& device : mDevices) {
    device.printStats();
  }
  mScheduler->printStats();
}

void
Runtime::setKernel(const string& source, const string& kernel)
{
  for (auto& device : mDevices) {
    device.setKernel(source, kernel);
  }
  mKernel = kernel;
}

void
Runtime::setKernelArgLocalAlloc(cl_uint index, const uint bytes)
{
  for (auto& device : mDevices) {
    device.setKernelArgLocalAlloc(index, bytes);
  }
}

void
Runtime::discoverDevices()
{
  IF_LOGGING(cout << "discoverDevices\n");
  cl::Platform::get(&mPlatforms);
  IF_LOGGING(cout << "platforms: " << mPlatforms.size() << "\n");
  IF_LOGGING(auto i = 0);
  for (auto& platform : mPlatforms) {
    vector<cl::Device> devices;
    platform.getDevices(CL_DEVICE_TYPE_ALL, &devices);
    IF_LOGGING(cout << "platform: " << i++ << " devices: " << devices.size() << "\n");
    mPlatformDevices.push_back(move(devices));
  }
}

cl::Platform
Runtime::usePlatformDiscovery(uint selPlatform)
{
  auto last_platform = mPlatforms.size() - 1;
  if (selPlatform > last_platform) {
    throw runtime_error("invalid platform selected");
  }
  return mPlatforms[selPlatform];
}

cl::Device
Runtime::useDeviceDiscovery(uint selPlatform, uint selDevice)
{
  auto last_platform = mPlatforms.size() - 1;
  if (selPlatform > last_platform) {
    throw runtime_error("invalid platform selected");
  }
  auto last_device = mPlatformDevices[selPlatform].size() - 1;
  if (selDevice > last_device) {
    throw runtime_error("invalid device selected");
  }
  return mPlatformDevices[selPlatform][selDevice];
}

void
Runtime::run()
{
  mScheduler->start();

  discoverDevices();
  saveDuration(ActionType::initDiscovery);
  saveDurationOffset(ActionType::initDiscovery);

  for (auto& device : mDevices) {
    device.setBarrier(mBarrier);
    device.start();
  }

  if (ECL_RUNTIME_WAIT_ALL_READY) {
    waitAllReady();
  }

  for (auto& device : mDevices) {
    device.notifyRun();
  }

  mBarrier.get()->wait(mDevices.size());
}

void
Runtime::notifyAllReady()
{
  mSemaAllReady.notify(1);
}

void
Runtime::waitAllReady()
{
  mSemaAllReady.wait(mDevices.size());
}

void
Runtime::notifyReady()
{
  mSemaReady.get()->notify(1);
}

void
Runtime::waitReady()
{
  mSemaReady.get()->wait(1);
}

void
Runtime::setScheduler(Scheduler* scheduler)
{
  mScheduler = scheduler;
  mScheduler->setTotalSize(mGws[0]); // TODO gws[0] ?
  mScheduler->setGws(mGws);
  mScheduler->setLws(mLws);
  mScheduler->setOutPattern(mOutWorkitems, mOutPositions);
  configDevices();
}

void
Runtime::configDevices()
{
  auto id = 0;
  vector<Device*> devices;
  int len = mDevices.size();
  for (auto i = 0; i < len; ++i) {
    Device& device = mDevices[i];
    device.setID(id++);
    device.setScheduler(mScheduler);
    device.setLWS(mLws);
    device.setRuntime(this);
    devices.push_back(&device);
  }
  mScheduler->setDevices(move(devices));
}

} // namespace ecl
