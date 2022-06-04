#pragma once
#ifndef DSY_IIR_1P_LP_H
#define DSY_IIR_1P_LP_H

#include <stdint.h>
#include "arm_math.h"
#ifdef __cplusplus

namespace daisysp
{
/** iir_1p_lp
 * 1st order Butterworth LP filter 
 *
 * fs = sampling frequency
 *
 * fc = center frequency in Hz
 *
 * Handbook for Digital Signal Processing Ch 9.
 *
 * [1] Sanjit K. Mitra,
 *     Digital Signal Processing a Computer Based Approach
 *
 */
class iir_1p_lp
{
  public:
  /*
   * Initialize by setting the sample rate fs
   * cutoff freq fc in Hz
   * and overall gain g
   */
  void init(float fs, float fc, float g = 1)
  {
    fs_ = fs;
    fc_ = fc;
    to_wc_ = 2 * PI / fs_; 
    wc_ = to_wc_ * fc_;
    xn_ = yn_ = 0;

    alpha_ = (1 - tan(wc_ / 2.0)) / (1 + tan(wc_ / 2.0));
    g_ = g * (1 - alpha_) / 2.0;
  }

  float Process(float in)
  {
    float out;

    out = in * b0_ + xn_ * b1_ - a1_ * yn_;
    xn_ = in;
    yn_ = out;

    return out;
  }


  void update_fc(float fc, float g = 1)
  {
    if (fc != fc_) {
      fc_ = fc;
      wc_ = to_wc_ * fc_;

      alpha_ = (1 - tan(wc_ / 2.0)) / (1 + tan(wc_ / 2.0));
      g_ = g * (1 - alpha_) / 2.0;
      b0_ = b1_ = g_;
      a1_ = -alpha_;
    }
  }

  void update_g(float g)
  {
    if (g != g_) {
      g_ = g * (1 - alpha_) / 2.0;
      b0_ = b1_ = g_;
    }
  }

  private:
    float b0_, b1_, a1_, alpha_, xn_, yn_;
    float fs_, fc_, wc_, g_, to_wc_;
};
} // namespace daisysp
#endif
#endif

