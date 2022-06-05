#pragma once
#ifndef DSY_MODAL_INHARM_H
#define DSY_MODAL_INHARM_H

// Maximum resonance - let's try and keep things stable
#define MAX_R 0.99999

#define CLAMP(x, min, max)  (x > max) ? max : ((x < min) ? min : x)
#define SGN(x)		    (x > 0) ? 1.0 : ((x < 0) ? -1.0 : 0)
//#define SGN(x)		    signbit(x) ? -1.0 : 1.0

#include <stdint.h>
#include "arm_math.h"
#include "iir_reson.h"
#include "iir_1p_lp.h"
#ifdef __cplusplus

namespace daisysp
{
/*
 * A modal inharmonic is constructed from a set of modes
 * Specify the fundamental frequency and the mode multiples, 
 * gain and resonance factors 
 *
 *   Jared Anderson June 2021
 */
class modal_inharm
{
  public:
    modal_inharm(int n) :n_modes_{n}, modes{new iir_reson[n]} {}
    ~modal_inharm() { delete[] modes; }

    void init(float fs, float fc, int n, float *my_modes, float *gains, float *res, 
	      float mgf = 0, float ifc = 220)
    {
      fs_ = fs;
      fc_ = fc;
      mgf_ = mgf;
      n_modes_ = n;

      for (int i = 0; i < n_modes_; i++) {
	modes_.push_back(my_modes[i]);
	gains_.push_back(gains[i]);
	res_.push_back(res[i]);

	float mode_f = modes_.at(i) * fc_;  
	// dont alias
	if (mode_f > (fs_ / 2)) break;

	float mode_g = gains_.at(i) / pow((i + 1), mgf_);
	float mode_r = res_.at(i);

	modes[i].init(fs_, mode_f, CLAMP(mode_r, 0, MAX_R), mode_g);
      }

      input_filt.init(fs_, ifc);
    }

    float Process(float in)
    {
      float out = 0;
      float in_filt = input_filt.Process(in);
      for (int i = 0; i < n_modes_; i++) {
        out += modes[i].Process(in_filt) / n_modes_;
      }
      //return SGN(out) * (1 - expf(-fabsf(out))); // Holy distortion Batman - what's going on here?
      return CLAMP(out, -1.0, 1.0);
    }

    void update_fc(float fc)
    {
      if (fc != fc_) {
	fc_ = fc;

	for (int i = 0; i < n_modes_; i++) {
      	  float mode_f = modes_.at(i) * fc_;  
      	  // dont alias
      	  if (mode_f > (fs_ / 2)) break;
      	  modes[i].update_fc(mode_f);
      	}
      }
    }
    
    void update_r(float *res)
    {
      for (int i = 0; i < n_modes_; i++) {
	float r = res[i];
	if (r != res_.at(i)) {
	  res_.at(i) = r;
      	  modes[i].update_r(res_.at(i));
	}
      }
    }

    // keeps res_.at(i) as the baseline and increases
    // amt should be between 0 and 1 where 0 is baseline and 1 is MAX_R
    void modulate_r(float amt)
    {
      for (int i = 0; i < n_modes_; i++) {
	float r = res_.at(i) + amt * (MAX_R - res_.at(i));
	modes[i].update_r(r);
      }
    }

    /*
    void update_g(float g)
    {
      if (g != g_) {
	g_ = g;

	int calculated_modes = 0;
      	for (int i = 0; calculated_modes < n_modes_; i++) {

      	  // skip modes defined by beta
      	  if (fmod(i, beta_) == 0) continue;

	  float mode_g = g_ / pow((i + 1), mgf_);

      	  modes[calculated_modes].update_g(mode_g);
      	  calculated_modes++;
      	}
      }
    }


    void update_mgf(float mgf)
    {
      if (mgf != mgf_) {
	mgf_ = mgf;

	int calculated_modes = 0;
      	for (int i = 0; calculated_modes < n_modes_; i++) {

      	  // skip modes defined by beta
      	  if (fmod(i, beta_) == 0) continue;

      	  float mode_g = g_ / pow((i + 1), mgf_);
      	  modes[calculated_modes].update_g(mode_g);
      	  calculated_modes++;
      	}
      }
    }
    */

    void update_ifc(float ifc)
    {
      input_filt.update_fc(ifc);
    }

  private:
    int n_modes_;
    iir_reson *modes;
    iir_1p_lp input_filt;
    float fs_, fc_, mgf_;
    std::vector<float> modes_, gains_, res_;
};
} // namespace daisysp
#endif
#endif

