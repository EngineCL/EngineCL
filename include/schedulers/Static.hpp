/**
 * Copyright (c) 2018    ATC (University of Cantabria) <nozalr@unican.es>
 * This file is part of EngineCL which is released under MIT License.
 * See file LICENSE for full license details.
 */
#ifndef ENGINECL_SCHEDULER_STATIC_HPP
#define ENGINECL_SCHEDULER_STATIC_HPP 1

#include <iostream>
#include <mutex>
#include <thread>
#include <tuple>
#include <vector>

#include "Inspector.hpp"
#include "Scheduler.hpp"
#include "Semaphore.hpp"
#include "Work.hpp"

using std::lock_guard;
using std::make_tuple;
using std::mutex;
using std::thread;
using std::tie;

namespace ecl {
enum class ActionType;
class StaticScheduler;

void
fnThreadScheduler(StaticScheduler& scheduler);

class StaticScheduler : public Scheduler
{
public:
  enum WorkSplit
  {
    Raw = 0,
    By_Devices = 1,
  };

  StaticScheduler(StaticScheduler const&) = delete;
  StaticScheduler& operator=(StaticScheduler const&) = delete;

  StaticScheduler(StaticScheduler&&) = default;
  StaticScheduler& operator=(StaticScheduler&&) = default;

  // Public API
  void start() override;

  StaticScheduler(WorkSplit wsplit = WorkSplit::By_Devices);

  ~StaticScheduler();

  void notifyCallbacks();
  void waitCallbacks() override;

  void setTotalSize(size_t size) override;

  tuple<size_t, size_t> splitWork(size_t size, float prop, size_t bound) override;

  void calcProportions() override;

  void setRawProportions(const vector<float>& props);
  void setDevices(vector<Device*>&& devices) override;

  int getWorkIndex(Device* device) override;
  Work getWork(uint queueIndex) override;

  // Thread API
  void init();

  void printStats() override;

  void saveDuration(ActionType action);
  void saveDurationOffset(ActionType action);

  void callback(int queueIndex) override;
  void requestWork(Device* device) override;
  void enqueueWork(Device* device) override;
  void preEnqueueWork() override;

  void setGws(ecl::NDRange gws) override;
  void setLws(size_t lws) override;
  void setOutPattern(uint outWorkitems, uint outPositions) override;

private:
  thread mThread;
  size_t mSize;
  vector<Device*> mDevices;
  uint mNumDevices;
  Semaphore mSema;
  mutex mMutexWork;
  vector<Work> mQueueWork;
  vector<vector<uint>> mQueueIdWork;
  uint mQueueWorkSize;
  vector<uint> mChunkTodo;
  vector<uint> mChunkGiven;
  vector<uint> mChunkDone;
  bool mHasWork;
  uint mDevicesWorking;
  vector<tuple<size_t, size_t>> mProportions;
  vector<float> mRawProportions;
  WorkSplit mWorkSplit;

  ecl::NDRange mGws;
  size_t mLws;
  uint mOutWorkitems;
  uint mOutPositions;

  mutex* mMutexDuration;
  std::chrono::duration<double> mTimeInit;
  std::chrono::duration<double> mTime;
  vector<tuple<size_t, ActionType>> mDurationActions;
  vector<tuple<size_t, ActionType>> mDurationOffsetActions;
};

} // namespace ecl

#endif /* ENGINECL_SCHEDULER_STATIC_HPP */
