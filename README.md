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

There are four pages of menu accessible via the encoder. LED1 shows Magenta, Red, Green and Blue respectively.

MAGENTA = MIDI page:  
&nbsp;&nbsp;No pots are active, MIDI control on Channel 1.  
&nbsp;&nbsp;MIDI CC messages control the following parameters:  
&nbsp;&nbsp;CC 1 (Mod Wheel) = resonance  
&nbsp;&nbsp;CC 7 (Volume) = gain  
&nbsp;&nbsp;CC 70 = stiffness  
&nbsp;&nbsp;CC 71 = beta (harmonics control)  
&nbsp;&nbsp;CC 74 = MGF (mode gain factor)  
  
RED = Input Filter / Gain page:  
&nbsp;&nbsp;POT1 = Input Filter (1 pole) Cutoff - simulates strike/pluck hardness  
&nbsp;&nbsp;POT2 = Gain  
  
GREEN = Stiffness / Beta page:  
&nbsp;&nbsp;POT1 = Stiffness (spreads higher modes)  
&nbsp;&nbsp;POT2 = Beta (controls harmonic content)  
  
BLUE = MGF page:  
&nbsp;&nbsp;POT1 = MGF (Brightness - boosts or cuts higher modes)
  
Button 2 toggles between ping, external input or inharmonic mode  
  
LED2 = OFF  
&nbsp;&nbsp;Ping mode, each note is excited by a single ping  
LED2 = CYAN  
&nbsp;&nbsp;Left input channel is fed into each note
&nbsp;&nbsp;*CAUTION* This can blow up under high resonances - keep the gain down and bring it up slowly  
LED2 = YELLOW  
&nbsp;&nbsp;Inharmonic mode - currently only one single preset. Resonance can be modulated with the MOD WHEEL  
&nbsp;&nbsp;Input filter cutoff can be adjusted by turning POT1 on the RED page  

