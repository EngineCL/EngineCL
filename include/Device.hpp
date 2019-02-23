/**
 * Copyright (c) 2018    ATC (University of Cantabria) <nozalr@unican.es>
 * This file is part of EngineCL which is released under MIT License.
 * See file LICENSE for full license details.
 */
#ifndef ENGINECL_DEVICE_HPP
#define ENGINECL_DEVICE_HPP 1

#include <CL/cl.hpp>
#include <algorithm>
#include <chrono>
#include <iostream>
#include <string>
#include <thread>
#include <tuple>
#include <utility>

#include <cmath>

#include "Buffer.hpp"
#include "CLUtils.hpp"
#include "Semaphore.hpp"
#include "config.hpp"

using std::cout;
using std::move;
using std::mutex;
using std::shared_ptr;
using std::string;
using std::thread;
using std::to_string;
using std::tuple;
using std::unique_ptr;
using std::vector;

namespace ecl {
enum class ActionType;
class Scheduler;
class Runtime;
class Device;
class Buffer;
class NDRange;

#if ECL_SAVE_CHUNKS
struct Chunk
{
  size_t offset;
  size_t size;
  size_t ts_ms;
  size_t duration_ms;
  Chunk() {}
  Chunk(size_t _offset, size_t _size, size_t _ts_ms, size_t _duration_ms)
  {
    offset = _offset;
    size = _size;
    ts_ms = _ts_ms;
    duration_ms = _duration_ms;
  }
};
#endif

void
device_thread_func(Device& device);

enum class ProgramType
{
  Source = 0,
  CustomSource = 1,
  CustomBinary = 2,
};

enum class ArgType
{
  T = 0,
  Vector = 1,
  LocalAlloc = 2,
};

class Device
{
public:
  Device(uint selPlatform, uint selDevice);
  ~Device();

  Device(Device const&) = delete;
  Device& operator=(Device const&) = delete;

  Device(Device&&) = default;
  Device& operator=(Device&&) = default;

  // Public API
  void start();
  void setScheduler(Scheduler* scheduler);
  Scheduler* getScheduler();
  void setBarrier(shared_ptr<Semaphore> barrier);

  template<typename T>
  void setInBuffer(shared_ptr<vector<T>> array)
  {
    Buffer b(Direction::In);
    b.set(array);
    mInEclBuffers.push_back(move(b));
    auto address = array.get();
    mInBuffersPtr.push_back(address);
  }
  template<typename T>
  void setOutBuffer(shared_ptr<vector<T>> array)
  {
    Buffer b(Direction::Out);
    b.set(array);
    mOutEclBuffers.push_back(move(b));
    auto address = array.get();
    mOutBuffersPtr.push_back(address);
  }

  void setKernel(const string& source);
  void setKernel(const vector<char>& source);
  void setKernel(const string& source, const string& kernel);
  void setID(int id);
  int getID();
  void waitWork();
  void notifyWork();

  void printStats();

  void saveDuration(ActionType action);
  void saveDurationOffset(ActionType action);

  template<typename T>
  void setKernelArg(cl_uint index, const T& value)
  {
    mArgIndex.push_back(index);
    mArgType.push_back(ArgType::T);
    mArgBytes.push_back(OpenCL::KernelArgumentHandler<T>::size(value));
    // NOTE: legacy OpenCL 1.2 (added (void*))
    mArgPtr.push_back((void*)OpenCL::KernelArgumentHandler<T>::ptr(value));
    mNumArgs++;
  }

  template<typename T>
  void setKernelArg(cl_uint index, const shared_ptr<vector<T>>& value)
  {
    mArgIndex.push_back(index);
    auto address = value.get();
    auto bytes = sizeof(T) * address->size();
    mArgType.push_back(ArgType::Vector);
    mArgBytes.push_back(bytes);
    mArgPtr.push_back(address);
    mNumArgs++;
  }

  void setKernelArg(cl_uint index, const uint bytes, ArgType type);

  void setKernelArgLocalAlloc(cl_uint index, const uint bytes);

  void notifyEvent();
  void waitEvent();

  void notifyRun();
  void waitRun();

  void setLWS(size_t lws);

  void setTimeInit(std::chrono::duration<double> timeInit);

#if ECL_SAVE_CHUNKS
  void initChunk(size_t offset, size_t size);
  void saveChunk();
#endif

  uint getMinChunkMultiplier() { return mMinMultiplier; }

  // Thread API
  void init();
  void notifyBarrier();
  string& getBuffer();
  void showInfo();

  void doWork(size_t offset, size_t size, uint workitems, uint outPosition, int queueIndex);

  Runtime* getRuntime();
  void setRuntime(Runtime* runtime);

private:
  void useRuntimeDiscovery();
  void initByIndex(uint selPlatform, uint selDevice);
  void initContext();
  void initQueue();
  void initBuffers();
  void writeBuffers(bool dummy = false);
  void initKernel();
  void initEvents();

  uint mSelPlatform;
  uint mSelDevice;

  Scheduler* mScheduler;
  Runtime* mRuntime;

  shared_ptr<Semaphore> mBarrier;

  thread mThread;
  string mInfoBuffer;
#pragma GCC diagnostic ignored "-Wignored-attributes"
  vector<cl_uint> mArgIndex;
#pragma GCC diagnostic pop
  vector<ArgType> mArgType;
  uint mNumArgs;
  vector<size_t> mArgBytes;
  vector<void*> mArgPtr; // NOTE: legacy OpenCL 1.2

  vector<void*> mInBuffersPtr;
  vector<cl::Buffer> mInBuffers;
  vector<void*> mOutBuffersPtr;
  vector<cl::Buffer> mOutBuffers;

  vector<ecl::Buffer> mInEclBuffers;
  vector<ecl::Buffer> mOutEclBuffers;

  cl::Platform mPlatform;
  cl::Device mDevice;
  cl::Context mContext;
  cl::CommandQueue mQueue;
  cl::Kernel mKernel;
  cl::UserEvent mEnd;
  string mKernelStr;

  vector<cl::Event> mPreviousEvents;

  int mId;
  Semaphore* mSemaWork;
  unique_ptr<Semaphore> mSemaRun;

  size_t mWorks;
  size_t mWorksSize;
  mutex* mMutexDuration;
  std::chrono::duration<double> mTimeInit;
  std::chrono::duration<double> mTime;
  vector<tuple<size_t, ActionType>> mDurationActions;
  vector<tuple<size_t, ActionType>> mDurationOffsetActions;

  string mProgramSource;
  vector<char> mProgramBinary;
  ProgramType mProgramType;

  size_t mLws;

  uint mMinMultiplier;

#if ECL_SAVE_CHUNKS
  vector<Chunk> mChunks;
  Chunk mChunk;
  bool mChunkUnsaved;
#endif
};

} // namespace ecl

#endif /* ENGINECL_DEVICE_HPP */
