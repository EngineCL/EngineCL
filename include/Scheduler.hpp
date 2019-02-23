/**
 * Copyright (c) 2018    ATC (University of Cantabria) <nozalr@unican.es>
 * This file is part of EngineCL which is released under MIT License.
 * See file LICENSE for full license details.
 */
#ifndef ENGINECL_SCHEDULER_HPP
#define ENGINECL_SCHEDULER_HPP 1

#include <iostream>
#include <mutex>
#include <thread>
#include <tuple>
#include <vector>

#include "CLUtils.hpp"
#include "NDRange.hpp"
#include "Semaphore.hpp"
#include "Work.hpp"

using std::tuple;
using std::vector;

namespace ecl {
class Device;

class Scheduler
{
public:
  virtual void start() = 0;
  virtual void calcProportions() = 0;
  virtual void waitCallbacks() = 0;
  virtual void setTotalSize(size_t size) = 0;
  virtual tuple<size_t, size_t> splitWork(size_t size, float prop, size_t bound) = 0;
  virtual void setDevices(vector<Device*>&& devices) = 0;
  virtual int getWorkIndex(Device* device) = 0;
  virtual Work getWork(uint queueIndex) = 0;

  virtual void printStats() = 0;

  virtual void callback(int queueIndex) = 0;
  virtual void requestWork(Device* device) = 0;
  virtual void enqueueWork(Device* device) = 0;
  virtual void preEnqueueWork() = 0;

  virtual void setGws(NDRange gws) = 0;
  virtual void setLws(size_t lws) = 0;
  virtual void setOutPattern(uint outWorkitems, uint outPositions) = 0;
};

} // namespace ecl

#endif /* ENGINECL_SCHEDULER_HPP */
