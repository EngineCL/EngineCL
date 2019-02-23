#ifndef ENGINECL_NDRANGE_HPP
#define ENGINECL_NDRANGE_HPP 1

#include <CL/cl.hpp>

namespace ecl {

class NDRange
{
private:
  size_t mSizes[3];
  cl_uint mDimensions;
  size_t mSpace;

public:
  //! \brief Default constructor - resulting range has zero dimensions.
  NDRange()
    : mDimensions(0)
  {}

  //! \brief Constructs one-dimensional range.
  NDRange(size_t size0)
    : mDimensions(1)
  {
    mSizes[0] = size0;
    mSpace = size0;
  }

  //! \brief Constructs two-dimensional range.
  NDRange(size_t size0, size_t size1)
    : mDimensions(2)
  {
    mSizes[0] = size0;
    mSizes[1] = size1;
    mSpace = size0 * size1;
  }

  //! \brief Constructs three-dimensional range.
  NDRange(size_t size0, size_t size1, size_t size2)
    : mDimensions(3)
  {
    mSizes[0] = size0;
    mSizes[1] = size1;
    mSizes[2] = size2;
    mSpace = size0 * size1 * size2;
  }

  /*! \brief Conversion operator to const size_t *.
   *
   *  \returns a pointer to the size of the first dimension.
   */
  operator const size_t*() const { return (const size_t*)mSizes; }

  //! \brief Queries the number of dimensions in the range.
  cl_uint dimensions() const { return mDimensions; }

  //! \brief Get the total space of the dimensions in the range.
  size_t space() const { return mSpace; }

  operator cl::NDRange() const
  {
    switch (mDimensions) {
      case 0:
        return cl::NDRange();
        break;
      case 1:
        return cl::NDRange(mSizes[0]);
        break;
      case 2:
        return cl::NDRange(mSizes[0], mSizes[1]);
        break;
      default:
        return cl::NDRange(mSizes[0], mSizes[1], mSizes[2]);
    }
  }
};
}

#endif /* ENGINECL_NDRANGE_HPP */
