/**
 * Copyright (c) 2018    ATC (University of Cantabria) <nozalr@unican.es>
 * This file is part of EngineCL which is released under MIT License.
 * See file LICENSE for full license details.
 */
#include "Buffer.hpp"

#include <memory>
#include <vector>

namespace ecl {

Buffer::Buffer(Direction direction)
  : mDirection(direction)
{}

Direction
Buffer::direction()
{
  return mDirection;
}

size_t
Buffer::size()
{
  return mSize;
}

size_t
Buffer::bytes()
{
  return mBytes;
}

size_t
Buffer::itemSize()
{
  return mItemSize;
}

void*
Buffer::data()
{
  return mData;
}

void*
Buffer::dataWithOffset(size_t offset)
{
  return reinterpret_cast<char*>(mData) + byBytes(offset);
}

void*
Buffer::get()
{
  return mAddress;
}

size_t
Buffer::byBytes(size_t size)
{
  return mItemSize * size;
}

} // namespace ecl
