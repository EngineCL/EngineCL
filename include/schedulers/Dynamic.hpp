/**
 * Copyright (c) 2018    ATC (University of Cantabria) <nozalr@unican.es>
 * This file is part of EngineCL which is released under MIT License.
 * See file LICENSE for full license details.
 */
#ifndef ENGINECL_SCHEDULER_DYNAMIC_HPP
#define ENGINECL_SCHEDULER_DYNAMIC_HPP 1

#include <atomic>
#include <chrono>
#include <iostream>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>

#include "Inspector.hpp"
#include "Scheduler.hpp"
#include "Semaphore.hpp"
#include "Work.hpp"
#include "config.hpp"

using std::atomic;
using std::lock_guard;
using std::make_tuple;
using std::mutex;
using std::queue;
using std::thread;
using std::tie;

namespace ecl {
enum class ActionType;
class Device;
class Scheduler;
class DynamicScheduler;

void
fnThreadScheduler(DynamicScheduler& scheduler);

class DynamicScheduler : public Scheduler
{
public:
  enum WorkSplit
  {
    Raw = 0,
    By_Devices = 1,
  };

  DynamicScheduler(WorkSplit wsplit = WorkSplit::By_Devices);
  ~DynamicScheduler();

  DynamicScheduler(DynamicScheduler const&) = delete;
  DynamicScheduler& operator=(DynamicScheduler const&) = delete;

  DynamicScheduler(DynamicScheduler&&) = default;
  DynamicScheduler& operator=(DynamicScheduler&&) = default;

  // Public API
  void start() override;

  void setChunks(size_t chunks);
  void setWorkSize(size_t size);

  bool hasWork();

  Device* getNextRequest();

  void notifyDevices();

  void waitRequests();
  void notifyRequests();
  void waitCallbacks() override;
  void notifyCallbacks();
  void setTotalSize(size_t size) override;

  tuple<size_t, size_t> splitWork(size_t size, float prop, size_t bound) override;

  void calcProportions() override;

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

  void setGws(NDRange gws) override;
  void setLws(size_t lws) override;
  void setOutPattern(uint outWorkitems, uint outPositions) override;

private:
  thread mThread;
  size_t mSize;
  vector<Device*> mDevices;
  uint mNumDevices;
  mutex mMutexWork;
  vector<Work> mQueueWork;
  vector<vector<uint>> mQueueIdWork;
  uint mQueueWorkSizes;
  vector<uint> mChunkTodo;
  vector<uint> mChunkGiven;
  vector<uint> mChunkDone;
  uint mDevicesWorking;
  vector<tuple<size_t, size_t>> mProportions;
  vector<float> mRawProportions;

  WorkSplit mWorkSplit;
  bool mHasWork;
  Semaphore mSemaRequests;
  Semaphore mSemaCallbacks;
  queue<int> mRequests;
  atomic<size_t> mChunksDone;
  int mRequestsMax;
  atomic<uint> mRequestsIdx;
  atomic<uint> mRequestsIdxDone;
  vector<uint> mRequestsList;

  size_t mSizeRemaining;
  size_t mSizeRemainingGiven;
  atomic<size_t> mSizeRemainingCompleted;
  size_t mSizeGiven;
  size_t mWorksize;
  size_t mWorkLast;

  NDRange mGws;
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

#endif /* ENGINECL_SCHEDULER_DYNAMIC_HPP */
