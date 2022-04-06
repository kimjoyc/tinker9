#pragma once
#include "ff/precision.h"
#include <cstddef>
#include <type_traits>

// https://docs.nvidia.com/cuda/cuda-c-programming-guide/index.html#atomic-functions
#if defined(__CUDA_ARCH__) && __CUDA_ARCH__ < 600
__device__
inline double atomicAdd(double* ptr, double v)
{
   unsigned long long int* ullptr = (unsigned long long int*)ptr;
   unsigned long long int old = *ullptr, assumed;
   do {
      assumed = old;
      old = atomicCAS(ullptr, assumed, __double_as_longlong(v + __longlong_as_double(assumed)));
   } while (assumed != old);
   // using floating-point comparison will hang in case of NaN (since NaN != NaN)
   return __longlong_as_double(old);
}
#endif

namespace tinker {
/// \ingroup cuda_syntax
/// \brief Adds `value` to `buffer[offset]`.
/// \note `value` and `buffer` elements are of the same type.
template <class T>
__device__
inline void atomic_add(T value, T* buffer, size_t offset = 0)
{
   atomicAdd(&buffer[offset], value);
}

/// \ingroup cuda_syntax
/// \brief Adds `value` to `buffer[offset]` via fixed-point arithmetic.
/// \tparam T  Must be of floating-point type.
template <class T,
   class = typename std::enable_if<std::is_same<T, float>::value ||
      std::is_same<T, double>::value>::type>
__device__
inline void atomic_add(T value, fixed* buffer, size_t offset = 0)
{
   // float -> (signed) long long -> fixed
   atomicAdd(&buffer[offset], static_cast<fixed>(static_cast<long long>(value * 0x100000000ull)));
}

/// \ingroup cuda_syntax
/// \brief Adds virial `{xx,yx,zx,yy,zy,zz}` to `buffer[offset][0 to 7]`.
template <class T>
__device__
inline void atomic_add(T vxx, T vyx, T vzx, T vyy, T vzy, T vzz, T (*buffer)[8], size_t offset = 0)
{
   atomic_add(vxx, buffer[offset], 0);
   atomic_add(vyx, buffer[offset], 1);
   atomic_add(vzx, buffer[offset], 2);
   atomic_add(vyy, buffer[offset], 3);
   atomic_add(vzy, buffer[offset], 4);
   atomic_add(vzz, buffer[offset], 5);
}

/// \ingroup cuda_syntax
/// \brief Adds virial `{xx,yx,zx,yy,zy,zz}` to `buffer[offset][0 to 7]` via fixed-point arithmetic.
template <class T,
   class = typename std::enable_if<std::is_same<T, float>::value ||
      std::is_same<T, double>::value>::type>
__device__
inline void atomic_add(
   T vxx, T vyx, T vzx, T vyy, T vzy, T vzz, fixed (*buffer)[8], size_t offset = 0)
{
   atomic_add(vxx, buffer[offset], 0);
   atomic_add(vyx, buffer[offset], 1);
   atomic_add(vzx, buffer[offset], 2);
   atomic_add(vyy, buffer[offset], 3);
   atomic_add(vzy, buffer[offset], 4);
   atomic_add(vzz, buffer[offset], 5);
}

/// \ingroup cuda_syntax
/// \brief Converts `val` of floating-point from its original type to type `G`.
template <class G, class T>
__device__
inline G floatTo(T val)
{
   static_assert(std::is_same<T, float>::value || std::is_same<T, double>::value, "");
   if CONSTEXPR (std::is_same<G, fixed>::value)
      return static_cast<G>(static_cast<long long>(val * 0x100000000ull));
   else
      return static_cast<G>(val);
}

/// \ingroup cuda_syntax
/// \brief Converts `val` of fixed-point to floating-point.
template <class T>
__device__
inline T fixedTo(fixed val)
{
   return static_cast<T>(static_cast<long long>(val)) / 0x100000000ull;
}

/// \ingroup cuda_syntax
/// \brief Used as `eq<T1,T2>()` for two type identifiers.
template <class T, class U>
__device__
__host__
constexpr bool eq()
{
   return std::is_same<T, U>::value;
}
}