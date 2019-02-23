/**
 * Copyright (c) 2018    ATC (University of Cantabria) <nozalr@unican.es>
 * This file is part of EngineCL which is released under MIT License.
 * See file LICENSE for full license details.
 */
#ifndef ENGINECL_CLUTILS_HPP
#define ENGINECL_CLUTILS_HPP 1

#include <CL/cl.hpp>
#include <iostream>
#include <sstream>
#include <string>

using std::cerr;
using std::cout;
using std::ostringstream;
using std::runtime_error;
using std::string;

#define VARGS_(_10, _9, _8, _7, _6, _5, _4, _3, _2, _1, N, ...) N
#define VARGS(...) VARGS_(__VA_ARGS__, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0)
#define CONCAT_(a, b) a##b
#define CONCAT(a, b) CONCAT_(a, b)

#define CL_CHECK_ERROR_2(error, msg) CLUtils::checkError((error), (msg), __FILE__, __LINE__)
#define CL_CHECK_ERROR_1(error) CL_CHECK_ERROR_2(error, "")

#define CL_CHECK_ERROR(...) CONCAT(CL_CHECK_ERROR_, VARGS(__VA_ARGS__))(__VA_ARGS__)

class CLUtils
{
public:
  static bool checkError(cl_int error, string msg, const char* file, const int line);

  static const char* clErrorToStr(cl_int error);

  static const char* clEnumToStr(cl_int error);
};

namespace ecl {
namespace OpenCL {
struct LocalSpaceArg
{
  ::size_t size_;
};

template<typename T>
struct KernelArgumentHandler
{
  static ::size_t size(const T&) { return sizeof(T); }
  static const T* ptr(const T& value) { return &value; }
};

template<>
struct KernelArgumentHandler<LocalSpaceArg>
{
  static ::size_t size(const LocalSpaceArg& value) { return value.size_; }
  static const void* ptr(const LocalSpaceArg&) { return NULL; }
};

} // namespace OpenCL
} // namespace ecl

#endif // ENGINECL_CLUTILS_HPP
