#pragma once
#ifndef DSY_TRI_LFO_H
#define DSY_TRI_LFO_H
#ifdef __cplusplus

#include <math.h>

namespace daisysp
{
/*
 * Triangle LFO lifted from Daisy DaisySP/Source/Effects/flanger.h and modified 
 */

class tri_lfo
{
  public:
    /** Initialize the modules
        \param sample_rate Audio engine sample rate.
    */
    void Init(float sample_rate)
    {
      sample_rate_ = sample_rate;

      phase_ = 0.f;
      depth_ = 0.f;
      range_ = 0.f;
      out_ = 0.f;
      SetFreq(.3);
    }

    void SetDepth(float depth)
    {
      depth_ = fclamp(depth, 0.f, 1.f);
    }

    void SetRange(float range)
    {
      range_ = range;
    }

    /** Set lfo frequency.
        \param freq Frequency in Hz
    */
    void SetFreq(float freq)
    {
      freq = 4.f * freq / sample_rate_;
      freq *= freq_ < 0.f ? -1.f : 1.f;  //if we're headed down, keep going
      freq_ = fclamp(freq, -.25f, .25f); //clip at +/- .125 * sr
    }

    float GetOutput() {
      return out_;
    }

    void Process()
    {
      phase_ += freq_;

      //wrap around and flip direction
      if(phase_ > 1.f)
      {
          phase_ = 1.f - (phase_ - 1.f);
          freq_ *= -1.f;
      }
      else if(phase_ < -1.f)
      {
          phase_ = -1.f - (phase_ + 1.f);
          freq_ *= -1.f;
      }

      out_ = phase_ * (range_ * depth_);
    }

  private:
    float sample_rate_;

    //triangle lfos
    float phase_;
    float freq_;
    float depth_;
    float range_;

    float out_;
};
}
#endif
#endif
