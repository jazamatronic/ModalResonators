#pragma once
#ifndef DSY_MODAL_NOTE_H
#define DSY_MODAL_NOTE_H

// Maximum resonance - let's try and keep things stable
#define RES_MAX 0.99999

#define DEFAULT_GDB   0
#define DEFAULT_STIFF 0.00001
#define DEFAULT_BETA  2
#define DEFAULT_MGF   0
#define DEFAULT_IFC   10

#define CLAMP(x, min, max)  (x > max) ? max : ((x < min) ? min : x)

#include <stdint.h>
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
    modal_note(int n) :n_modes_{n}, modes{new iir_reson[n]} {}
    ~modal_note() { delete[] modes; }

    void init(float fs, float fc, float r)
    {
      fs_ = fs;
      fc_ = fc;
      r_ = r;
      gdb_ = DEFAULT_GDB;
      g_ = powf(10, gdb_ / 20.0);
      stiffness_ = DEFAULT_STIFF;
      beta_ = DEFAULT_BETA;
      mgf_ = DEFAULT_MGF;
      mrf_ = 0;

      int calculated_modes = 0;
      for (int i = 0; calculated_modes < n_modes_; i++) {

	// skip modes defined by beta
	if (fmod(i, beta_) == 0) continue;

	float mode_f = (i + 1) * fc_ * sqrt(1 + stiffness_ * pow(i, 2));  
	// dont alias
	if (mode_f > (fs_ / 2)) break;

	float mode_g = g_ / pow((i + 1), mgf_);

	float mode_r = r_ - i * mrf_;
	if (mode_r < 0) mode_r = 0;

	modes[calculated_modes].init(fs_, mode_f, CLAMP(mode_r, 0, RES_MAX), mode_g);
	calculated_modes++;
      }
      n_modes_ = calculated_modes;

      input_filt.init(fs_, DEFAULT_IFC);
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
      	for (int i = 0; calculated_modes < n_modes_; i++) {

      	  // skip modes defined by beta
      	  if (fmod(i, beta_) == 0) continue;

      	  float mode_f = (i + 1) * fc_ * sqrt(1 + stiffness_ * pow(i, 2));  

      	  // dont alias
      	  if (mode_f > (fs_ / 2)) break;

      	  modes[calculated_modes].update_fc(mode_f);
      	  calculated_modes++;
      	}
      }
    }

    void update_r(float r)
    {
      if (r != r_) {
	r_ = r;

        int calculated_modes = 0;
        for (int i = 0; calculated_modes < n_modes_; i++) {
  
	  // skip modes defined by beta
  	  if (fmod(i, beta_) == 0) continue;
  
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

    void update_stiffness(float stiffness)
    {
      if (stiffness != stiffness_) {
	stiffness_ = stiffness;

	int calculated_modes = 0;
	for (int i = 0; calculated_modes < n_modes_; i++) {
  
	  // skip modes defined by beta
  	  if (fmod(i, beta_) == 0) continue;
  	  
  	  float mode_f = (i + 1) * fc_ * sqrt(1 + stiffness_ * pow(i, 2));  
  	  // dont alias
  	  if (mode_f > (fs_ / 2)) break;

      	  modes[calculated_modes].update_fc(mode_f);
      	  calculated_modes++;
	}
      }
    }

    void update_beta(int beta)
    {
      if (beta != beta_) {
	beta_ = beta;

	int calculated_modes = 0;
      	for (int i = 0; calculated_modes < n_modes_; i++) {

      	  // skip modes defined by beta
      	  if (fmod(i, beta_) == 0) continue;

      	  float mode_f = (i + 1) * fc_ * sqrt(1 + stiffness_ * pow(i, 2));  

      	  // dont alias
      	  if (mode_f > (fs_ / 2)) break;

      	  modes[calculated_modes].update_fc(mode_f);
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

    void update_ifc(float ifc)
    {
      input_filt.update_fc(ifc);
    }

    /* TODO:
     * Add chords
     */

  private:
    int n_modes_;
    iir_reson *modes;
    iir_1p_lp input_filt;
    float fs_, fc_, r_, gdb_, g_, stiffness_, mgf_, mrf_;
    int beta_;

};
} // namespace daisysp
#endif
#endif

