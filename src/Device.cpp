/**
 * Copyright (c) 2018    ATC (University of Cantabria) <nozalr@unican.es>
 * This file is part of EngineCL which is released under MIT License.
 * See file LICENSE for full license details.
 */
#include "Device.hpp"

#include "Buffer.hpp"
#include "Inspector.hpp"
#include "Runtime.hpp"
#include "Scheduler.hpp"

#define USE_EVENTS 1
// #define USE_EVENTS 0

struct CBData
{
  int queue_index;
  ecl::Device* device;
  CBData(int queue_index_, ecl::Device* device_)
    : queue_index(queue_index_)
    , device(device_)
  {}
};

void CL_CALLBACK
callbackRead(cl_event /*event*/, cl_int /*status*/, void* data)
{
  CBData* cbdata = reinterpret_cast<CBData*>(data);
  ecl::Device* device = cbdata->device;
  ecl::Scheduler* scheduler = device->getScheduler();
#if ECL_SAVE_CHUNKS
  device->saveChunk();
#endif
  device->saveDuration(ecl::ActionType::completeWork);
  scheduler->callback(cbdata->queue_index);
  delete cbdata;
}

namespace ecl {

void
device_thread_func(Device& device)
{
  auto time1 = std::chrono::system_clock::now().time_since_epoch();
  device.init();
  Scheduler* scheduler = device.getScheduler();
  device.saveDuration(ActionType::deviceStart);
  device.saveDurationOffset(ActionType::deviceStart);

  Runtime* runtime = device.getRuntime();
  device.saveDuration(ActionType::deviceReady);
  device.saveDurationOffset(ActionType::deviceReady);
  runtime->notifyReady();
  runtime->notifyAllReady();

  device.waitRun();
  device.saveDuration(ActionType::deviceRun);
  device.saveDurationOffset(ActionType::deviceRun);

  scheduler->requestWork(&device);

  auto cont = true;
  while (cont) {
    device.waitWork();
    auto queue_index = scheduler->getWorkIndex(&device);

    if (queue_index >= 0) { //
      Work work = scheduler->getWork(queue_index);

      device.doWork(work.mOffset, work.mSize, work.mOutWorkitems, work.mOutPositions, queue_index);
    } else {
      device.notifyBarrier();
      cont = false;
    }
  }

  device.saveDuration(ActionType::deviceEnd);
  device.saveDurationOffset(ActionType::deviceEnd);
}

Device::Device(uint selPlatform, uint selDevice)
  : mSelPlatform(selPlatform)
  , mSelDevice(selDevice)
  , mNumArgs(0)
  , mProgramType(ProgramType::Source)
  , mMinMultiplier(1)
{
  mMutexDuration = new mutex();
  mTimeInit = std::chrono::system_clock::now().time_since_epoch();
  mTime = std::chrono::system_clock::now().time_since_epoch();
  mWorks = 0;
  mWorksSize = 0;
  mSemaWork = new Semaphore(1);
  mSemaRun = make_unique<Semaphore>(1);
  mDurationActions.reserve(1024);    // NOTE improve
  mDurationOffsetActions.reserve(8); // the reserve

#if ECL_SAVE_CHUNKS
  mChunks.reserve(160);
#endif
}

Device::~Device()
{
  if (mThread.joinable()) {
    mThread.join();
  }
}

void
Device::printStats()
{
  cout << "Device id: " << getID() << "\n";
  showInfo();
  cout << "program type: ";
  switch (mProgramType) {
    case ProgramType::CustomBinary:
      cout << "custom binary\n";
      break;
    case ProgramType::CustomSource:
      cout << "custom source\n";
      break;
    default:
      cout << "source\n";
  }
  cout << "kernel: " << mKernelStr << "\n";
  cout << "works: " << mWorks << " works_size: " << mWorksSize << "\n";
  size_t acc = 0;
  size_t total = 0;
  cout << "duration increments:\n";
  for (auto& t : mDurationActions) {
    auto d = std::get<0>(t);
    auto action = std::get<1>(t);
    if (action == ActionType::completeWork) {
      acc += d;
    } else {
      Inspector::printActionTypeDuration(action, d);
    }
    total += d;
  }
  cout << " completeWork: " << acc << " ms.\n";
  cout << " total: " << total << " ms.\n";
  cout << "duration offsets from init:\n";
  for (auto& t : mDurationOffsetActions) {
    Inspector::printActionTypeDuration(std::get<1>(t), std::get<0>(t));
  }
#if USE_EVENTS
  cout << "use events: yes\n";
#else
  cout << "use events: no\n";
#endif

#if ECL_SAVE_CHUNKS
  cout << "chunks (mOffset+mSize:ts_ms+duration_ms)";
  cout << "type-chunks,";
  for (auto chunk : mChunks) {
    cout << chunk.offset << "+" << chunk.size << ":" << chunk.ts_ms << "+" << chunk.duration_ms
         << ",";
  }
  cout << "\n";
#endif
}

#if ECL_SAVE_CHUNKS
void
Device::initChunk(size_t offset, size_t size)
{
  mChunk.offset = offset;
  mChunk.size = size;
  auto t2 = std::chrono::system_clock::now().time_since_epoch();
  size_t diff_ms = std::chrono::duration_cast<std::chrono::milliseconds>(t2 - mTimeInit).count();
  mChunk.ts_ms = diff_ms;
  mChunkUnsaved = true;
}

void
Device::saveChunk()
{
  if (mChunkUnsaved) {
    auto t2 = std::chrono::system_clock::now().time_since_epoch();
    size_t diff_ms = std::chrono::duration_cast<std::chrono::milliseconds>(t2 - mTimeInit).count();
    size_t duration_ms = diff_ms - mChunk.ts_ms;
    mChunks.push_back(Chunk(mChunk.offset, mChunk.size, mChunk.ts_ms, duration_ms));
    mChunkUnsaved = false;
  }
}
#endif

void
Device::saveDuration(ActionType action)
{
  lock_guard<mutex> lock(*mMutexDuration);
  auto t2 = std::chrono::system_clock::now().time_since_epoch();
  size_t diff_ms = std::chrono::duration_cast<std::chrono::milliseconds>(t2 - mTime).count();
  mDurationActions.push_back(make_tuple(diff_ms, action));
  mTime = t2;
}
#include <memory>
void
Device::saveDurationOffset(ActionType action)
{
  lock_guard<mutex> lock(*mMutexDuration);
  auto t2 = std::chrono::system_clock::now().time_since_epoch();
  size_t diff_ms = std::chrono::duration_cast<std::chrono::milliseconds>(t2 - mTimeInit).count();
  mDurationOffsetActions.push_back(make_tuple(diff_ms, action));
}

void
Device::setKernelArg(cl_uint index, const uint bytes, ArgType type)
{
  if (type != ArgType::LocalAlloc) {
    throw runtime_error("setLocalArg(uint, uint, ArgType) only admits ArgType::LocalAlloc");
  }
  mArgIndex.push_back(index);
  mArgType.push_back(type);
  mArgBytes.push_back(bytes);
  mArgPtr.push_back(NULL);
  mNumArgs++;
}

void
Device::setKernelArgLocalAlloc(cl_uint index, const uint bytes)
{
  mArgIndex.push_back(index);
  mArgType.push_back(ArgType::LocalAlloc);
  mArgBytes.push_back(bytes);
  mArgPtr.push_back(NULL);
  mNumArgs++;
}

void
Device::setScheduler(Scheduler* scheduler)
{
  mScheduler = scheduler;
}
Scheduler*
Device::getScheduler()
{
  return mScheduler;
}

void
Device::setID(int id)
{
  mId = id;
}
int
Device::getID()
{
  return mId;
}

void
Device::waitRun()
{
  string multiplier("");
  char* val = getenv("MIN_CHUNK_MULTIPLIER");
  if (val != nullptr) {
    multiplier = string(val);
    std::stringstream ss(multiplier);
    std::string item;
    auto i = 0;
    while (std::getline(ss, item, ',')) {
      if (mId == i) {
        mMinMultiplier = stoi(item);
      }
      i++;
    }
  }
  mSemaRun.get()->wait(1);
}

void
Device::notifyRun()
{
  mSemaRun.get()->notify(1);
}

void
Device::waitWork()
{
  mSemaWork->wait(1);
}

void
Device::notifyWork()
{
  mSemaWork->notify(1);
}

string&
Device::getBuffer()
{
  return mInfoBuffer;
}

void
Device::start()
{
  mThread = thread(device_thread_func, std::ref(*this));
}
void
Device::useRuntimeDiscovery()
{
  Runtime* runtime = mRuntime;
  mPlatform = runtime->usePlatformDiscovery(mSelPlatform);
  mDevice = runtime->useDeviceDiscovery(mSelPlatform, mSelDevice);
}

void
Device::setKernel(const string& source, const string& kernel)
{
  if (mProgramType == ProgramType::Source) {
    mProgramSource = source;
  } // else, using custom kernel
  mKernelStr = kernel;
}

void
Device::setKernel(const vector<char>& binary)
{
  mProgramType = ProgramType::CustomBinary;
  mProgramBinary = binary;
}

void
Device::setKernel(const string& source)
{
  mProgramType = ProgramType::CustomSource;
  mProgramSource = source;
}

void
Device::waitEvent()
{
  cl::Event::waitForEvents({ mEnd });
}
void
Device::notifyEvent()
{
  mEnd.setStatus(CL_COMPLETE);
}

void
Device::setBarrier(shared_ptr<Semaphore> barrier)
{
  mBarrier = barrier;
}

void
Device::doWork(size_t poffset, size_t size, uint outWorkitems, uint outPosition, int queueIndex)
{
  if (!size) {
    return callbackRead(nullptr, CL_COMPLETE, this);
  }
  if (mPreviousEvents.size() && mWorks) {
    mPreviousEvents.clear();
  }
  cl::Event evkernel;

  auto gws = size;
  size = outWorkitems * size / outPosition;
  size_t offset = outWorkitems * poffset / outPosition;

  cl_int cl_err;

#if ECL_SAVE_CHUNKS
  initChunk(offset, size);
#endif

#if ECL_KERNEL_GLOBAL_WORK_OFFSET_SUPPORTED == 1
  cl_err = mQueue.enqueueNDRangeKernel(mKernel,
                                       cl::NDRange(poffset),
                                       cl::NDRange(gws),
                                       cl::NDRange(mLws),
                                       &mPreviousEvents,
                                       &evkernel);
#else
  mKernel.setArg(mNumArgs, (uint)poffset);
  cl_err = mQueue.enqueueNDRangeKernel(
    mKernel, cl::NullRange, cl::NDRange(gws), cl::NDRange(mLws), &mPreviousEvents, &evkernel);
#endif
  CL_CHECK_ERROR(cl_err, "enqueue kernel");
#if USE_EVENTS
  cl::Event evread;
  vector<cl::Event> events({ evkernel });
#endif

  auto len = mOutEclBuffers.size();
  for (uint i = 0; i < len; ++i) {
#if USE_EVENTS
    vector<cl::Event> levents = events;
    cl::Event levread;
#endif
    Buffer& b = mOutEclBuffers[i];
    size_t size_bytes = b.byBytes(size);
    auto offset_bytes = b.byBytes(offset);
    cl_err = mQueue.enqueueReadBuffer(mOutBuffers[i],
#if ECL_OPERATION_BLOCKING_READ == 1
                                      CL_TRUE,
#else
                                      CL_FALSE,
#endif
                                      offset_bytes,
                                      size_bytes,
                                      b.dataWithOffset(offset),
#if USE_EVENTS
                                      &levents,
                                      &levread);
    events.push_back(levread);
    evread = levread;
#else
                                      NULL,
                                      NULL);
#endif
    CL_CHECK_ERROR(cl_err, "enqueue read buffer");
  }
  auto cbdata = new CBData(queueIndex, this);

#if ECL_OPERATION_BLOCKING_READ == 1
#if USE_EVENTS
  callbackRead(evread(), CL_COMPLETE, cbdata);
#else
  callbackRead(nullptr, CL_COMPLETE, cbdata);
#endif
#else
  evread.setCallback(CL_COMPLETE, callbackRead, cbdata);
#endif
  mQueue.flush();
  mWorks++;
  mWorksSize += size;
}

void
Device::init()
{
  mTime = std::chrono::system_clock::now().time_since_epoch();
  mInfoBuffer.reserve(128);
  saveDuration(ActionType::init);
  saveDurationOffset(ActionType::init);

  useRuntimeDiscovery();
  saveDuration(ActionType::useDiscovery);
  saveDurationOffset(ActionType::useDiscovery);

  initContext();
  saveDuration(ActionType::initContext);
  initQueue();
  saveDuration(ActionType::initQueue);
  initBuffers();
  saveDuration(ActionType::initBuffers);
  saveDurationOffset(ActionType::initBuffers);
  initKernel();
  saveDuration(ActionType::initKernel);
  saveDurationOffset(ActionType::initKernel);
  initEvents();
  writeBuffers();
  saveDuration(ActionType::writeBuffers);
  saveDurationOffset(ActionType::writeBuffers);
}

void
Device::notifyBarrier()
{
  mBarrier.get()->notify(1);
}

/**
 * \brief Does not check bounds
 */
void
Device::initByIndex(uint selPlatform, uint selDevice)
{
  vector<cl::Platform> platforms;
  cl::Platform::get(&platforms);
  mPlatform = platforms.at(selPlatform);
  vector<cl::Device> devices;
  mPlatform.getDevices(CL_DEVICE_TYPE_ALL, &devices);
  mDevice = devices.at(selDevice);
}

void
Device::initContext()
{
  cl::Context context(mDevice);
  mContext = move(context);
}

void
Device::initQueue()
{
  cl_int cl_err;

  cl::Context& context = mContext;
  cl::Device& device = mDevice;

  cl::CommandQueue queue(context, device, 0, &cl_err);
  CL_CHECK_ERROR(cl_err, "CommandQueue queue");
  mQueue = move(queue);
}

void
Device::initBuffers()
{
  cl_int cl_err = CL_SUCCESS;

  cl_int buffer_in_flags = CL_MEM_READ_WRITE;
  cl_int buffer_out_flags = CL_MEM_READ_WRITE;

  mInBuffers.reserve(mInEclBuffers.size());
  mOutBuffers.reserve(mOutEclBuffers.size());

  auto len = mInEclBuffers.size();
  for (uint i = 0; i < len; ++i) {
    ecl::Buffer& b = mInEclBuffers[i];
    cl::Buffer tmp_buffer(mContext, buffer_in_flags, b.bytes(), NULL);
    CL_CHECK_ERROR(cl_err, "in buffer " + to_string(i));
    mInBuffers.push_back(move(tmp_buffer));
  }

  len = mOutEclBuffers.size();
  for (uint i = 0; i < len; ++i) {
    ecl::Buffer& b = mOutEclBuffers[i];
    cl::Buffer tmp_buffer(mContext, buffer_out_flags, b.bytes(), NULL);
    CL_CHECK_ERROR(cl_err, "out buffer " + to_string(i));
    mOutBuffers.push_back(move(tmp_buffer));
  }
}

void
Device::writeBuffers(bool /* dummy */)
{
  auto len = mInEclBuffers.size();
  mPreviousEvents.reserve(len);
  mPreviousEvents.resize(len);
  for (uint i = 0; i < len; ++i) {
    Buffer& b = mInEclBuffers[i];
    auto data = b.data();
    CL_CHECK_ERROR(mQueue.enqueueWriteBuffer(
      mInBuffers[i], CL_FALSE, 0, b.bytes(), data, NULL, &(mPreviousEvents.data()[i])));
  }
}

void
Device::initKernel()
{
  cl_int cl_err;

  cl::Program::Sources sources;
  cl::Program::Binaries binaries;
  cl::Program program;

  if (mProgramType == ProgramType::CustomBinary) {
    binaries.push_back({ mProgramBinary.data(), mProgramBinary.size() });
#pragma GCC diagnostic ignored "-Wignored-attributes"
    vector<cl_int> status = { -1 };
#pragma GCC diagnostic pop
    program = cl::Program(mContext, { mDevice }, binaries, &status, &cl_err);
    CL_CHECK_ERROR(cl_err, "building program from binary failed for device " + to_string(mId));
  } else {
    sources.push_back({ mProgramSource.c_str(), mProgramSource.length() });
    program = cl::Program(mContext, sources);
  }
  string options;
  options.reserve(32);
  options += "-DECL_KERNEL_GLOBAL_WORK_OFFSET_SUPPORTED=" +
             to_string(ECL_KERNEL_GLOBAL_WORK_OFFSET_SUPPORTED);

  cl_err = program.build({ mDevice }, options.c_str());
  if (cl_err != CL_SUCCESS) {
    cout << " Error building: " << program.getBuildInfo<CL_PROGRAM_BUILD_LOG>(mDevice) << "\n";
    CL_CHECK_ERROR(cl_err);
  }

  cl::Kernel kernel(program, mKernelStr.c_str(), &cl_err);
  CL_CHECK_ERROR(cl_err, "kernel");

  auto len = mArgIndex.size();
  auto unassigned = mInBuffersPtr.size();
  for (uint i = 0; i < len; ++i) {
    auto index = mArgIndex[i];
    auto type = mArgType[i];
    if (type == ArgType::Vector) {
      int pos = -1;
      auto address = mArgPtr[i];
      auto assigned = false;
      if (unassigned > 0) { // usually more in buffers than out
        auto it = find(begin(mInBuffersPtr), end(mInBuffersPtr), address);
        if (it != end(mInBuffersPtr)) {
          pos = distance(mInBuffersPtr.begin(), it);
          cl_err = kernel.setArg(index, mInBuffers[pos]);
          CL_CHECK_ERROR(cl_err, "kernel arg in buffer " + to_string(i));
          unassigned--;
          assigned = true;
        }
      }

      if (pos == -1) {
        auto it = find(begin(mOutBuffersPtr), end(mOutBuffersPtr), address);
        if (it != end(mOutBuffersPtr)) {
          auto pos = distance(mOutBuffersPtr.begin(), it);
          cl_err = kernel.setArg(index, mOutBuffers[pos]);
          CL_CHECK_ERROR(cl_err, "kernel arg out buffer " + to_string(i));
          assigned = true;
        }
      }

      if (!assigned) { // maybe empty space (__local)
        if (mArgBytes[i] > 0) {
          size_t bytes = mArgBytes[i];
          vector<void*>* vptr = reinterpret_cast<vector<void*>*>(mArgPtr[i]);
          void* ptr = vptr->data();
          cl_err = kernel.setArg((cl_uint)i, bytes, ptr);
          CL_CHECK_ERROR(cl_err, "kernel arg " + to_string(i));
        } else {
          throw runtime_error("unknown kernel arg address " + to_string(i));
        }
      }
    } else if (type == ArgType::T) {
      size_t bytes = mArgBytes[i];
      void* ptr = mArgPtr[i];
      cl_err = kernel.setArg((cl_uint)i, bytes, ptr);
      CL_CHECK_ERROR(cl_err, "kernel arg " + to_string(i));
    } else { // ArgType::LocalAlloc
      size_t bytes = mArgBytes[i];
      void* ptr = mArgPtr[i];
      // ptr should be NULL
      cl_err = kernel.setArg((cl_uint)i, bytes, ptr);
      CL_CHECK_ERROR(cl_err, "kernel arg " + to_string(i));
    }
  }

  mKernel = move(kernel);
}

void
Device::setLWS(size_t lws)
{
  mLws = lws;
}

void
Device::setTimeInit(std::chrono::duration<double> timeInit)
{
  mTimeInit = timeInit;
}

void
Device::initEvents()
{
  cl_int cl_err;
  cl::UserEvent end(mContext, &cl_err);
  CL_CHECK_ERROR(cl_err, "user event end");

  mEnd = move(end);
}

void
Device::showInfo()
{
  CL_CHECK_ERROR(mPlatform.getInfo(CL_PLATFORM_NAME, &mInfoBuffer));
  if (mInfoBuffer.size() > 2)
    mInfoBuffer.erase(mInfoBuffer.size() - 1, 1);
  cout << "Selected platform: " << mInfoBuffer << "\n";
  CL_CHECK_ERROR(mDevice.getInfo(CL_DEVICE_NAME, &mInfoBuffer));
  if (mInfoBuffer.size() > 2)
    mInfoBuffer.erase(mInfoBuffer.size() - 1, 1);
  cout << "Selected device: " << mInfoBuffer << "\n";
}

Runtime*
Device::getRuntime()
{
  return mRuntime;
}
void
Device::setRuntime(Runtime* runtime)
{
  mRuntime = runtime;
}

} // namespace ecl
