#pragma once
#ifndef DSY_MODAL_NOTE_H
#define DSY_MODAL_NOTE_H

#include "modal_defs.h"

#include <stdint.h>
#include <memory>
#include "arm_math.h"
#include "iir_reson.h"
#include "iir_1p_lp.h"
#ifdef __cplusplus

namespace daisysp
{
/*
 * A modal note is constructed from a set of modes
 * Specify the fundamental frequency and number of modes as well as different stiffness, pluck position 
 * and gain/resonance factors 
 *
 *   Jared Anderson May 2021
 */
class modal_note
{
  public:
    modal_note(int n) :max_modes_{n}, n_modes_{n}, modes{new iir_reson[n]}, mode_i{new int[n]}  {}

    void init(float fs, float fc, float r)
    {
      fs_ = fs;
      fc_ = fc;
      r_ = r;
      gdb_ = GDB_DEFAULT;
      g_ = powf(10, gdb_ / 20.0);
      pos_ = POS_DEFAULT;
      width_ = WIDTH_DEFAULT;
      stiffness_ = STIFF_DEFAULT;
      beta_ = BETA_DEFAULT;
      mgf_ = MGF_DEFAULT;
      mrf_ = 0;

      int calculated_modes = 0;
      for (int i = 0; calculated_modes < max_modes_; i++) {

	// skip modes defined by beta
	if ((beta_ > 1) && (fmod(i, beta_) == 0)) continue;
	
	float mode_f = (i + 1) * fc_ * sqrt(1 + stiffness_ * pow(i, 2));  
	// dont alias
	if (mode_f > (fs_ / 2)) {
	  calculated_modes++;
	  break;
	}

        // do this in recompute_gains now we have position based gains
	//float mode_g = g_ / pow((i + 1), mgf_);
	float mode_g = 0.0f;
	
	float mode_r = r_ - i * mrf_;
	if (mode_r < 0) mode_r = 0;
	
	modes[calculated_modes].init(fs_, mode_f, CLAMP(mode_r, 0, RES_MAX), mode_g);
	mode_i[calculated_modes] = i;
	calculated_modes++;
      }
      n_modes_ = calculated_modes;
      recompute_gains();

      input_filt.init(fs_, INPUT_FILT_IFC_DEFAULT);
    }

    float Process(float in)
    {
      float out = 0;
      float in_filt = input_filt.Process(in);
      for (int i = 0; i < n_modes_; i++) {
        out += modes[i].Process(in_filt) / n_modes_;
      }
      // Let's do any clamping after summing in the top level
      return out;
    }

    void update_fc(float fc)
    {
      if (fc != fc_) {
	fc_ = fc;

	int calculated_modes = 0;
      	for (int i = 0; calculated_modes < max_modes_; i++) {

      	  // skip modes defined by beta
	if ((beta_ > 1) && (fmod(i, beta_) == 0)) continue;

      	  float mode_f = (i + 1) * fc_ * sqrt(1 + stiffness_ * pow(i, 2));  

      	  // dont alias
      	  if (mode_f > (fs_ / 2)) {
	    calculated_modes++;
	    break;
	  }

      	  modes[calculated_modes].update_fc(mode_f);
          mode_i[calculated_modes] = i;
      	  calculated_modes++;
      	}
	n_modes_ = calculated_modes;
        recompute_gains();
      }
    }

    void update_r(float r)
    {
      if (r != r_) {
	r_ = r;

        int calculated_modes = 0;
        for (int i = 0; calculated_modes < n_modes_; i++) {
  
	  // skip modes defined by beta
	  if ((beta_ > 1) && (fmod(i, beta_) == 0)) continue;
  
  	  float mode_r = r_ - i * mrf_;
  	  if (mode_r < 0) mode_r = 0;
      	  modes[calculated_modes].update_r(mode_r);
      	  calculated_modes++;
      	}
      }
    }

    void update_g(float g)
    {
      if (g != g_) {
	g_ = g;

        recompute_gains();
      }
    }

    void update_pos(float pos)
    {
      if (pos != pos_) {
	pos_ = pos;

        recompute_gains();
      }
    }

    void update_width(float width)
    {
      if (width != width_) {
	width_ = width;

        recompute_gains();
      }
    }

    void update_stiffness(float stiffness)
    {
      if (stiffness != stiffness_) {
	stiffness_ = stiffness;

	int calculated_modes = 0;
	for (int i = 0; calculated_modes < max_modes_; i++) {
  
	  // skip modes defined by beta
	  if ((beta_ > 1) && (fmod(i, beta_) == 0)) continue;
  	  
  	  float mode_f = (i + 1) * fc_ * sqrt(1 + stiffness_ * pow(i, 2));  
  	  // dont alias
  	  if (mode_f > (fs_ / 2)) {
	    calculated_modes++;
	    break;
	  }

      	  modes[calculated_modes].update_fc(mode_f);
          mode_i[calculated_modes] = i;
      	  calculated_modes++;
	}
	n_modes_ = calculated_modes;
        recompute_gains();
      }
    }

    void update_beta(int beta)
    {
      if (beta != beta_) {
	beta_ = beta;

	int calculated_modes = 0;
      	for (int i = 0; calculated_modes < max_modes_; i++) {

      	  // skip modes defined by beta
	  if ((beta_ > 1) && (fmod(i, beta_) == 0)) continue;

      	  float mode_f = (i + 1) * fc_ * sqrt(1 + stiffness_ * pow(i, 2));  

      	  // dont alias
      	  if (mode_f > (fs_ / 2)) {
	    calculated_modes++;
	    break;
	  }

      	  modes[calculated_modes].update_fc(mode_f);
          mode_i[calculated_modes] = i;
      	  calculated_modes++;
      	}
	n_modes_ = calculated_modes;
        recompute_gains();
      }
    }

    void update_mgf(float mgf)
    {
      if (mgf != mgf_) {
	mgf_ = mgf;

        recompute_gains();
      }
    }

    void update_ifc(float ifc)
    {
      input_filt.update_fc(ifc);
    }

    /* TODO:
     * Add chords
     */

  private:
    const int max_modes_;
    int n_modes_;
    std::unique_ptr<iir_reson[]> modes;
    std::unique_ptr<int[]> mode_i;
    iir_1p_lp input_filt;
    float fs_, fc_, r_, gdb_, g_, pos_, width_, stiffness_, mgf_, mrf_;
    int beta_;

    float sinc(float x)
    {
      return (x == 0.0f) ? 1.0f : sinf(x) / x;
    }

    void recompute_gains() {

      int i, n;
      float p, w;

      for (i = 0; i < n_modes_; i++) {
        n = mode_i[i] + 1;
        p = sinf(n * PI * pos_); // strike position modelling
        w = sinc(n * PI * width_ / 2); // strike width modelling
	float mode_g = g_ * p * w / pow(n, mgf_);
        modes[i].update_g(mode_g);
      }

    }

};
} // namespace daisysp
#endif
#endif

