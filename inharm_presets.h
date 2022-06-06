#pragma once
#ifndef DSY_INHARM_PRESETS_H
#define DSY_INHARM_PRESETS_H

#define  NUM_INHARM_PARTIALS  5

#ifdef __cplusplus

/*
 * A set of inharmonic modal presets
 * Mostly cribbed from Perry Cook's Real Sound Synthesis book and
 * https://github.com/thestk/stk/blob/master/src/ModalBar.cpp
 */

typedef struct {
  int num_modes;
  float modes[NUM_INHARM_PARTIALS];
  float res[NUM_INHARM_PARTIALS];
  float gains[NUM_INHARM_PARTIALS];
} inharm_preset;

#define NUM_INHARM_PRESETS 10
inharm_preset inharm_presets[NUM_INHARM_PRESETS] = {
  // Drum thing
  {5, {1.0, 1.58, 2.0, 2.24, 2.92}, {0.9997, 0.9997, 0.9997, 0.9997, 0.9997}, {1, 1, 1, 1, 1}},
  // Marimba
  {4, {1.0, 3.99, 10.65, -2443}, {0.9996, 0.9994, 0.9994, 0.999}, {1, 0.25, 0.25, 0.2}},
  // Vibraphone
  {4, {1.0, 2.01, 3.9, 14.37}, {0.99995, 0.99991, 0.99992, 0.9999}, {1, 0.6, 0.6, 0.6 }},
  // Agogo
  {4, {1.0, 4.08, 6.669, -3725.0}, {0.999, 0.999, 0.999, 0.999}, {1, 0.83333, 0.5, 0.33333}},
  // Wood1
  {4, {1.0, 2.777, 7.378, 15.377}, {0.996, 0.994, 0.994, 0.99}, {1, 0.25, 0.25, 0.2}},
  // Wood2
  {4, {1.0, 1.777, 2.378, 3.377}, {0.996, 0.994, 0.994, 0.99}, {1, 0.25, 0.25, 0.2}},
  // Reso
  {4, {1.0, 2.777, 7.378, 15.377}, {0.99996, 0.99994, 0.99994, 0.9999}, {1, 0.25, 0.25, 0.2}},
  // Beats
  {4, {1.0, 1.004, 1.013, 2.377}, {0.9999, 0.9999, 0.9999, 0.999}, {1, 0.25, 0.25, 0.2}},
  // 2Fix
  {4, {1.0, 4.0, -1320.0, -3960.0}, {0.9996, 0.999, 0.9994, 0.999}, {1, 0.25, 0.25, 0.2}},
  // Clump
  {4, {1.0, 1.217, 1.475, 1.729}, {0.999, 0.999, 0.999, 0.999}, {1, 1, 1, 1}}
};
#endif
#endif
