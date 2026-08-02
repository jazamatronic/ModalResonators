# ModalResonators  

## Author

<!-- Insert Your Name Here -->
Jared ANDERSON

20220604

## Description


BiQuad Resonators  

Initial build for the Daisy Seed platform  

Clone this under the DaisyExamples/pod directory and run make to build.  

The left input of the line-in is used in pass through mode.  
Stereo output is provided.  

Five note polyphony where each note consists of four "modes" or "partials".  

There are nine pages of menu accessible via the encoder. LED1 shows Magenta, Red, Cyan, Green, Light Green, Orange, Blue, Light Blue and Yellow respectively.

MAGENTA = MIDI page:  
&nbsp;&nbsp;No pots are active, MIDI control on Channel 1.  
&nbsp;&nbsp;MIDI CC messages control the following parameters:  
&nbsp;&nbsp;CC 1 (Mod Wheel) = resonance  
&nbsp;&nbsp;CC 7 (Volume) = gain  
&nbsp;&nbsp;CC 14 = Input Filter Cutoff (IFC)  
&nbsp;&nbsp;CC 70 = stiffness  
&nbsp;&nbsp;CC 71 = beta (harmonics control - decimates the harmonic series; beta = 1 plays every harmonic including the fundamental)  
&nbsp;&nbsp;CC 72 = attack time for envelope modes
&nbsp;&nbsp;CC 73 = decay time for envelope modes
&nbsp;&nbsp;CC 74 = MGF (mode gain factor)  
&nbsp;&nbsp;CC 75 = Mode  
&nbsp;&nbsp;CC 76 = Inharmonic Preset  
&nbsp;&nbsp;CC 77 = Strike Position  
&nbsp;&nbsp;CC 78 = Strike Width (harmonic voice only - no effect in INHARM/INHARM_NOISE mode)  
&nbsp;&nbsp;CC 85 = IFC LFO Rate  
&nbsp;&nbsp;CC 86 = IFC LFO Depth  
&nbsp;&nbsp;CC 87 = Stiffness LFO Rate  
&nbsp;&nbsp;CC 88 = Stiffness LFO Depth  
&nbsp;&nbsp;CC 89 = BETA LFO Rate  
&nbsp;&nbsp;CC 90 = BETA LFO Depth  
  
RED = Gain / Overdrive page:  
&nbsp;&nbsp;POT1 = Gain  
&nbsp;&nbsp;POT2 = Overdrive model - hard clipping, exponential distortion, tanh distortion, arctan distortion    
  
CYAN = Strike Position / Width page:  
&nbsp;&nbsp;POT1 = Strike Width - mallet/hammer contact size, 0.0 (point strike) to 0.3. Progressively dulls higher harmonics as it increases. Only affects the harmonic voice (PING/NOISE_ENV/EXT/EXT_ENV) - has no effect in INHARM/INHARM_NOISE mode  
&nbsp;&nbsp;POT2 = Strike Position - where along the modelled string/bar the strike lands, 0.0 to 1.0. Nulls harmonics at their nodes; avoid 0.5 with beta = 2 (or other position/beta combinations that null the same harmonics) or the note goes silent  
  
GREEN = Stiffness / Beta page:  
&nbsp;&nbsp;POT1 = Stiffness (spreads higher modes)  
&nbsp;&nbsp;POT2 = Beta (controls harmonic content)  
  
LIGHT GREEN = Stiffness LFO:  
&nbsp;&nbsp;POT1 = LFO Rate  
&nbsp;&nbsp;POT2 = LFO Depth  
  
ORANGE = BETA LFO:  
&nbsp;&nbsp;POT1 = LFO Rate  
&nbsp;&nbsp;POT2 = LFO Depth  
  
BLUE = IFC / MGF page:  
&nbsp;&nbsp;POT1 = Input Filter Cutoff = 10 to 22000Hz  
&nbsp;&nbsp;POT2 = MGF (Brightness - boosts or cuts higher modes)

LIGHT BLUE = IFC LFO:  
&nbsp;&nbsp;POT1 = LFO Rate  
&nbsp;&nbsp;POT2 = LFO Depth  

YELLOW = ENVELOPE page:  
&nbsp;&nbsp;POT1 = Attack time - 1 to 100 ms  
&nbsp;&nbsp;POT2 = DECAY time - 1 to 100 ms  
  
Button 2 toggles between harmonic ping, harmonic noise env, external input, enveloped external input, inharmonic ping or inharmonic noise mode  
  
LED2 = OFF  
&nbsp;&nbsp;Ping mode, each harmonic note is excited by a single ping  
LED2 = MAGENTA   
&nbsp;&nbsp;Harmonic noise env, each note is excited by enveloped noise  
LED2 = CYAN  
&nbsp;&nbsp;Left input channel is fed into each note  
&nbsp;&nbsp;*CAUTION* This can blow up under high resonances - keep the gain down and bring it up slowly  
LED2 = BLUE  
&nbsp;&nbsp;Left input channel is enveloped and fed into each note  
&nbsp;&nbsp;*CAUTION* This can blow up under high resonances - keep the gain down and bring it up slowly    
LED2 = YELLOW  
&nbsp;&nbsp;Inharmonic mode - 10 presets available, cycle through with Button 1. Excited by a ping.  
&nbsp;&nbsp;Resonance can be modulated with the MOD WHEEL  
&nbsp;&nbsp;Input filter cutoff can be adjusted by turning POT1 on the BLUE page  
LED2 = WHITE  
&nbsp;&nbsp;Same as Inharmonic mode but excited by enveloped noise   
  
  
An early Demo / feature walkthrough available click here:  
[![ModalResonators Demo](https://img.youtube.com/vi/S-_UZKW8978/0.jpg)](https://www.youtube.com/watch?v=S-_UZKW8978 "ModalResonators Demo")


