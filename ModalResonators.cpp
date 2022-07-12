#include "daisy_pod.h"
#include "daisysp.h"
#include "modal_note.h"
#include "modal_inharm.h"
#include "crc_noise.h"
#include "led_colours.h"
#include "tri_lfo.h"

// TODO: Make parameter handling cleaner

#define NUM_HARM_PARTIALS   4
#define NUM_NOTES	    5

#define NUM_LFOS      3
#define LFO_RATE_MAX  60
#define LFO_DEPTH_MAX 1
#define LFO_IFC	      0
#define LFO_STIFF     1
#define LFO_BETA      2

#define PING_AMT      	    1 //0.25 

#define CATCH_THRESH	  0.05f

#define RES_MIN	  0.99333
#define RES_MAX   0.99999
#define IFC_MIN   10
#define IFC_MAX   22000
#define GAIN_MIN  0
#define GAIN_MAX  10 
#define STIFF_MIN 0
#define STIFF_MAX 0.005 
#define BETA_MIN  2
#define BETA_MAX  5
#define MGF_MIN	  -1
#define MGF_MAX   3
#define ENV_MIN	  0.001
#define ENV_MAX	  0.1


#define CLAMP(x, min, max)  (x > max) ? max : ((x < min) ? min : x)
#define SGN(x)		    (signbit(x) ? -1.0 : 1.0)

#define CC_TO_VAL(x, min, max) (min + (x / 127.0f) * (max - min))
#define POT_TO_VAL(x, min, max) (min + x * (max - min))
#define VAL_TO_POT(x, min, max) ((x - min) / (max - min))

#define  MIDI_CHANNEL	0 // todo - make this settable somehow. Daisy starts counting MIDI channels from 0
#define  CC_MOD	       	1
#define  CC_GAIN       	7
#define  CC_STIFF      	70
#define  CC_BETA       	71
#define  CC_REL        	72
#define  CC_ATK        	73
#define  CC_MGF	       	74
#define  CC_MODE	75
#define  CC_INHARM	76
#define  CC_LFO_IFC_R  	85
#define  CC_LFO_IFC_D  	86
#define  CC_LFO_STIFF_R	87
#define  CC_LFO_STIFF_D	88
#define  CC_LFO_BETA_R	89
#define  CC_LFO_BETA_D	90

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
float cur_ifc_pot, cur_g_pot, cur_stiff_pot, cur_beta_pot, cur_mgf_pot, cur_mrf_pot, cur_out_pot, cur_a_pot, cur_d_pot;
float cur_ifc, cur_g, cur_stiff, cur_beta, cur_mgf, cur_mrf, cur_out, cur_a, cur_d;
float new_ifc, new_g, new_stiff, new_beta, new_mgf, new_mrf, new_out, new_a, new_d;
float cur_lfo_ifc_rate_pot, cur_lfo_ifc_depth_pot, cur_lfo_stiff_rate_pot, cur_lfo_stiff_depth_pot, cur_lfo_beta_rate_pot, cur_lfo_beta_depth_pot;
float cur_lfo_ifc_rate, cur_lfo_ifc_depth, cur_lfo_stiff_rate, cur_lfo_stiff_depth, cur_lfo_beta_rate, cur_lfo_beta_depth;
float new_lfo_ifc_rate, new_lfo_ifc_depth, new_lfo_stiff_rate, new_lfo_stiff_depth, new_lfo_beta_rate, new_lfo_beta_depth;

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
float CatchParam(float old, float cur, float thresh);

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
	  if (cur_mode == INHARM || cur_mode == INHARM_NOISE) {
	    midi_v = CC_TO_VAL(this_note.velocity, 0, 1);
	    inharms[next_note]->update_fc(midi_f);
	    inharms[next_note]->modulate_g(midi_v);
	  } else {
	    midi_v = CC_TO_VAL(this_note.velocity, cur_g, GAIN_MAX);
	    notes[next_note]->update_fc(midi_f);
	    notes[next_note]->update_g(midi_v);
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
		{
		  if (cur_mode == INHARM || cur_mode == INHARM_NOISE) {
		    if (cur_g > 1) { cur_g = 1; }
		    new_g = CatchParam(cur_g, CC_TO_VAL(p.value, 0, 1), CATCH_THRESH);
		  } else {
		    new_g = POT_TO_VAL(CatchParam(cur_g, CC_TO_VAL(p.value, 0, 1), CATCH_THRESH), GAIN_MIN, GAIN_MAX);
		  }
		  break;
		}
	      case CC_STIFF: 
		{
		  new_stiff = POT_TO_VAL(CatchParam(cur_stiff, CC_TO_VAL(p.value, 0, 1), CATCH_THRESH), STIFF_MIN, STIFF_MAX);
		  break;
		}
	      case CC_BETA: 
		{
		  new_beta = (int)roundf(POT_TO_VAL(CatchParam(cur_beta, CC_TO_VAL(p.value, 0, 1), CATCH_THRESH), BETA_MIN, BETA_MAX));
		  break;
		}
	      case CC_MGF: 
		{
		  new_mgf = POT_TO_VAL(CatchParam(cur_mgf, CC_TO_VAL(p.value, 0, 1), CATCH_THRESH), MGF_MIN, MGF_MAX);
		  break;
		}
	      case CC_REL:
		{
		  new_d = POT_TO_VAL(CatchParam(cur_d, CC_TO_VAL(p.value, 0, 1), CATCH_THRESH), ENV_MIN, ENV_MAX);
		  break;
		}
	      case CC_ATK:
		{
		  new_a = POT_TO_VAL(CatchParam(cur_a, CC_TO_VAL(p.value, 0, 1), CATCH_THRESH), ENV_MIN, ENV_MAX);
		  break;
		}
	      case CC_MODE:
		{
		  cur_mode = (ui_mode)floor(CC_TO_VAL(p.value, 0, (LAST_MODE - 0.1))); // - 0.1 to avoid hitting LAST_MODE
		  SetLedMode();
		  break;
		}
	      case CC_INHARM:
		{
		  cur_preset = floor(CC_TO_VAL(p.value, 0, NUM_INHARM_PRESETS));
		  for (int i = 0; i < NUM_NOTES; i++) {
    		    inharms[i]->load_preset(&inharm_presets[cur_preset]);
    		  }
		  break;
		}
	      case CC_LFO_IFC_R:
		{
		  new_lfo_ifc_rate = POT_TO_VAL(CatchParam(cur_lfo_ifc_rate, CC_TO_VAL(p.value, 0, 1), CATCH_THRESH), 0, LFO_RATE_MAX);
		  break;
		}
	      case CC_LFO_IFC_D:
		{
		  new_lfo_ifc_depth = CatchParam(cur_lfo_ifc_depth, CC_TO_VAL(p.value, 0, 1), CATCH_THRESH);
		  break;
		}
	      case CC_LFO_STIFF_R:
		{
		  new_lfo_stiff_rate = POT_TO_VAL(CatchParam(cur_lfo_stiff_rate, CC_TO_VAL(p.value, 0, 1), CATCH_THRESH), 0, LFO_RATE_MAX);
		  break;
		}
	      case CC_LFO_STIFF_D:
		{
		  new_lfo_stiff_depth = CatchParam(cur_lfo_stiff_depth, CC_TO_VAL(p.value, 0, 1), CATCH_THRESH);
		  break;
		}
	      case CC_LFO_BETA_R:
		{
		  new_lfo_beta_rate = POT_TO_VAL(CatchParam(cur_lfo_beta_rate, CC_TO_VAL(p.value, 0, 1), CATCH_THRESH), 0, LFO_RATE_MAX);
		  break;
		}
	      case CC_LFO_BETA_D:
		{
		  new_lfo_beta_depth = CatchParam(cur_lfo_beta_depth, CC_TO_VAL(p.value, 0, 1), CATCH_THRESH);
		  break;
		}
              default: break;
	    }
	    break;
	}
      default: break;
  }
}

float CatchParam(float old, float cur, float thresh) 
{
  return (abs(old - cur) < thresh) ? cur : old;
}

void UpdateEncoder()
{

  cur_page = (ui_page)(cur_page + hw.encoder.Increment());
  if (cur_page >= LAST_PAGE) { cur_page = MIDI; }
  switch(cur_page)
  {
    case MIDI:
      // disable controls
      hw.led1.Set(PURPLE);
      break;
    case GAIN_OUT:
      {
	cur_g_pot = CatchParam(cur_g_pot, knob1_log.Process(), CATCH_THRESH);
	new_g = POT_TO_VAL(cur_g_pot, GAIN_MIN, GAIN_MAX);
	cur_out_pot = CatchParam(cur_out_pot, knob2_lin.Process(), CATCH_THRESH);
	new_out = roundf(cur_out_pot * (LAST_OUTPUT - 1));
	hw.led1.Set(RED);
	break;
      }
    case STIFF_BETA:
      {
	cur_stiff_pot = CatchParam(cur_stiff_pot, knob1_log.Process(), CATCH_THRESH);
	new_stiff = POT_TO_VAL(cur_stiff_pot, STIFF_MIN, STIFF_MAX);
	cur_beta_pot = CatchParam(cur_beta_pot, knob2_lin.Process(), CATCH_THRESH);
	new_beta = (int)roundf(POT_TO_VAL(cur_beta_pot, BETA_MIN, BETA_MAX));
	hw.led1.Set(GREEN);
	break;
      }
    case STIFF_LFO:
      {
	cur_lfo_stiff_rate_pot = CatchParam(cur_lfo_stiff_rate_pot, knob1_log.Process(), CATCH_THRESH);
	new_lfo_stiff_rate = POT_TO_VAL(cur_lfo_stiff_rate_pot, 0, LFO_RATE_MAX);
	cur_lfo_stiff_depth_pot = CatchParam(cur_lfo_stiff_depth_pot, knob2_lin.Process(), CATCH_THRESH);
	new_lfo_stiff_depth = POT_TO_VAL(cur_lfo_stiff_depth_pot, 0, LFO_DEPTH_MAX);
	hw.led1.Set(LGREEN);
	break;
      }
    case BETA_LFO:
      {
	cur_lfo_beta_rate_pot = CatchParam(cur_lfo_beta_rate_pot, knob1_log.Process(), CATCH_THRESH);
	new_lfo_beta_rate = POT_TO_VAL(cur_lfo_beta_rate_pot, 0, LFO_RATE_MAX);
	cur_lfo_beta_depth_pot = CatchParam(cur_lfo_beta_depth_pot, knob2_lin.Process(), CATCH_THRESH);
	new_lfo_beta_depth = POT_TO_VAL(cur_lfo_beta_depth_pot, 0, LFO_DEPTH_MAX);
	hw.led1.Set(ORANGE);
	break;
      }
    case IFC_MGF:
      {
	cur_ifc_pot = CatchParam(cur_ifc_pot, knob1_log.Process(), CATCH_THRESH);
	new_ifc = POT_TO_VAL(cur_ifc_pot, IFC_MIN, IFC_MAX);
	cur_mgf_pot = CatchParam(cur_mgf_pot, knob2_log.Process(), CATCH_THRESH);
	new_mgf = POT_TO_VAL(cur_mgf_pot, MGF_MIN, MGF_MAX);
	hw.led1.Set(BLUE);
	break;
      }
    case IFC_LFO:
      {
	cur_lfo_ifc_rate_pot = CatchParam(cur_lfo_ifc_rate_pot, knob1_log.Process(), CATCH_THRESH);
	new_lfo_ifc_rate = POT_TO_VAL(cur_lfo_ifc_rate_pot, 0, LFO_RATE_MAX);
	cur_lfo_ifc_depth_pot = CatchParam(cur_lfo_ifc_depth_pot, knob2_lin.Process(), CATCH_THRESH);
	new_lfo_ifc_depth = POT_TO_VAL(cur_lfo_ifc_depth_pot, 0, LFO_DEPTH_MAX);
	hw.led1.Set(LBLUE);
	break;
      }
    case AD:
      {
	cur_a_pot = CatchParam(cur_a_pot, knob1_log.Process(), CATCH_THRESH);
	new_a = POT_TO_VAL(cur_a_pot, ENV_MIN, ENV_MAX);;
	cur_d_pot = CatchParam(cur_d_pot, knob2_log.Process(), CATCH_THRESH);
	new_d = POT_TO_VAL(cur_d_pot, ENV_MIN, ENV_MAX);;
	hw.led1.Set(YELLOW);
	break;
      }
    default: break;
  } 
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
  if (new_lfo_ifc_rate != cur_lfo_ifc_rate) {
    lfos[LFO_IFC].SetFreq(new_lfo_ifc_rate);
    cur_lfo_ifc_rate = new_lfo_ifc_rate;
  }
  if (new_lfo_ifc_depth != cur_lfo_ifc_depth) {
    lfos[LFO_IFC].SetDepth(new_lfo_ifc_depth);
    cur_lfo_ifc_depth = new_lfo_ifc_depth;
  }
  float lfo_new_ifc = CLAMP(new_ifc + lfos[LFO_IFC].GetOutput(), IFC_MIN, IFC_MAX);

  if (new_lfo_stiff_rate != cur_lfo_stiff_rate) {
    lfos[LFO_STIFF].SetFreq(new_lfo_stiff_rate);
    cur_lfo_stiff_rate = new_lfo_stiff_rate;
  }
  if (new_lfo_stiff_depth != cur_lfo_stiff_depth) {
    lfos[LFO_STIFF].SetDepth(new_lfo_stiff_depth);
    cur_lfo_stiff_depth = new_lfo_stiff_depth;
  }
  float lfo_new_stiff = CLAMP(new_stiff + lfos[LFO_STIFF].GetOutput(), STIFF_MIN, STIFF_MAX);

  if (new_lfo_beta_rate != cur_lfo_beta_rate) {
    lfos[LFO_BETA].SetFreq(new_lfo_beta_rate);
    cur_lfo_beta_rate = new_lfo_beta_rate;
  }
  if (new_lfo_beta_depth != cur_lfo_beta_depth) {
    lfos[LFO_BETA].SetDepth(new_lfo_beta_depth);
    cur_lfo_beta_depth = new_lfo_beta_depth;
  }
  float lfo_new_beta = CLAMP(new_beta + lfos[LFO_BETA].GetOutput(), BETA_MIN, BETA_MAX);

  if (new_out != cur_out) {
    cur_output_mode = (ui_output_mode)new_out;
    cur_out = new_out;
  }

  for (int i = 0; i < NUM_NOTES; i++) {

    if (new_a != cur_a) {
      if (!env[i].IsRunning()) {
        env[i].SetTime(ADSR_SEG_ATTACK, new_a);
      }
    }

    if (new_d != cur_d) {
      if (!env[i].IsRunning()) {
        env[i].SetTime(ADSR_SEG_DECAY, new_d);
      }
    }

    if (cur_mode == INHARM || cur_mode == INHARM_NOISE) {
      if (new_g != cur_g) { 
        inharms[i]->modulate_g(new_g);
      } 
      if (lfo_new_ifc != cur_ifc) { 
	inharms[i]->update_ifc(lfo_new_ifc);
      } 
    } else {
      if (new_g != cur_g) { 
        notes[i]->update_g(new_g);
      }
      if (lfo_new_stiff != cur_stiff) { 
        notes[i]->update_stiffness(lfo_new_stiff);
      }
      if (lfo_new_beta != cur_beta) { 
	notes[i]->update_beta(lfo_new_beta);
      }
      if (new_mgf != cur_mgf) { 
	notes[i]->update_mgf(new_mgf);
      }
      if (lfo_new_ifc != cur_ifc) { 
	notes[i]->update_ifc(lfo_new_ifc);
      } 
    }
  }
  cur_a = new_a;
  cur_beta = new_beta;
  cur_d = new_d;
  cur_g = new_g;
  cur_ifc = new_ifc;
  cur_mgf = new_mgf;
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
      	  env[i].SetTime(ADSR_SEG_ATTACK, 0.015);
      	  env[i].SetTime(ADSR_SEG_DECAY, 0.015);
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

	new_g = cur_g = 0.5;
	cur_g_pot = VAL_TO_POT(new_g, GAIN_MIN, GAIN_MAX);
	new_a = cur_a = 0.015;
	cur_a_pot = VAL_TO_POT(new_a, ENV_MIN, ENV_MAX);
	new_d = cur_d = 0.015;
	cur_d_pot = VAL_TO_POT(new_d, ENV_MIN, ENV_MAX);
	new_ifc = cur_ifc = 220; 
	cur_ifc_pot = VAL_TO_POT(new_ifc, IFC_MIN, IFC_MAX);
	new_stiff = cur_stiff = STIFF_MIN;
	cur_stiff_pot = VAL_TO_POT(new_stiff, STIFF_MIN, STIFF_MAX);
	new_beta = cur_beta = BETA_MIN;
	cur_beta_pot = 0;
	cur_mgf = new_mgf = 0;
	cur_mgf_pot = VAL_TO_POT(new_mgf, MGF_MIN, MGF_MAX);
	cur_out = new_out = cur_out_pot = 0;
	
	cur_lfo_ifc_rate = new_lfo_ifc_rate = 0.3;
	cur_lfo_ifc_rate_pot = VAL_TO_POT(new_lfo_ifc_rate, 0, LFO_RATE_MAX); 
	cur_lfo_ifc_depth = new_lfo_ifc_depth = cur_lfo_ifc_depth_pot = 0;
	cur_lfo_stiff_rate = new_lfo_stiff_rate = 0.3;
	cur_lfo_stiff_rate_pot = VAL_TO_POT(new_lfo_stiff_rate, 0, LFO_RATE_MAX); 
	cur_lfo_stiff_depth = new_lfo_stiff_depth = cur_lfo_stiff_depth_pot = 0;
	cur_lfo_beta_rate = new_lfo_beta_rate = 0.3;
	cur_lfo_beta_rate_pot = VAL_TO_POT(new_lfo_beta_rate, 0, LFO_RATE_MAX); 
	cur_lfo_beta_depth = new_lfo_beta_depth = cur_lfo_beta_depth_pot = 0;

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
//	  UpdateParams();
	  hw.UpdateLeds();
	  hw.DelayMs(1);
	}
}
