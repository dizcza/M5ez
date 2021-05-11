#include <M5Core2.h>
#include <ezSound.h>

//ezSynth s(SINE, 1000);
// Waveform, Frequency, Attack, Decay, Sustain, Release (, Gain)
//ezSynth c5(SINE, NOTE_C5, 50, 300, 0.7, 1000);
//ezSynth e5(SINE, NOTE_E5, 50, 300, 0.7, 1000);
//ezSynth g5(SINE, NOTE_G5, 50, 300, 0.7, 1000);

// Set up the synthesizers . Arguments:
//    waveform (SINE | SQUARE | TRIANGLE | SAWTOOTH | NOISE), freq (Hz),
//    attack (ms), decay (ms), sustain (float), release (ms), gain (float)
ezSynth s(TRIANGLE, 1000);

void setup() {
  M5.begin();
  ezSound.begin();

//  c5.playFor(5000);
//  ezSound.delay(1000);
//  e5.playFor(4000);
//  ezSound.delay(1000);
//  g5.playFor(3000);
//  ezSound.waitForSilence();
  
  s.start();
}

void loop() {
  ezSound.update();
}