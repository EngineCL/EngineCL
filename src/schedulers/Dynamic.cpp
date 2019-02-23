/**
 * Copyright (c) 2018    ATC (University of Cantabria) <nozalr@unican.es>
 * This file is part of EngineCL which is released under MIT License.
 * See file LICENSE for full license details.
 */
#include "schedulers/Dynamic.hpp"

#include <tuple>

#include "Device.hpp"
#include "Scheduler.hpp"

#define ATOMIC 1
// #define ATOMIC 0

namespace ecl {

void
fnThreadScheduler(DynamicScheduler& scheduler)
{
  auto time1 = std::chrono::system_clock::now().time_since_epoch();
  scheduler.saveDuration(ActionType::schedulerStart);
  scheduler.saveDurationOffset(ActionType::schedulerStart);
  scheduler.preEnqueueWork();
  while (scheduler.hasWork()) {
    auto moreReqs = true;
    do {
      auto device = scheduler.getNextRequest();
      if (device != nullptr) {
        scheduler.enqueueWork(device);
        device->notifyWork();
      } else {
        moreReqs = false;
      }
    } while (moreReqs);
    scheduler.waitCallbacks();
  }
  scheduler.notifyDevices();
  scheduler.saveDuration(ActionType::schedulerEnd);
  scheduler.saveDurationOffset(ActionType::schedulerEnd);
}

DynamicScheduler::DynamicScheduler(WorkSplit wsplit)
  : mWorkSplit(wsplit)
  , mHasWork(false)
  , mSemaRequests(1)
  , mSemaCallbacks(1)
  , mChunksDone(0)
  , mRequestsMax(0)
  , mRequestsIdx(0)
  , mRequestsIdxDone(0)
  , mRequestsList(0, 0)
{
  mMutexDuration = new mutex();
  mTimeInit = std::chrono::system_clock::now().time_since_epoch();
  mTime = std::chrono::system_clock::now().time_since_epoch();
  mDurationActions.reserve(8);       // NOTE: improve the reserve
  mDurationOffsetActions.reserve(8); // trade-off memory/common usage
}

DynamicScheduler::~DynamicScheduler()
{
  if (mThread.joinable()) {
    mThread.join();
  }
}

void
DynamicScheduler::printStats()
{
  auto sum = 0;
  auto len = mDevices.size();
  for (uint i = 0; i < len; ++i) {
    sum += mChunkDone[i];
  }
  cout << "DynamicScheduler:\n";
#if ATOMIC
  cout << "chunks: " << mChunksDone << "\n";
#else
  cout << "chunks: " << sum << "\n";
#endif
  cout << "duration offsets from init:\n";
  for (auto& t : mDurationOffsetActions) {
    Inspector::printActionTypeDuration(std::get<1>(t), std::get<0>(t));
  }
}

void
DynamicScheduler::notifyDevices()
{
  for (auto dev : mDevices) {
    dev->notifyWork();
  }
  for (auto dev : mDevices) {
    dev->notifyEvent();
  }
}

void
DynamicScheduler::saveDuration(ActionType action)
{
  lock_guard<mutex> lock(*mMutexDuration);
  auto t2 = std::chrono::system_clock::now().time_since_epoch();
  size_t diffMs = std::chrono::duration_cast<std::chrono::milliseconds>(t2 - mTime).count();
  mDurationActions.push_back(make_tuple(diffMs, action));
  mTime = t2;
}
void
DynamicScheduler::saveDurationOffset(ActionType action)
{
  lock_guard<mutex> lock(*mMutexDuration);
  auto t2 = std::chrono::system_clock::now().time_since_epoch();
  size_t diffMs = std::chrono::duration_cast<std::chrono::milliseconds>(t2 - mTimeInit).count();
  mDurationOffsetActions.push_back(make_tuple(diffMs, action));
}

void
DynamicScheduler::setChunks(size_t chunks)
{
  size_t total = mSize;

  if ((total % mLws) != 0) {
    throw runtime_error("requirement: problem mSize multiple of lws: " + to_string(total) + " % " +
                        to_string(mLws));
  }

  size_t chunks128 = total / mLws;
  size_t chunks128PerChunk = chunks128 / chunks;
  size_t chunksize = chunks128PerChunk * mLws;

  size_t restSize = total - (chunks * chunksize);
  size_t exactRestChunks = restSize % mLws;
  if (exactRestChunks != 0) {
    throw runtime_error("requirement: restSize chunks multiple of lws: " + to_string(restSize) +
                        " % " + to_string(mLws));
  }
  mWorksize = chunksize;
  mWorkLast = chunksize + restSize;
}

void
DynamicScheduler::setWorkSize(size_t size)
{
  size_t total = mSize;
  size_t times = size / mLws;
  size_t rest = size % mLws;
  size_t given = mLws;
  if (!size) {
    throw runtime_error("requirement: worksize > 0");
  }
  if (rest > 0) {
    given = (times + 1) * mLws;
  } else {
    given = size;
  }
  if (total < given) {
    given = total;
    mWorkLast = total;
  } else {
    times = total / given;
    rest = total - (times * given);
    total = times * given;
    mWorkLast = rest;
  }
  mWorksize = given;
  if ((mWorksize % mLws) != 0) {
    throw runtime_error("mWorksize % lws: " + to_string(mWorksize) + " % " + to_string(mLws));
  }
}

void
DynamicScheduler::setGws(NDRange gws)
{
  mGws = gws;
}

void
DynamicScheduler::setLws(size_t lws)
{
  mLws = lws;
}

void
DynamicScheduler::setOutPattern(uint outWorkitems, uint outPositions)
{
  mOutWorkitems = outWorkitems;
  mOutPositions = outPositions;
}

bool
DynamicScheduler::hasWork()
{
  return mSizeRemainingCompleted != 0;
}

void
DynamicScheduler::waitCallbacks()
{
  mSemaCallbacks.wait(1);
}

void
DynamicScheduler::waitRequests()
{
  mSemaRequests.wait(1);
}

void
DynamicScheduler::notifyCallbacks()
{
  mSemaCallbacks.notify(1);
}

void
DynamicScheduler::notifyRequests()
{
  mSemaRequests.notify(1);
}

void
DynamicScheduler::setTotalSize(size_t size)
{
  mSize = size;
  mHasWork = true;
  mSizeRemaining = size;
  mSizeGiven = 0;
  mSizeRemainingGiven = size;
  mSizeRemainingCompleted = size;
}

tuple<size_t, size_t>
DynamicScheduler::splitWork(size_t /* size */, float /* prop */, size_t /* bound */)
{
  return { 0, 0 };
}

void
DynamicScheduler::setDevices(vector<Device*>&& devices)
{
  mDevices = move(devices);
  mNumDevices = mDevices.size();
  mChunkTodo = vector<uint>(mNumDevices, 0);
  mChunkGiven = vector<uint>(mNumDevices, 0);
  mChunkDone = vector<uint>(mNumDevices, 0);

  mRequestsMax = mNumDevices * 2;
  mRequestsList = vector<uint>(mRequestsMax, 0);
  mQueueWork.reserve(65536);

  mQueueIdWork.reserve(mNumDevices);
  mQueueIdWork = vector<vector<uint>>(mNumDevices, vector<uint>());
  for (auto& qIdWork : mQueueIdWork) {
    qIdWork.reserve(65536);
  }
  mDevicesWorking = 0;
}

void
DynamicScheduler::calcProportions()
{}

void
DynamicScheduler::start()
{
  mThread = thread(fnThreadScheduler, std::ref(*this));
}

void
DynamicScheduler::enqueueWork(Device* device)
{
  int id = device->getID();
  if (mSizeRemaining > 0) {

    size_t size = mSizeGiven == 0 ? mWorkLast : mWorksize;
    size_t index = -1;
    {
      lock_guard<mutex> guard(mMutexWork);
      size_t offset = mSizeGiven;
      mSizeRemaining -= size;
      mSizeGiven += size;
      index = mQueueWork.size();
      mQueueWork.push_back(Work(id, offset, size, mOutWorkitems, mOutPositions));
      mQueueIdWork[id].push_back(index);
      mChunkTodo[id]++;
    }
  }
}

void
DynamicScheduler::preEnqueueWork()
{}

void
DynamicScheduler::requestWork(Device* device)
{
#if ATOMIC == 1
  if (mSizeRemainingCompleted > 0) {
    auto idx = mRequestsIdx++ % mRequestsMax;
    mRequestsList[idx] = device->getID() + 1;
  }
#else
  {
    lock_guard<mutex> guard(mMutexWork);
    if (mSizeRemainingCompleted) {
      mRequests.push(device->getID());
    }
  }
#endif
  notifyCallbacks();
}

void
DynamicScheduler::callback(int queueIndex)
{
#if ATOMIC == 1
  Work work = mQueueWork[queueIndex];
  int id = work.mDeviceId;
  mChunksDone++;
  mSizeRemainingCompleted -= work.mSize;
  if (mSizeRemainingCompleted > 0) {
    auto idx = mRequestsIdx++ % mRequestsMax;
    mRequestsList[idx] = id + 1;
  }
#else
  {
    lock_guard<mutex> guard(mMutexWork);
    Work work = mQueueWork[queue_index];
    int id = work.mDeviceId;
    mChunkDone[id]++;
    mSizeRemainingCompleted -= work.mSize;
    if (mSizeRemainingCompleted) {
      mRequests.push(id);
    }
  }
#endif
  notifyCallbacks();
}

int
DynamicScheduler::getWorkIndex(Device* device)
{
  lock_guard<mutex> guard(mMutexWork);
  int id = device->getID();
  if (mSizeRemainingGiven > 0 && mChunkTodo[id] > mChunkGiven[id]) {
    uint next = 0;
    int index = -1;
    next = mChunkGiven[id]++;
    mSizeRemainingGiven -= mWorksize;
    index = mQueueIdWork[id][next];
    return index;
  } else {
    return -1;
  }
}

Work
DynamicScheduler::getWork(uint queueIndex)
{
  lock_guard<mutex> guard(mMutexWork);
  return mQueueWork[queueIndex];
}

Device*
DynamicScheduler::getNextRequest()
{
  Device* dev = nullptr;
#if ATOMIC == 1
  uint idxDone = mRequestsIdxDone % mRequestsMax;
  uint id = mRequestsList[idxDone];
  if (id > 0) {
    dev = mDevices[id - 1];
    mRequestsList[idxDone] = 0;
    mRequestsIdxDone++;
  }
#else
  lock_guard<mutex> guard(mMutexWork);
  if (!mRequests.empty()) {
    int id = mRequests.front();
    mRequests.pop();
    dev = mDevices[id];
  }
#endif
  return dev;
}
} // namespace ecl
