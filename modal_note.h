#pragma once
#ifndef DSY_MODAL_NOTE_H
#define DSY_MODAL_NOTE_H

// Maximum resonance - let's try and keep things stable
#define MAX_R 0.99999

#define CLAMP(x, min, max)  (x > max) ? max : ((x < min) ? min : x)
//#define SGN(x)		    (x > 0) ? 1 : ((x < 0) ? -1 : 0)
#define SGN(x)		    signbit(x) ? -1 : 1

#include <stdint.h>
#include "arm_math.h"
#include "iir_reson.h"
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

    void init(float fs, float fc, float r, float gdb = 0, float stiffness = 0.00001, int beta = 2, 
	      float mgf = 0, float mrf = 0)
    {
      fs_ = fs;
      fc_ = fc;
      r_ = r;
      gdb_ = gdb;
      g_ = powf(10, gdb_ / 20.0);
      stiffness_ = stiffness;
      beta_ = beta;
      mgf_ = mgf;
      mrf_ = mrf;

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

	modes[calculated_modes].init(fs_, mode_f, CLAMP(mode_r, 0, MAX_R), mode_g);
	calculated_modes++;
      }
      n_modes_ = calculated_modes;
    }

    float Process(float in)
    {
      float out = 0;
      for (int i = 0; i < n_modes_; i++) {
	out += modes[i].Process(in) / n_modes_;
      }
      //return SGN(out) * (1 - expf(-fabsf(out))); // Holy distortion Batman - what's going on here?
      return CLAMP(out, -1, 1);
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

    /* TODO:
     * Add a way to change the note, either fc, r or g
     * Add chords
     */

  private:
    int n_modes_;
    iir_reson *modes;
    float fs_, fc_, r_, gdb_, g_, stiffness_, mgf_, mrf_;
    int beta_;

};
} // namespace daisysp
#endif
#endif

