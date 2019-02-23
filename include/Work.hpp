/**
 * Copyright (c) 2018    ATC (University of Cantabria) <nozalr@unican.es>
 * This file is part of EngineCL which is released under MIT License.
 * See file LICENSE for full license details.
 */
#ifndef ENGINECL_WORK_HPP
#define ENGINECL_WORK_HPP 1

#include <string>

using std::runtime_error;
using std::to_string;

class Work
{
public:
  Work() = default;
  Work(int deviceId, size_t offset, size_t size, uint outWorkitems, uint outPositions)
    : mDeviceId(deviceId)
    , mOffset(offset)
    , mSize(size)
    , mOutWorkitems(outWorkitems)
    , mOutPositions(outPositions)
  {}

  int mDeviceId;
  size_t mOffset;
  size_t mSize;
  uint mOutWorkitems;
  uint mOutPositions;
};

inline size_t
splitWorkLikeHGuided(size_t total, size_t minWorksize, size_t lws, float computePower)
{
  const auto K = 2;
  uint ret = total * computePower / K;
  uint mult = ret / lws;
  uint rem = static_cast<size_t>(ret) % lws;
  if (rem) {
    mult++;
  }
  ret = mult * lws;
  if (ret < minWorksize) {
    ret = minWorksize;
  }
  if (total < ret) {
    ret = total;
  }
  if ((ret % lws) != 0) {
    throw runtime_error("ret % lws: " + to_string(ret) + " % " + to_string(lws));
  }
  return ret;
}

#endif // ENGINECL_WORK_HPP
