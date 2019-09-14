#ifndef TINKER_ACC_SWITCH_H_
#define TINKER_ACC_SWITCH_H_

#include "acc_mathfunc.h"

TINKER_NAMESPACE_BEGIN
/**
 * @brief
 * second order smooth step function
 * @f[
 * f: [cut,off]\rightarrow[1,0]
 * @f]
 *
 * deriving from
 * @f[ S_2: [0,1]\rightarrow[0,1] @f]
 * @f[ S_2(x) = 6 x^5 - 15 x^4 + 10 x^3 @f]
 *
 * @param[in] rik
 * distance
 *
 * @param[in] cut
 * distance at which switching of the potential begins
 *
 * @param[out] off
 * distance at which the potential energy goes to zero
 *
 * @param[out] taper
 * @f$ f @f$ value
 *
 * @param[out] dtaper
 * @f$ df/dx @f$ value
 *
 * @tparam DO_DTAPER
 * if @c false, @c dtaper will not be calculated
 */
#pragma acc routine seq
template <int DO_DTAPER>
inline void switch_taper5(real rik, real cut, real off,
                          real& __restrict__ taper, real& __restrict__ dtaper) {
  real _1_ab = REAL_RECIP(cut - off);
  real x = (rik - off) * _1_ab;
  real x2 = x * x;
  real x3 = x2 * x;
  taper = x3 * (6 * x2 - 15 * x + 10);
  if_constexpr(DO_DTAPER) { dtaper = 30 * REAL_SQ(x * (1 - x)) * _1_ab; }
}
TINKER_NAMESPACE_END

#endif
