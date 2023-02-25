#include "daisy_pod.h"
#include "daisysp.h"
#include "modal_note.h"
#include "modal_inharm.h"
#include "crc_noise.h"
#include "led_colours.h"
#include "tri_lfo.h"
#include "PagedParam.h"

#define NUM_HARM_PARTIALS   4
#define NUM_NOTES	    5

#define NUM_LFOS      3
#define LFO_RATE_DEFAULT 0.3
#define LFO_RATE_MIN  0
#define LFO_RATE_MAX  60
#define LFO_DEPTH_MIN 0.0f
#define LFO_DEPTH_MAX 1.0f
#define LFO_IFC	      0
#define LFO_STIFF     1
#define LFO_BETA      2

#define PING_AMT      	    1 //0.25 

#define PARAM_THRESH	  0.05f

#define RES_MIN	  0.99333
#define RES_MAX   0.99999
#define IFC_DEFAULT 220
#define IFC_MIN   10
#define IFC_MAX   22000
#define GAIN_DEFAULT 0.5f
#define GAIN_MIN  0.0f
#define STIFF_MIN 0
#define STIFF_MAX 0.005 
#define BETA_MIN  2
#define BETA_MAX  5
#define MGF_DEFAULT 0
#define MGF_MIN	  -1
#define MGF_MAX   3
#define ENV_DEFAULT 0.015
#define ENV_MIN	  0.001
#define ENV_MAX	  0.1


#define SGN(x)		    (signbit(x) ? -1.0 : 1.0)

#define CC_TO_VAL(x, min, max) (min + (x / 127.0f) * (max - min))
#define POT_TO_VAL(x, min, max) (min + x * (max - min))
#define VAL_TO_POT(x, min, max) ((x - min) / (max - min))

#define MIDI_CHANNEL	0 // todo - make this settable somehow. Daisy starts counting MIDI channels from 0
#define CC_MOD	       	1
#define CC_GAIN       	7
#define	CC_IFC		14
#define CC_STIFF      	70
#define CC_BETA       	71
#define CC_REL        	72
#define CC_ATK        	73
#define CC_MGF	       	74
#define CC_MODE		75
#define CC_INHARM	76
#define CC_LFO_IFC_R  	85
#define CC_LFO_IFC_D  	86
#define CC_LFO_STIFF_R	87
#define CC_LFO_STIFF_D	88
#define CC_LFO_BETA_R	89
#define CC_LFO_BETA_D	90

// For distortion models
#define INV_ARCTAN_1 1.273239544735163
#define INV_TANH_1   1.313035285499331

using namespace daisy;
using namespace daisysp;

DaisyPod hw;

modal_note *notes[NUM_NOTES];
modal_inharm *inharms[NUM_NOTES];

AdEnv env[NUM_NOTES];
crc_noise noise;

tri_lfo lfos[NUM_LFOS];

int  blink_mask = 511; 
int  blink_cnt = 0;
bool led_state = true;

static Parameter knob1_lin, knob1_log, knob2_lin, knob2_log;
PagedParam ifc_p, g_p, inharm_g_p, stiff_p, beta_p, mgf_p, mrf_p, out_p, at_p, dt_p;
PagedParam lfo_ifc_rate_p, lfo_ifc_depth_p, lfo_stiff_rate_p, lfo_stiff_depth_p, lfo_beta_rate_p, lfo_beta_depth_p;
float new_ifc, new_g, new_stiff, new_beta, new_mgf, new_mrf, new_out, new_at, new_dt;
float new_lfo_ifc_rate, new_lfo_ifc_depth, new_lfo_stiff_rate, new_lfo_stiff_depth, new_lfo_beta_rate, new_lfo_beta_depth;
float cur_beta, cur_ifc, cur_stiff;

int midi_f = 0;
float midi_v = 0;
int next_note = 0;
bool play_note = false;

typedef enum {MIDI = 0, GAIN_OUT, STIFF_BETA, STIFF_LFO, BETA_LFO, IFC_MGF, IFC_LFO, AD, LAST_PAGE} ui_page;
ui_page cur_page = MIDI;
typedef enum {PING = 0, NOISE_ENV, EXT, EXT_ENV, INHARM, INHARM_NOISE, LAST_MODE} ui_mode;
ui_mode cur_mode = PING;
typedef enum {NONE = 0, EXP_DIST, TANH, ARCTAN, LAST_OUTPUT} ui_output_mode;
ui_output_mode cur_output_mode = NONE;
int cur_preset = 0;


void UpdateEncoder();
void UpdateButtons();
void UpdateParams();
void SetLedMode();

void AudioCallback(AudioHandle::InputBuffer in, AudioHandle::OutputBuffer out, size_t size)
{

	UpdateParams();

	lfos[LFO_IFC].Process();
	lfos[LFO_STIFF].Process();
	lfos[LFO_BETA].Process();

	float to_in = 0;

	for (size_t i = 0; i < size; i++)
	{
	  float to_out = 0;
	  for (int j = 0; j < NUM_NOTES; j++) {
	    if (cur_mode == EXT || cur_mode == EXT_ENV) {
	      to_in = in[0][i];
	    } else {
	      to_in = 0;
	      if (play_note && (j == next_note)) {
	        to_in = PING_AMT;
	        play_note = false;
	        if (++next_note == NUM_NOTES) {
	          next_note = 0;
	        }
		if (cur_mode == NOISE_ENV || cur_mode == INHARM_NOISE) {
		  env[j].Trigger();
		}
	      }
	    }
	    if (cur_mode == NOISE_ENV || cur_mode == INHARM_NOISE) {
	      to_in = env[j].Process() * noise.Process();
	    } else if (cur_mode == EXT_ENV) {
	      to_in = env[j].Process() * to_in;
	    }
	    if (cur_mode == INHARM || cur_mode == INHARM_NOISE) {
	      to_out += inharms[j]->Process(to_in) / NUM_NOTES;
	    } else {
	      to_out += notes[j]->Process(to_in) / NUM_NOTES;
	    }
	  }

	  // no effort made here to avoid aliasing due to harmonics introduced by waveshaping/clipping
	  switch(cur_output_mode) {
	    case NONE:
	      break;
	    case EXP_DIST:
	      to_out = SGN(to_out) * (1 - expf(-fabsf(to_out))); // Holy distortion Batman - what's going on here?
	      break;
	    case TANH:
	      to_out = tanhf(to_out) * INV_TANH_1;
	      break;
	    case ARCTAN:
	      to_out = atanf(to_out) * INV_ARCTAN_1;
	      break;
	    default:
	      break;
	  }

	  out[0][i] = to_out;
	  out[1][i] = to_out;
	} 
} 

void HandleMidiMessage(MidiEvent m) { 
   if (m.channel != MIDI_CHANNEL) { return; } //Broken - no, it just looks like 0 is channel 1? 
   switch(m.type) { 
     case NoteOn: {
	  NoteOnEvent this_note = m.AsNoteOn();
	  midi_f = mtof(this_note.note);
	  // TODO: fix velocity 
	  if (cur_mode == INHARM || cur_mode == INHARM_NOISE) {
	    midi_v = CC_TO_VAL(this_note.velocity, 0, 1);
	    inharms[next_note]->modulate_g(midi_v);
	    inharms[next_note]->update_fc(midi_f);
	  } else {
	    midi_v = CC_TO_VAL(this_note.velocity, 0, new_g);
	    notes[next_note]->update_g(midi_v);
	    notes[next_note]->update_fc(midi_f);
	  }
	  play_note = true;
          break;
	}
        case ControlChange:
        {
	  ControlChangeEvent p = m.AsControlChange();
	  switch(p.control_number)
	  {
	    case CC_MOD: 
	      {
	        if (cur_mode == INHARM || cur_mode == INHARM_NOISE) {
	          float new_res = CC_TO_VAL(p.value, -1, 1);
	          for (int i = 0; i < NUM_NOTES; i++) {
	            inharms[i]->modulate_r(new_res);
	          }
	        } else {
	          float new_res = CC_TO_VAL(p.value, RES_MIN, RES_MAX);
	          for (int i = 0; i < NUM_NOTES; i++) {
	            notes[i]->update_r(new_res);
	          }
	        }
	        break;
	      }
	    case CC_GAIN: 
	      if (cur_mode == INHARM || cur_mode == INHARM_NOISE) {
	        new_g = inharm_g_p.MidiCCIn(p.value); // inharmonic gain is really a modulation factor between 0 and 1
	      } else {
	        new_g = g_p.MidiCCIn(p.value);
	      }
	      break;
	    case CC_STIFF: 
	      new_stiff = stiff_p.MidiCCIn(p.value);
	      break;
	    case CC_BETA: 
	      new_beta = (int)roundf(beta_p.MidiCCIn(p.value));
	      break;
	    case CC_MGF: 
	      new_mgf = mgf_p.MidiCCIn(p.value);
	      break;
	    case CC_REL:
	      new_dt = dt_p.MidiCCIn(p.value);
	      break;
	    case CC_ATK:
	      new_at = at_p.MidiCCIn(p.value);
	      break;
	    case CC_MODE:
	      cur_mode = (ui_mode)floor(CC_TO_VAL(p.value, 0, (LAST_MODE - 0.1))); // - 0.1 to avoid hitting LAST_MODE
	      SetLedMode();
	      break;
	    case CC_INHARM:
	      cur_preset = floor(CC_TO_VAL(p.value, 0, NUM_INHARM_PRESETS));
	      for (int i = 0; i < NUM_NOTES; i++) {
	        inharms[i]->load_preset(&inharm_presets[cur_preset]);
	      }
	      break;
	    case CC_LFO_IFC_R:
	      new_lfo_ifc_rate = lfo_ifc_rate_p.MidiCCIn(p.value);
	      break;
	    case CC_LFO_IFC_D:
	      new_lfo_ifc_depth = lfo_ifc_depth_p.MidiCCIn(p.value);
	      break;
	    case CC_LFO_STIFF_R:
	      new_lfo_stiff_rate = lfo_stiff_rate_p.MidiCCIn(p.value);
	      break;
	    case CC_LFO_STIFF_D:
	      new_lfo_stiff_depth = lfo_stiff_depth_p.MidiCCIn(p.value);
	      break;
	    case CC_LFO_BETA_R:
	      new_lfo_beta_rate = lfo_beta_rate_p.MidiCCIn(p.value);
	      break;
	    case CC_LFO_BETA_D:
	      new_lfo_beta_depth = lfo_beta_depth_p.MidiCCIn(p.value);
	      break;
	    case CC_IFC:
	      new_ifc = ifc_p.MidiCCIn(p.value);
	      break;
	    default: break;
	  }
	  break;
	}
      default: break;
  }
}

void UpdateEncoder()
{

  //float k1_lin, k1_log, k2_lin, k2_log;
  //k1_lin = knob1_lin.Process();
  float k1_log, k2_lin, k2_log;
  k1_log = knob1_log.Process();
  k2_lin = knob2_lin.Process();
  k2_log = knob2_log.Process();

  cur_page = (ui_page)(cur_page + hw.encoder.Increment());
  if (cur_page >= LAST_PAGE) { cur_page = MIDI; }
  switch(cur_page)
  {
    case MIDI:
      // disable controls
      hw.led1.Set(PURPLE);
      break;
    case GAIN_OUT:
      hw.led1.Set(RED);
      break;
    case STIFF_BETA:
      hw.led1.Set(GREEN);
      break;
    case STIFF_LFO:
      hw.led1.Set(LGREEN);
      break;
    case BETA_LFO:
      hw.led1.Set(ORANGE);
      break;
    case IFC_MGF:
      hw.led1.Set(BLUE);
      break;
    case IFC_LFO:
      hw.led1.Set(LBLUE);
      break;
    case AD:
      hw.led1.Set(YELLOW);
      break;
    default: break;
  } 

  if (cur_mode == INHARM || cur_mode == INHARM_NOISE) {
    new_g = inharm_g_p.Process(k1_log, cur_page); // inharmonic gain is really a modulation factor between 0 and 1
  } else {
    new_g = g_p.Process(k1_log, cur_page);
  }
  new_out = roundf(out_p.Process(k2_lin, cur_page));
  new_stiff = stiff_p.Process(k1_log, cur_page);
  new_beta = (int)roundf(beta_p.Process(k2_lin, cur_page));
  new_ifc = ifc_p.Process(k1_log, cur_page);
  new_mgf = mgf_p.Process(k2_log, cur_page);
  new_at = at_p.Process(k1_log, cur_page);
  new_dt = dt_p.Process(k2_log, cur_page);

  new_lfo_stiff_rate = lfo_stiff_rate_p.Process(k1_log, cur_page);
  new_lfo_stiff_depth = lfo_stiff_depth_p.Process(k2_lin, cur_page);
  new_lfo_beta_rate = lfo_beta_rate_p.Process(k1_log, cur_page);
  new_lfo_beta_depth = lfo_beta_depth_p.Process(k2_lin, cur_page);
  new_lfo_ifc_rate = lfo_ifc_rate_p.Process(k1_log, cur_page);
  new_lfo_ifc_depth = lfo_ifc_depth_p.Process(k2_lin, cur_page);
}

void SetLedMode()
{
  switch(cur_mode) {
    case PING:
      hw.led2.Set(OFF);
      break;
    case NOISE_ENV:
      hw.led2.Set(PURPLE);
      break;
    case EXT:
      hw.led2.Set(CYAN);
      break;
    case EXT_ENV:
      hw.led2.Set(BLUE);
      break;
    case INHARM:
      hw.led2.Set(YELLOW);
      break;
    case INHARM_NOISE:
      hw.led2.Set(WHITE);
      break;
    default:
      break;
  }
}

void UpdateButtons()
{
  if(hw.button1.RisingEdge()) {
    if (++cur_preset == NUM_INHARM_PRESETS) {
      cur_preset = 0;
    }
    for (int i = 0; i < NUM_NOTES; i++) {
      inharms[i]->load_preset(&inharm_presets[cur_preset]);
    }
  }

  if(hw.button2.RisingEdge()) {
    cur_mode = (ui_mode)(cur_mode + 1);
    if (cur_mode >= LAST_MODE) {
      cur_mode = PING;
    }
    SetLedMode();
  }
}

void UpdateParams() 
{
  if (lfo_ifc_rate_p.Changed()) {
    lfos[LFO_IFC].SetFreq(new_lfo_ifc_rate);
  }
  if (lfo_ifc_depth_p.Changed()) {
    lfos[LFO_IFC].SetDepth(new_lfo_ifc_depth);
  }
  float lfo_new_ifc = CLAMP(new_ifc + lfos[LFO_IFC].GetOutput(), IFC_MIN, IFC_MAX);

  if (lfo_stiff_rate_p.Changed()) {
    lfos[LFO_STIFF].SetFreq(new_lfo_stiff_rate);
  }
  if (lfo_stiff_depth_p.Changed()) {
    lfos[LFO_STIFF].SetDepth(new_lfo_stiff_depth);
  }
  float lfo_new_stiff = CLAMP(new_stiff + lfos[LFO_STIFF].GetOutput(), STIFF_MIN, STIFF_MAX);

  if (lfo_beta_rate_p.Changed()) {
    lfos[LFO_BETA].SetFreq(new_lfo_beta_rate);
  }
  if (lfo_beta_depth_p.Changed()) {
    lfos[LFO_BETA].SetDepth(new_lfo_beta_depth);
  }
  float lfo_new_beta = CLAMP(new_beta + lfos[LFO_BETA].GetOutput(), BETA_MIN, BETA_MAX);

  if (out_p.Changed()) {
    cur_output_mode = (ui_output_mode)new_out;
  }

  for (int i = 0; i < NUM_NOTES; i++) {

    if (at_p.Changed()) {
      if (!env[i].IsRunning()) {
        env[i].SetTime(ADSR_SEG_ATTACK, new_at);
      }
    }

    if (dt_p.Changed()) {
      if (!env[i].IsRunning()) {
        env[i].SetTime(ADSR_SEG_DECAY, new_dt);
      }
    }

    if (cur_mode == INHARM || cur_mode == INHARM_NOISE) {
      if (inharm_g_p.Changed()) { 
        inharms[i]->modulate_g(new_g);
      } 
      if (lfo_new_ifc != cur_ifc) { 
	inharms[i]->update_ifc(lfo_new_ifc);
      } 
    } else {
      if (g_p.Changed()) { 
        notes[i]->update_g(new_g);
      }
      if (lfo_new_stiff != cur_stiff) { 
        notes[i]->update_stiffness(lfo_new_stiff);
      }
      if (lfo_new_beta != cur_beta) { 
	notes[i]->update_beta(lfo_new_beta);
      }
      if (mgf_p.Changed()) { 
	notes[i]->update_mgf(new_mgf);
      }
      if (lfo_new_ifc != cur_ifc) { 
	notes[i]->update_ifc(lfo_new_ifc);
      } 
    }
  }
  cur_beta = new_beta;
  cur_ifc = new_ifc;
  cur_stiff = new_stiff;
}


int main(void)
{

	hw.Init();
	float sr = hw.AudioSampleRate();
	float cr = hw.AudioCallbackRate();

	for (int i = 0; i < NUM_NOTES; i++) {
	  notes[i] = new modal_note(NUM_HARM_PARTIALS);
	  notes[i]->init(sr, 45, 0.9999);
	  inharms[i] = new modal_inharm(NUM_INHARM_PARTIALS);
	  inharms[i]->init(sr, 45, &inharm_presets[cur_preset]);

	  env[i].Init(sr);
      	  env[i].SetTime(ADSR_SEG_ATTACK, ENV_DEFAULT);
      	  env[i].SetTime(ADSR_SEG_DECAY, ENV_DEFAULT);
	  env[i].SetCurve(20);
	}

	noise.Init();

	knob1_lin.Init(hw.knob1, 0.0f, 1.0f, knob1_lin.LINEAR); 
	knob1_log.Init(hw.knob1, 0.0f, 1.0f, knob1_log.EXPONENTIAL); 
	knob2_lin.Init(hw.knob2, 0.0f, 1.0f, knob2_lin.LINEAR); 
	knob2_log.Init(hw.knob2, 0.0f, 1.0f, knob2_log.EXPONENTIAL); 
	knob1_lin.Process();
        knob1_log.Process();
        knob2_lin.Process();
        knob2_log.Process();

	hw.led1.Set(PURPLE); // MIDI MODE
	hw.led2.Set(OFF); // PING MODE

	g_p.Init(         (uint8_t)GAIN_OUT,    GAIN_DEFAULT,  GAIN_MIN,   GAIN_MAX,   PARAM_THRESH);
	inharm_g_p.Init(  (uint8_t)GAIN_OUT,    GAIN_DEFAULT,  0.0f,       (LAST_OUTPUT - 1), PARAM_THRESH);
	out_p.Init(       (uint8_t)GAIN_OUT,    0.0f,          0.0f,       1.0f,       PARAM_THRESH);
	at_p.Init(        (uint8_t)AD,          ENV_DEFAULT,   ENV_MIN,    ENV_MAX,    PARAM_THRESH);
	dt_p.Init(        (uint8_t)AD,          ENV_DEFAULT,   ENV_MIN,    ENV_MAX,    PARAM_THRESH);
	ifc_p.Init(       (uint8_t)IFC_MGF,     IFC_DEFAULT,   IFC_MIN,    IFC_MAX,    PARAM_THRESH);
	stiff_p.Init(     (uint8_t)STIFF_BETA,  STIFF_MIN,     STIFF_MIN,  STIFF_MAX,  PARAM_THRESH);
	beta_p.Init(      (uint8_t)STIFF_BETA,  BETA_MIN,      BETA_MIN,   BETA_MAX,   PARAM_THRESH);
	mgf_p.Init(       (uint8_t)IFC_MGF,     MGF_DEFAULT,   MGF_MIN,    MGF_MAX,    PARAM_THRESH);
	
	lfo_ifc_rate_p.Init(     (uint8_t)IFC_LFO,    LFO_RATE_DEFAULT,  LFO_RATE_MIN,   LFO_RATE_MAX,   PARAM_THRESH);
	lfo_ifc_depth_p.Init(    (uint8_t)IFC_LFO,    LFO_DEPTH_MIN,     LFO_DEPTH_MIN,  LFO_DEPTH_MAX,  PARAM_THRESH);
	lfo_stiff_rate_p.Init(   (uint8_t)STIFF_LFO,  LFO_RATE_DEFAULT,  LFO_RATE_MIN,   LFO_RATE_MAX,   PARAM_THRESH);
	lfo_stiff_depth_p.Init(  (uint8_t)STIFF_LFO,  LFO_DEPTH_MIN,     LFO_DEPTH_MIN,  LFO_DEPTH_MAX,  PARAM_THRESH);
	lfo_beta_rate_p.Init(    (uint8_t)BETA_LFO,   LFO_RATE_DEFAULT,  LFO_RATE_MIN,   LFO_RATE_MAX,   PARAM_THRESH);
	lfo_beta_depth_p.Init(   (uint8_t)BETA_LFO,   LFO_DEPTH_MIN,     LFO_DEPTH_MIN,  LFO_DEPTH_MAX,  PARAM_THRESH);

	new_g = GAIN_DEFAULT;
	new_at = new_dt = ENV_DEFAULT;
	new_ifc = cur_ifc = IFC_DEFAULT; 
	new_stiff = cur_stiff = STIFF_MIN;
	new_beta = cur_beta = BETA_MIN;
	new_mgf = MGF_DEFAULT;
	new_out = 0.0f;
	
	new_lfo_stiff_rate = new_lfo_beta_rate = new_lfo_ifc_rate = LFO_RATE_DEFAULT;
	new_lfo_ifc_depth = new_lfo_stiff_depth = new_lfo_beta_depth = LFO_DEPTH_MIN;

	cur_page = MIDI;
	cur_mode = PING;
	cur_output_mode = NONE;

	UpdateButtons();
	UpdateEncoder();

	for (int i = 0; i < NUM_LFOS; i++) {
	  lfos[i].Init(cr);
	}
	lfos[LFO_IFC].SetRange(IFC_MAX - IFC_MIN);
	lfos[LFO_STIFF].SetRange(STIFF_MAX - STIFF_MIN);
	lfos[LFO_BETA].SetRange(BETA_MAX - BETA_MIN);

	hw.StartAdc();
	hw.StartAudio(AudioCallback);
	hw.midi.StartReceive();

	while(1) {
	  blink_cnt &= blink_mask;
	  if (blink_cnt == 0) {
	    hw.seed.SetLed(led_state);
	    led_state = !led_state;
	  }
	  blink_cnt++;

	  hw.midi.Listen();
          while(hw.midi.HasEvents())
          {
              HandleMidiMessage(hw.midi.PopEvent());
          }
	  hw.ProcessAnalogControls();
	  hw.ProcessDigitalControls();
	  UpdateEncoder();
	  UpdateButtons();
	  hw.UpdateLeds();
	  hw.DelayMs(1);
	}
}
