#pragma once

// Single home for the constants and small utility macros shared across
// modal_note.h, modal_inharm.h and ModalResonators.cpp, so parameter
// defaults/ranges live in one place instead of being redefined per file.

// ---- Resonance ----
// Maximum resonance - let's try and keep things stable
#define RES_MAX 0.99999f
#define RES_MIN 0.99333f

// ---- Strike position ----
#define POS_DEFAULT 0.1f
#define POS_MIN     0.02f
#define POS_MAX     0.98f

// ---- Strike width ----
#define WIDTH_DEFAULT 0.0f
#define WIDTH_MIN     0.0f
#define WIDTH_MAX     0.3f

// ---- Stiffness (inharmonicity) ----
#define STIFF_DEFAULT 0.00001f
#define STIFF_MIN     0.0f
#define STIFF_MAX     0.005f

// ---- Beta (pluck/strike exponent) ----
#define BETA_DEFAULT 1
#define BETA_MIN     1
#define BETA_MAX     5

// ---- Mode gain factor ----
#define MGF_DEFAULT 0
#define MGF_MIN     -1
#define MGF_MAX     3

// ---- Gain ----
#define GAIN_DEFAULT 5
#define GAIN_MIN     0.0f
#define GAIN_MAX     10.0f

// ---- Input filter cutoff (UI-facing, global) ----
#define IFC_DEFAULT 220
#define IFC_MIN     10
#define IFC_MAX     22000

// ---- Input filter cutoff used internally by modal_note/modal_inharm ----
// Deliberately distinct from IFC_DEFAULT above - this seeds each voice's
// own input filter and is unrelated to the UI's IFC parameter default.
#define INPUT_FILT_IFC_DEFAULT 10

// ---- Note gain (dB) ----
#define GDB_DEFAULT 0.0f

// ---- Envelope ----
#define ENV_DEFAULT 0.015f
#define ENV_MIN     0.001f
#define ENV_MAX     0.1f

// ---- LFOs ----
#define NUM_LFOS         3
#define LFO_RATE_DEFAULT 0.3f
#define LFO_RATE_MIN     0.0f
#define LFO_RATE_MAX     60.0f
#define LFO_DEPTH_MIN    0.0f
#define LFO_DEPTH_MAX    1.0f
#define LFO_IFC	         0
#define LFO_STIFF        1
#define LFO_BETA         2

// ---- Voice/partial counts ----
#define NUM_HARM_PARTIALS 4
#define NUM_NOTES         5

// ---- Misc ----
#define PING_AMT     1 //0.25
#define PARAM_THRESH 0.05f

// ---- MIDI ----
#define MIDI_CHANNEL   0 // todo - make this settable somehow. Daisy starts counting MIDI channels from 0
#define CC_MOD	       1
#define CC_GAIN        7
#define CC_IFC	       14
#define CC_STIFF       70
#define CC_BETA        71
#define CC_REL         72
#define CC_ATK         73
#define CC_MGF	       74
#define CC_MODE        75
#define CC_INHARM      76
#define CC_POS	       77
#define CC_WIDTH       78
#define CC_LFO_IFC_R   85
#define CC_LFO_IFC_D   86
#define CC_LFO_STIFF_R 87
#define CC_LFO_STIFF_D 88
#define CC_LFO_BETA_R  89
#define CC_LFO_BETA_D  90

// ---- Math constants ----
#define INV_ARCTAN_1 1.273239544735163f
#define INV_TANH_1   1.313035285499331f

// ---- Utility macros ----
// Note: MIDI CC -> value conversion (CC_TO_VAL) lives in PagedParam.h, not
// here, since that macro belongs to PagedParam regardless of this sketch.
#define CLAMP(x, min, max)       ((x) > max) ? max : (((x) < min) ? min : x)
#define SGN(x)                   (signbit(x) ? -1.0 : 1.0)
#define POT_TO_VAL(x, min, max)  (min + x * (max - min))
#define VAL_TO_POT(x, min, max)  ((x - min) / (max - min))
