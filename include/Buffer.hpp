/**
 * Copyright (c) 2018    ATC (University of Cantabria) <nozalr@unican.es>
 * This file is part of EngineCL which is released under MIT License.
 * See file LICENSE for full license details.
 */
#ifndef ENGINECL_BUFFER_HPP
#define ENGINECL_BUFFER_HPP 1

#include <CL/cl.h>
#include <iostream>
#include <memory>
#include <typeinfo>
#include <vector>

#include "config.hpp"

using std::shared_ptr;
using std::vector;

namespace ecl {

enum class Direction
{
  In = 0,
  Out = 1,
  InOut = 2
};

class Buffer
{
public:
  Buffer(Direction direction);

  Buffer(Buffer const&) = delete;
  Buffer& operator=(Buffer const&) = delete;

  Buffer(Buffer&&) = default;
  Buffer& operator=(Buffer&&) = default;

  Direction direction();
  size_t size();
  size_t itemSize();
  size_t bytes();

  template<typename T>
  void set(shared_ptr<vector<T>> in)
  {
    vector<T>* v = in.get();
    mItemSize = sizeof(T);
    mSize = v->size();
    mBytes = sizeof(T) * mSize;
    mData = reinterpret_cast<void*>(v->data());
    mAddress = reinterpret_cast<void*>(v);
  }

  void* get();
  void* data();
  void* dataWithOffset(size_t offset);

  size_t byBytes(size_t size);

private:
  Direction mDirection;
  size_t mItemSize;
  size_t mSize;
  size_t mBytes;
  void* mData;
  void* mAddress;
};

} // namespace ecl

#endif /* ENGINECL_BUFFER_HPP */
