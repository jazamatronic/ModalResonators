#pragma once
#ifndef DSY_DUMB_BIQUAD_H
#define DSY_DUMB_BIQUAD_H

#include <stdint.h>
#ifdef __cplusplus

namespace daisysp
{
/** Dumb Biquad
 *
 *   Direct form 2nd order dumb biquad
 *   No coefficient calculation - just the filter core and functions to get/set the coefs
 *   Jared Anderson May 2021
 *
 *   a are the denominator coefs - assumes a0 = 1
 *   b are the numerator coefs 
 *   similar to the matlab/octave convention
 */
class dumb_biquad
{
  public:
    dumb_biquad() {}
    ~dumb_biquad() {}

    float Process(float in)
    {
      float out = in * b[0] + xn[0] * b[1] + xn[1] * b[2] 
		  - a[0] * yn[0] - a[1] * yn[1];
      xn[1] = xn[0];
      xn[0] = in;
      yn[1] = yn[0];
      yn[0] = out;
      return out;
    }

    inline void set_a(float a1, float a2)
    {
      a[0] = a1;
      a[1] = a2;
    }

    inline void set_b(float b0, float b1, float b2)
    {
      b[0] = b0;
      b[1] = b1;
      b[3] = b2;
    }

  protected:
    float a[2], b[3]; //Allow derived classes to directly manipulate coefs
  private:
    float xn[2] = {0, 0};
    float yn[2] = {0, 0};;
};
} // namespace daisysp
#endif
#endif
