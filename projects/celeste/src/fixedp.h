/* @file fixedp.h
*
* C-bindings for fixed-point math operations.
*
* CC0 http://creativecommons.org/publicdomain/zero/1.0/
* SPDX-License-Identifier: CC0-1.0
*
*/
#ifndef FIXEDP_H
#define FIXEDP_H

float fp_div(float x, float y);
float fp_mul(float x, float y);

float fp_fmodf(float x, float y);
float fp_sinf(float x);

#endif // FIXEDP_H
