#include "daisy_pod.h"
#include "daisysp.h"
#include "modal_note.h"
#include "modal_inharm.h"
#include "crc_noise.h"
#include "led_colours.h"

#define  NUM_HARM_PARTIALS    4
#define  NUM_NOTES     5
#define  PING_AMT      1 //0.25 

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

#define  MIDI_CHANNEL  0 // todo - make this settable somehow. Daisy starts counting MIDI channels from 0
#define  CC_MOD	       1
#define  CC_GAIN       7
#define  CC_STIFF      70
#define  CC_BETA       71
#define  CC_REL        72
#define  CC_ATK        73
#define  CC_MGF	       74

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

int  blink_mask = 511; 
int  blink_cnt = 0;
bool led_state = true;

static Parameter knob1_lin, knob1_log, knob2_lin, knob2_log;
float cur_ifc, cur_g, cur_stiff, cur_beta, cur_mgf, cur_mrf, cur_out, cur_a, cur_d;

int midi_f = 0;
int next_note = 0;
bool play_note = false;

typedef enum {MIDI = 0, GAIN_OUT, STIFF_BETA, IFC_MGF, AD, LAST_PAGE} ui_page;
ui_page cur_page = MIDI;
typedef enum {PING = 0, NOISE_ENV, EXT, EXT_ENV, INHARM, INHARM_NOISE, LAST_MODE} ui_mode;
ui_mode cur_mode = PING;
typedef enum {NONE = 0, EXP_DIST, TANH, ARCTAN, LAST_OUTPUT} ui_output_mode;
ui_output_mode cur_output_mode = NONE;
int cur_preset = 0;


void UpdateEncoder();
void UpdateButtons();
float CatchParam(float old, float cur, float thresh);

void AudioCallback(AudioHandle::InputBuffer in, AudioHandle::OutputBuffer out, size_t size)
{

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
	    inharms[next_note]->update_fc(midi_f);
	  } else {
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
		{
		  cur_g = CatchParam(cur_g, CC_TO_VAL(p.value, 0, 1), CATCH_THRESH);
		  float new_g = POT_TO_VAL(cur_g, GAIN_MIN, GAIN_MAX);
		  for (int i = 0; i < NUM_NOTES; i++) {
		    notes[i]->update_g(new_g);
		  }
		  break;
		}
	      case CC_STIFF: 
		{
		  cur_stiff = CatchParam(cur_stiff, CC_TO_VAL(p.value, 0, 1), CATCH_THRESH);
		  float new_stiff = POT_TO_VAL(cur_stiff, STIFF_MIN, STIFF_MAX);
		  for (int i = 0; i < NUM_NOTES; i++) {
		    notes[i]->update_stiffness(new_stiff);
		  }
		  break;
		}
	      case CC_BETA: 
		{
		  cur_beta = CatchParam(cur_beta, CC_TO_VAL(p.value, 0, 1), CATCH_THRESH);
		  int new_beta = (int)roundf(POT_TO_VAL(cur_beta, BETA_MIN, BETA_MAX));
		  for (int i = 0; i < NUM_NOTES; i++) {
		    notes[i]->update_beta(new_beta);
		  }
		  break;
		}
	      case CC_MGF: 
		{
		  cur_mgf = CatchParam(cur_mgf, CC_TO_VAL(p.value, 0, 1), CATCH_THRESH);
		  float new_mgf = POT_TO_VAL(cur_mgf, MGF_MIN, MGF_MAX);
		  for (int i = 0; i < NUM_NOTES; i++) {
		    notes[i]->update_mgf(new_mgf);
		  }
		  break;
		}
	      case CC_REL:
		{
		  cur_d = CatchParam(cur_d, CC_TO_VAL(p.value, 0, 1), CATCH_THRESH);
		  float new_d = POT_TO_VAL(cur_d, ENV_MIN, ENV_MAX);
		  for (int i = 0; i < NUM_NOTES; i++) {
		    if (!env[i].IsRunning()) {
      	  	      env[i].SetTime(ADSR_SEG_DECAY, new_d);
	  	    }
		  }
		  break;
		}
	      case CC_ATK:
		{
		  cur_a = CatchParam(cur_a, CC_TO_VAL(p.value, 0, 1), CATCH_THRESH);
		  float new_a = POT_TO_VAL(cur_a, ENV_MIN, ENV_MAX);
		  for (int i = 0; i < NUM_NOTES; i++) {
		    if (!env[i].IsRunning()) {
      	  	      env[i].SetTime(ADSR_SEG_ATTACK, new_a);
	  	    }
		  }
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
	cur_g = CatchParam(cur_g, knob1_log.Process(), CATCH_THRESH);
	float new_g = POT_TO_VAL(cur_g, GAIN_MIN, GAIN_MAX);
	cur_out = CatchParam(cur_out, knob2_lin.Process(), CATCH_THRESH);
	cur_output_mode = (ui_output_mode)roundf(cur_out * (LAST_OUTPUT - 1));
	for (int i = 0; i < NUM_NOTES; i++) {
	  if (cur_mode == INHARM || cur_mode == INHARM_NOISE) {
	    inharms[i]->modulate_g(new_g);
	  } else {
	    notes[i]->update_g(new_g);
	  }
	}
	hw.led1.Set(RED);
	break;
      }
    case STIFF_BETA:
      {
	cur_stiff = CatchParam(cur_stiff, knob1_log.Process(), CATCH_THRESH);
	float new_stiff = POT_TO_VAL(cur_stiff, STIFF_MIN, STIFF_MAX);
	cur_beta = CatchParam(cur_beta, knob2_lin.Process(), CATCH_THRESH);
	int new_beta = (int)roundf(POT_TO_VAL(cur_beta, BETA_MIN, BETA_MAX));
	for (int i = 0; i < NUM_NOTES; i++) {
	  notes[i]->update_stiffness(new_stiff);
	  notes[i]->update_beta(new_beta);
	}
	hw.led1.Set(GREEN);
	break;
      }
    case IFC_MGF:
      {
	cur_ifc = CatchParam(cur_ifc, knob1_log.Process(), CATCH_THRESH);
	float new_ifc = POT_TO_VAL(cur_ifc, IFC_MIN, IFC_MAX);
	cur_mgf = CatchParam(cur_mgf, knob2_log.Process(), CATCH_THRESH);
	float new_mgf = POT_TO_VAL(cur_mgf, MGF_MIN, MGF_MAX);
	for (int i = 0; i < NUM_NOTES; i++) {
	  if (cur_mode == INHARM || cur_mode == INHARM_NOISE) {
	    inharms[i]->update_ifc(new_ifc);
	  } else {
	    notes[i]->update_mgf(new_mgf);
	    notes[i]->update_ifc(new_ifc);
	  }
	}
	hw.led1.Set(BLUE);
	break;
      }
    case AD:
      {
	cur_a = CatchParam(cur_a, knob1_log.Process(), CATCH_THRESH);
	float new_a = POT_TO_VAL(cur_a, ENV_MIN, ENV_MAX);
	cur_d = CatchParam(cur_d, knob2_log.Process(), CATCH_THRESH);
	float new_d = POT_TO_VAL(cur_d, ENV_MIN, ENV_MAX);
	for (int i = 0; i < NUM_NOTES; i++) {
	  if (!env[i].IsRunning()) {
	    env[i].SetTime(ADSR_SEG_ATTACK, new_a);
      	    env[i].SetTime(ADSR_SEG_DECAY, new_d);
	  }
	}
	hw.led1.Set(YELLOW);
	break;
      }
    default: break;
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
}


int main(void)
{

	hw.Init();
	float sr = hw.AudioSampleRate();

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

	UpdateButtons();
	UpdateEncoder();
	cur_g = 0.5;
	cur_a = 0.015;
	cur_d = 0.015;
	cur_page = MIDI;
	cur_mode = PING;
	cur_output_mode = NONE;

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
