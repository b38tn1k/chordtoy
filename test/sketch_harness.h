#ifndef SKETCH_HARNESS_H
#define SKETCH_HARNESS_H
// Compiles the real chordtoyv2.ino on a host against the fake Arduino/MIDI in
// test/fakes (put it on the include path with -I fakes). The Arduino IDE
// auto-generates these prototypes; a plain C++ compiler needs them because
// setup() calls the handlers before they're defined further down the sketch.
#include <MIDI.h>
void handleNoteOn(byte, byte, byte);
void handleNoteOff(byte, byte, byte);
void pollInputs();
void toggleBank();
#include "../chordtoyv2/chordtoyv2.ino"
#endif
