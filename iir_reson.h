#pragma once
#ifndef DSY_IIR_RESON_H
#define DSY_IIR_RESON_H

#include <stdint.h>
#include "arm_math.h"
#include "dumb_biquad.h"
#ifdef __cplusplus

namespace daisysp
{
/** iir_reson
 *
 * Εxtend the dumb_biquad to turn it into a resonator
 *   Jared Anderson May 2021
 *
 * Based on:
 * STK BiQuad https://github.com/thestk/stk/blob/master/src/BiQuad.cpp
 * Perry R. Cook's Real Sound Synthesis 3.10
 * Garth Loy's Musimathics volume 2 5.13.3
 * Will Pirkle - Designing Audio Effect Plugins in C++ 2nd Edition - 11.1.2
 * A Note on Constant-Gain Digital Resonators
 *   Author(s): Ken Steiglitz
 *   Source: Computer Music Journal, Vol. 18, No. 4, (Winter, 1994), pp. 8-10
 *
 */
class iir_reson : public dumb_biquad
{
  public:
  /*
   * Initialize by setting the sample rate fs
   * cutoff freq fc in Hz
   * pole radius r
   * and overall gain g
   */
  void init(float fs, float fc, float r, float g)
  {
    fs_ = fs;
    fc_ = fc;
    wc_ = 2 * PI * fc_ / fs_;
    r_  = r;
    g_  = g;

    a[0] = -2 * r_ * cos(wc_);
    a[1] = r_ * r_;

    /*
     * Normalize by placing poles near DC and nyquist
     * There are lots of different propositions of ways to do this
     * This way seems to behave the best for a large range of r
     */
    b[0] = g_ * r_;
    b[1] = 0;
    b[2] = -b[0];
  }

  void update_fc(float fc)
  {
    if (fc != fc_) {
      fc_ = fc;
      wc_ = 2 * PI * fc_ / fs_;
      a[0] = -2 * r_ * cos(wc_);
    }
  }

  void update_r(float r)
  {
    if (r != r_) {
      r_ = r;
      wc_ = 2 * PI * fc_ / fs_;
      a[0] = -2 * r_ * cos(wc_);
      a[1] = r_ * r_;
      b[0] = g_ * r_;
      b[2] = -b[0];
    }
  }

  void update_g(float g)
  {
    if (g != g_) {
      g_ = g;
      b[0] = g_ * r_;
      b[2] = -b[0];
    }
  }

  private:
    float fs_, fc_, wc_, r_, g_;
};
} // namespace daisysp
#endif
#endif

