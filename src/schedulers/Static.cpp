/**
 * Copyright (c) 2018    ATC (University of Cantabria) <nozalr@unican.es>
 * This file is part of EngineCL which is released under MIT License.
 * See file LICENSE for full license details.
 */
#include "schedulers/Static.hpp"

#include <tuple>

#include "Device.hpp"

namespace ecl {

void
fnThreadScheduler(StaticScheduler& scheduler)
{
  auto time1 = std::chrono::system_clock::now().time_since_epoch();
  scheduler.saveDuration(ActionType::schedulerStart);
  scheduler.saveDurationOffset(ActionType::schedulerStart);
  scheduler.preEnqueueWork();
  scheduler.waitCallbacks();
  scheduler.saveDuration(ActionType::schedulerEnd);
  scheduler.saveDurationOffset(ActionType::schedulerEnd);
}

StaticScheduler::StaticScheduler(WorkSplit wsplit)
  : mSema(1)
  , mHasWork(false)
  , mWorkSplit(wsplit)
{
  mMutexDuration = new mutex();
  mTimeInit = std::chrono::system_clock::now().time_since_epoch();
  mTime = std::chrono::system_clock::now().time_since_epoch();
  mDurationActions.reserve(8);       // NOTE: improve the reserve
  mDurationOffsetActions.reserve(8); // trade-off memory/common usage
}

StaticScheduler::~StaticScheduler()
{
  if (mThread.joinable()) {
    mThread.join();
  }
}

void
StaticScheduler::printStats()
{
  auto sum = 0;
  auto len = mDevices.size();
  for (uint i = 0; i < len; ++i) {
    sum += mChunkDone[i];
  }
  cout << "StaticScheduler:\n";
  cout << "chunks: " << sum << "\n";
  cout << "duration offsets from init:\n";
  for (auto& t : mDurationOffsetActions) {
    Inspector::printActionTypeDuration(std::get<1>(t), std::get<0>(t));
  }
}

void
StaticScheduler::waitCallbacks()
{
  mSema.wait(1);
}

void
StaticScheduler::notifyCallbacks()
{
  mSema.notify(1);
}

void
StaticScheduler::setTotalSize(size_t size)
{
  mSize = size;
  mHasWork = true;
}

void
StaticScheduler::setGws(NDRange gws)
{
  mGws = gws;
  mHasWork = true;
}

void
StaticScheduler::setLws(size_t lws)
{
  mLws = lws;
}

void
StaticScheduler::setOutPattern(uint outWorkitems, uint outPositions)
{
  mOutWorkitems = outWorkitems;
  mOutPositions = outPositions;
}

void
StaticScheduler::setDevices(vector<Device*>&& devices)
{
  mDevices = move(devices);
  mNumDevices = mDevices.size();
  mChunkTodo = vector<uint>(mNumDevices, 0);
  mChunkGiven = vector<uint>(mNumDevices, 0);
  mChunkDone = vector<uint>(mNumDevices, 0);
  mQueueIdWork.reserve(mNumDevices);
  mQueueIdWork = vector<vector<uint>>(mNumDevices, vector<uint>());
  mDevicesWorking = 0;
}

void
StaticScheduler::setRawProportions(const vector<float>& props)
{
  auto last = mNumDevices - 1;
  if (props.size() < last) {
    throw runtime_error("proportions < number of devices - 1");
  }

  if (last == 0) {
    mRawProportions = { 1.0f };
  } else {
    for (auto prop : props) {
      if (prop <= 0.0f || prop >= 1.0f) {
        throw runtime_error("proportion should be between (0.0f, 1.0f)");
      }
    }
    mRawProportions = move(props);
  }
  mWorkSplit = WorkSplit::Raw;
}

tuple<size_t, size_t>
StaticScheduler::splitWork(size_t size, float prop, size_t bound)
{
  size_t given = bound * (static_cast<size_t>(prop * size) / bound);
  size_t rem = size - given;
  if ((given % mLws) != 0) {
    throw runtime_error("given % lws: " + to_string(given) + " % " + to_string(mLws));
  }

  return { given, rem };
}

void
StaticScheduler::saveDuration(ActionType action)
{
  lock_guard<mutex> lock(*mMutexDuration);
  auto t2 = std::chrono::system_clock::now().time_since_epoch();
  size_t diffMs = std::chrono::duration_cast<std::chrono::milliseconds>(t2 - mTime).count();
  mDurationActions.push_back(make_tuple(diffMs, action));
  mTime = t2;
}
void
StaticScheduler::saveDurationOffset(ActionType action)
{
  lock_guard<mutex> lock(*mMutexDuration);
  auto t2 = std::chrono::system_clock::now().time_since_epoch();
  size_t diffMs = std::chrono::duration_cast<std::chrono::milliseconds>(t2 - mTimeInit).count();
  mDurationOffsetActions.push_back(make_tuple(diffMs, action));
}

void
StaticScheduler::calcProportions()
{
  vector<tuple<size_t, size_t>> proportions;
  int len = mNumDevices;
  uint last = len - 1;
  size_t wsizeGivenAcc = 0;
  size_t wsizeGiven = 0;
  size_t wsizeRemaining = mSize;
  size_t wsizeRestRemaining = 0;
  switch (mWorkSplit) {
    case WorkSplit::Raw:
      for (uint i = 0; i < last; ++i) {
        auto prop = mRawProportions[i];
        tie(wsizeGiven, wsizeRestRemaining) = splitWork(mSize, prop, mLws);
        size_t wsizeOffset = wsizeGivenAcc;
        proportions.push_back(make_tuple(wsizeGiven, wsizeOffset));
        wsizeGivenAcc += wsizeGiven;
        wsizeRemaining -= wsizeGiven;
      }
      proportions.push_back(make_tuple(wsizeRemaining, wsizeGivenAcc));
      break;
    case WorkSplit::By_Devices:
      for (uint i = 0; i < last; ++i) {
        tie(wsizeGiven, wsizeRemaining) = splitWork(wsizeRemaining, 1.0f / len, mLws);
        size_t wsizeOffset = wsizeGivenAcc;
        proportions.push_back(make_tuple(wsizeGiven, wsizeOffset));
        wsizeGivenAcc += wsizeGiven;
      }
      proportions.push_back(make_tuple(wsizeRemaining, wsizeGivenAcc));
      break;
  }
  mProportions = move(proportions);
}

void
StaticScheduler::start()
{
  mThread = thread(fnThreadScheduler, std::ref(*this));
}

void
StaticScheduler::enqueueWork(Device* device)
{
  int id = device->getID();
  if (mChunkTodo[id] == 0) {
    auto prop = mProportions[id];
    size_t size, offset;
    tie(size, offset) = prop;
    uint index;
    {
      lock_guard<mutex> guard(mMutexWork);
      index = mQueueWork.size();
      mQueueWork.push_back(Work(id, offset, size, mOutWorkitems, mOutPositions));
    }
    mQueueIdWork[id].push_back(index);
    mChunkTodo[id]++;
  }
}

void
StaticScheduler::preEnqueueWork()
{
  mDevicesWorking = mNumDevices;
  calcProportions();
}

void
StaticScheduler::requestWork(Device* device)
{
  enqueueWork(device);
  device->notifyWork();
}

void
StaticScheduler::callback(int queueIndex)
{
  {
    lock_guard<mutex> guard(mMutexWork);
    Work work = mQueueWork[queueIndex];
    int id = work.mDeviceId;
    mChunkDone[id]++;
    if (mChunkDone[id] == 1) {
      Device* device = mDevices[id];
      mDevicesWorking--;
      device->notifyWork();
      device->notifyEvent();
      if (mDevicesWorking == 0) {
        notifyCallbacks();
      }
    }
  }
}

int
StaticScheduler::getWorkIndex(Device* device)
{
  int id = device->getID();
  if (mHasWork) {
    if (mChunkGiven[id] == 0) {
      uint next = 0;
      {
        lock_guard<mutex> guard(mMutexWork);
        next = mChunkGiven[id]++;
      }
      return mQueueIdWork[id][next];
    } else {
      return -1;
    }
  } else {
    return -1;
  }
}

Work
StaticScheduler::getWork(uint queueIndex)
{
  return mQueueWork[queueIndex];
}

} // namespace ecl
