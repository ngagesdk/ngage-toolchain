/* @file fixedp.cpp
*
* C-bindings for fixed-point math operations.
*
* CC0 http://creativecommons.org/publicdomain/zero/1.0/
* SPDX-License-Identifier: CC0-1.0
*
*/

#include <3dtypes.h>

#ifdef __cplusplus
extern "C" {
#endif

#define _HUGE_ENUF 1e+300
#define INFINITY ((float)(_HUGE_ENUF * _HUGE_ENUF))
#define NAN (-(float)(INFINITY * 0.0F))

#include "fixedp.h"

    float fp_div(float x, float y)
    {
        if (y == 0.0f)
        {
            return NAN;
        }
        TFixed fx = Real2Fix(x);
        TFixed fy = Real2Fix(y);
        TFixed dividend = FixDiv(fx, fy);
        return Fix2Real(dividend);
    }

    float fp_mul(float x, float y)
    {
        TFixed fx = Real2Fix(x);
        TFixed fy = Real2Fix(y);
        TFixed product = FixMul(fx, fy);
        return Fix2Real(product);
    }

    float fp_fmodf(float x, float y)
    {
        if (y == 0.0f)
        {
            return NAN;
        }
        TFixed fx = Real2Fix(x);
        TFixed fy = Real2Fix(y);
        TFixed dividend = FixDiv(fx, fy);
        TFixed product = FixMul(fy, dividend);
        TFixed remainder = fx - product;
        return Fix2Real(remainder);
    }

    float fp_sinf(float x)
    {
        TFixed fx = Real2Fix(x);
        TFixed sin, cos;
        FixSinCos(fx, sin, cos);
        return Fix2Real(sin);
    }

#ifdef __cplusplus
}
#endif
