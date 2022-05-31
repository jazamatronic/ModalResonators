#include "daisy_pod.h"
#include "daisysp.h"
#include "modal_note.h"

#define  NUM_PARTIALS  4
#define  NUM_NOTES     4
#define  MIDI_CHANNEL  1 // todo - make this settable somehow.
#define  PING_AMT      0.25 

using namespace daisy;
using namespace daisysp;

DaisyPod hw;

modal_note *notes[NUM_NOTES];

int  blink_mask = 511; 
int  blink_cnt = 0;
bool led_state = true;

static Parameter fc, res, mgf; 

int midi_f = 0;
int next_note = 0;
bool play_note = false;

void AudioCallback(AudioHandle::InputBuffer in, AudioHandle::OutputBuffer out, size_t size)
{
	hw.ProcessAllControls();

	for (size_t i = 0; i < size; i++)
	{
	  float to_out = 0;
	  for (int j = 0; j < NUM_NOTES; j++) {
	    float to_in = 0;
	    if (play_note && (j == next_note)) {
	      to_in = PING_AMT;
	      play_note = false;
	      if (++next_note == NUM_NOTES) {
	        next_note = 0;
	      }
	    }
	    to_out += notes[j]->Process(to_in) / NUM_NOTES;
	  }

	  out[0][i] = to_out;
	  out[1][i] = to_out;
	} 
} 
 void HandleMidiMessage(MidiEvent m) { 
   //if (m.channel != MIDI_CHANNEL) { return; } //Broken? 
   switch(m.type) { 
     case NoteOn: {
	  NoteOnEvent this_note = m.AsNoteOn();
	  midi_f = mtof(this_note.note);
	  notes[next_note]->update_fc(midi_f);
	  play_note = true;
          break;
	}
      default: break;
  }
}

int main(void)
{

	hw.Init();
	float sr = hw.AudioSampleRate();
	float new_res, new_mgf;

	for (int i = 0; i < NUM_NOTES; i++) {
	  notes[i] = new modal_note(NUM_PARTIALS);
	  notes[i]->init(sr, 45, 0.9999);
	}

    	res.Init(hw.knob1, 0.99333, 0.99999, res.LINEAR); 
    	mgf.Init(hw.knob2, 0, 2, mgf.LINEAR); 

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
	  new_res = res.Process();
	  new_mgf = mgf.Process();
	  for (int i = 0; i < NUM_NOTES; i++) {
	    notes[i]->update_r(new_res);
	    notes[i]->update_mgf(new_mgf);
	  }
	  hw.DelayMs(1);
	}
}
