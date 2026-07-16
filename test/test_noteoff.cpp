// Stuck-note tests: drive the real chordtoyv2.ino note handlers through overlap,
// control-change, legato and bypass sequences, then replay the emitted MIDI into
// a tiny synth model and assert nothing is left sounding (no stuck notes) and no
// held note gets cut by a superseded one.
#include "sketch_harness.h"
#include <cassert>
#include <cstdio>

// Replays every emitted event into on[channel][note]; last event per note wins,
// exactly how a synth tracks it. What's left `true` at the end is stuck.
struct Synth {
  bool on[16][128];
  Synth() {
    for (int c = 0; c < 16; c++) for (int n = 0; n < 128; n++) on[c][n] = false;
    for (size_t i = 0; i < midiLog.size(); i++) {
      NoteEvent &e = midiLog[i];
      if (e.channel >= 0 && e.channel < 16 && e.note >= 0 && e.note < 128) on[e.channel][e.note] = e.on;
    }
  }
  int sounding() { int c = 0; for (int ch = 0; ch < 16; ch++) for (int n = 0; n < 128; n++) if (on[ch][n]) c++; return c; }
  int soundingOn(int ch) { int c = 0; for (int n = 0; n < 128; n++) if (on[ch][n]) c++; return c; }
  bool isOn(int ch, int n) { return on[ch][n]; }
};

static int failures = 0;
static void check(bool ok, const char *name) {
  if (ok) { printf("ok   %s\n", name); }
  else    { printf("FAIL %s\n", name); failures++; }
}

static void resetToy() {
  midiLog.clear();
  strumCount = 0; strumActive = false; strumRoot = 0; cur = 0;
  blockCount = 0; blockActive = false; blockRoot = 0;
  bypass = false; bypassDebounce = false;
  legato = false; legatoDebounce = false;
  strumRate = 0;
  chordSelection = 3; chordSize = MAXIMUM_CHORD_SIZE;
  inversionSelection = 0; inversionSelection2 = 0;
  isMinor = false; key = 0; prevKey = 0; prevMode = false;
}

// The strum sends one note per loop() over time; run it enough to flush the chord.
static void flushStrum() { for (int i = 0; i < 8; i++) loop(); }

int main() {
  // 1) Block: on then off, nothing stuck.
  resetToy();
  handleNoteOn(MIDI_BLOCK_CHORD_CHANNEL, 60, 100);
  handleNoteOff(MIDI_BLOCK_CHORD_CHANNEL, 60, 0);
  check(Synth().sounding() == 0, "block on/off leaves nothing sounding");

  // 2) Strum: on, flush, off, nothing stuck.
  resetToy();
  handleNoteOn(MIDI_STRUM_CHANNEL, 60, 100);
  flushStrum();
  check(Synth().soundingOn(MIDI_STRUM_CHANNEL) > 0, "strum actually sounds after flush");
  handleNoteOff(MIDI_STRUM_CHANNEL, 60, 0);
  check(Synth().sounding() == 0, "strum on/off leaves nothing sounding");

  // 3) THE CLASSIC: three overlapping strum notes, then release all. The old
  //    1-deep prevChord left the earliest chord stuck; owned tracking doesn't.
  resetToy();
  handleNoteOn(MIDI_STRUM_CHANNEL, 60, 100); flushStrum();
  handleNoteOn(MIDI_STRUM_CHANNEL, 64, 100); flushStrum();
  handleNoteOn(MIDI_STRUM_CHANNEL, 67, 100); flushStrum();
  handleNoteOff(MIDI_STRUM_CHANNEL, 60, 0);   // superseded -> ignored
  handleNoteOff(MIDI_STRUM_CHANNEL, 64, 0);   // superseded -> ignored
  handleNoteOff(MIDI_STRUM_CHANNEL, 67, 0);   // owner -> turns its chord off
  check(Synth().sounding() == 0, "3 overlapping strum notes: none stuck");

  // 4) Releasing a superseded note must NOT cut the chord that replaced it.
  resetToy();
  handleNoteOn(MIDI_BLOCK_CHORD_CHANNEL, 60, 100);
  handleNoteOn(MIDI_BLOCK_CHORD_CHANNEL, 64, 100);   // replaces 60's chord
  handleNoteOff(MIDI_BLOCK_CHORD_CHANNEL, 60, 0);    // 60 already superseded
  {
    Synth s;
    check(s.soundingOn(MIDI_BLOCK_CHORD_CHANNEL) == blockCount && s.isOn(MIDI_BLOCK_CHORD_CHANNEL, 64),
          "releasing superseded note leaves the replacing chord sounding");
  }
  handleNoteOff(MIDI_BLOCK_CHORD_CHANNEL, 64, 0);    // owner
  check(Synth().sounding() == 0, "then releasing the owner clears it");

  // 5) Controls changed while a block note is held: must still turn off what was
  //    SENT, not what the pots would compute now.
  resetToy();
  handleNoteOn(MIDI_BLOCK_CHORD_CHANNEL, 60, 100);
  chordSelection = 8; key = 5; chordSize = 4;         // twist the knobs mid-note
  handleNoteOff(MIDI_BLOCK_CHORD_CHANNEL, 60, 0);
  check(Synth().sounding() == 0, "block: control change mid-note doesn't strand notes");

  // 6) Same for the strum channel.
  resetToy();
  handleNoteOn(MIDI_STRUM_CHANNEL, 60, 100); flushStrum();
  chordSelection = 8; key = 5;
  handleNoteOff(MIDI_STRUM_CHANNEL, 60, 0);
  check(Synth().sounding() == 0, "strum: control change mid-note doesn't strand notes");

  // 7) Legato = ring out: release leaves it sounding; the next trigger turns it off.
  resetToy();
  legato = true;
  handleNoteOn(MIDI_STRUM_CHANNEL, 60, 100); flushStrum();
  handleNoteOff(MIDI_STRUM_CHANNEL, 60, 0);
  check(Synth().soundingOn(MIDI_STRUM_CHANNEL) == strumCount, "legato: chord rings out after release");
  handleNoteOn(MIDI_STRUM_CHANNEL, 64, 100); flushStrum();
  check(Synth().soundingOn(MIDI_STRUM_CHANNEL) == strumCount,
        "legato: next trigger replaces the ringing chord (old one turned off)");

  // 8) Engaging bypass silences anything sounding (no drone left behind).
  resetToy();
  handleNoteOn(MIDI_BLOCK_CHORD_CHANNEL, 60, 100);
  handleNoteOn(MIDI_STRUM_CHANNEL, 62, 100); flushStrum();
  bypass = true;          // pollInputs would set this; loop() sees the change and clears
  loop();
  check(Synth().sounding() == 0, "engaging bypass clears sounding notes");

  if (failures) { printf("\n%d note-off test(s) FAILED\n", failures); return 1; }
  printf("\nnote-off tests passed\n");
  return 0;
}
