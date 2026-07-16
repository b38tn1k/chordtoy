// End-to-end proof through the REAL sketch: compiles chordtoyv2.ino against a
// fake Arduino/MIDI (test/fakes) and drives the note handlers, checking the MIDI
// actually emitted. This is where the bug lived — the block channel reading the
// strum channel's buffer — so it's the test that must catch a regression of it.
//
//   Build & run:  test/run_tests.sh  (compiles with -I test/fakes)
#include "sketch_harness.h"                // compiles the real chordtoyv2.ino (fake MIDI via -I fakes)
#include <cassert>
#include <cstdio>

static std::vector<int> blockNotesOn() {
  std::vector<int> notes;
  for (size_t i = 0; i < midiLog.size(); i++)
    if (midiLog[i].on && midiLog[i].channel == MIDI_BLOCK_CHORD_CHANNEL)
      notes.push_back(midiLog[i].note);
  return notes;
}

int main() {
  // Deterministic settings (pots/switches are read as 0 by the fake, so set the
  // globals directly): C-ish, major mode, key A=0, triad, full size, no ch2 inversion.
  chordSelection = 3;
  chordSize = MAXIMUM_CHORD_SIZE;
  inversionSelection = 0;
  inversionSelection2 = 0;
  isMinor = false;
  key = 0;
  bypass = false;

  // 1) Strum a DIFFERENT pitch (72). Pre-fix this populated the shared buffer the
  //    block channel then wrongly reused.
  handleNoteOn(MIDI_STRUM_CHANNEL, 72, 100);

  // 2) Play the block channel on its own pitch (60).
  midiLog.clear();
  handleNoteOn(MIDI_BLOCK_CHORD_CHANNEL, 60, 100);

  std::vector<int> got = blockNotesOn();
  // No root here — Thru sends the fundamental (60). The handler emits the added
  // voices + octaves, built from 60's OWN chord.
  const int want[] = {40, 43, 46, 72, 48, 52, 28, 55, 31, 58, 34};

  bool ok = (got.size() == 11);
  for (size_t i = 0; ok && i < 11; i++) ok = (got[i] == want[i]);

  if (!ok) {
    printf("FAIL block chord wiring\n  got [");
    for (size_t i = 0; i < got.size(); i++) printf("%d ", got[i]);
    printf("]\n  want[");
    for (int i = 0; i < 11; i++) printf("%d ", want[i]);
    printf("]\n");
    return 1;
  }

  // The smoking gun: voices come from 60's own chord (40,43,46), NOT from the
  // strummed 72's chord (which would give 52,55,58).
  assert(got[0] == 40 && got[1] == 43 && got[2] == 46 && "voices from THIS pitch");
  assert(!(got[0] == 52 && got[1] == 55 && got[2] == 58) && "must not leak the strum chord");

  printf("ok   block chord built from its own pitch, independent of strum (real .ino)\n");
  printf("\nintegration test passed\n");
  return 0;
}
