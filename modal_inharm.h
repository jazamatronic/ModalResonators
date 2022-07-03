#pragma once
#ifndef DSY_MODAL_INHARM_H
#define DSY_MODAL_INHARM_H

// Maximum resonance - let's try and keep things stable
#define RES_MAX 0.99999

// Match this to ModalResonators.cpp
#define GAIN_MAX  10

#define CLAMP(x, min, max)  (x > max) ? max : ((x < min) ? min : x)

#include <stdint.h>
#include "arm_math.h"
#include "iir_reson.h"
#include "iir_1p_lp.h"
#include "inharm_presets.h"
#ifdef __cplusplus

namespace daisysp
{
/*
 * An inharmonic modal voice is constructed from a set of modes
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

    void init(float fs, float fc, inharm_preset *preset)
    {
      fs_ = fs;
      fc_ = fc;
      mgf_ = DEFAULT_MGF;
      n_modes_ = preset->num_modes;

      for (int i = 0; i < n_modes_; i++) {
	modes_.push_back(preset->modes[i]);
	gains_.push_back(preset->gains[i]);
	res_.push_back(preset->res[i]);

	float mode_f;
	if (modes_.at(i) > 0) {
	  mode_f = modes_.at(i) * fc_;  
	} else {
	  mode_f = -modes_.at(i);
	}
	// dont alias
	if (mode_f > (fs_ / 2)) {
	  n_modes_ = i;
	  break;
	}

	float mode_g = gains_.at(i) / pow((i + 1), mgf_);
	float mode_r = res_.at(i);

	modes[i].init(fs_, mode_f, CLAMP(mode_r, 0, RES_MAX), mode_g);
      }

      input_filt.init(fs_, DEFAULT_IFC);
    }

    void load_preset(inharm_preset *preset)
    {
      int i;
      n_modes_ = preset->num_modes;
      for (i = 0; i < n_modes_; i++) {
	modes_.at(i) = preset->modes[i];
	gains_.at(i) = preset->gains[i];
	res_.at(i) = preset->res[i];

	float mode_f;
	if (modes_.at(i) > 0) {
	  mode_f = modes_.at(i) * fc_;  
	} else {
	  mode_f = -modes_.at(i);
	}
	// dont alias
	if (mode_f > (fs_ / 2)) {
	  n_modes_ = i;
	  break;
	}

	float mode_g = gains_.at(i) / pow((i + 1), mgf_);
	float mode_r = res_.at(i);

	modes[i].update_fc(mode_f);
	modes[i].update_r(CLAMP(mode_r, 0, RES_MAX));
	modes[i].update_g(mode_g);
      }
    }

    float Process(float in)
    {
      float out = 0;
      float in_filt = input_filt.Process(in);
      for (int i = 0; i < n_modes_; i++) {
        out += modes[i].Process(in_filt) / n_modes_;
      }
      // Let's do clamping after summing in the top level
      return out;
    }

    void update_fc(float fc)
    {
      int i;
      if (fc != fc_) {
	fc_ = fc;

	for (i = 0; i < n_modes_; i++) {
      	  float mode_f;
	  if (modes_.at(i) > 0) {
	    mode_f = modes_.at(i) * fc_;  
	  } else {
	    mode_f = -modes_.at(i);  
	  }
      	  // dont alias
      	  if (mode_f > (fs_ / 2)) { 
	    n_modes_ = i;
	    break;
	  }
	
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
    // amt should be between 0 and 1 where 0 is baseline and 1 is RES_MAX
    void modulate_r(float amt)
    {
      for (int i = 0; i < n_modes_; i++) {
	float r = res_.at(i) + amt * (RES_MAX - res_.at(i));
	modes[i].update_r(r);
      }
    }

    // keeps gains_.at(i) as the baseline and increases
    // amt should be between 0 and 1 where 0 is baseline and 1 is GAIN_MAX
    void modulate_g(float amt)
    {
      for (int i = 0; i < n_modes_; i++) {
	float g = gains_.at(i) + amt * (GAIN_MAX - gains_.at(i));
	modes[i].update_g(g);
      }
    }


    /*
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

