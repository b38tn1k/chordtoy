// Proves the refactor did NOT change the strum channel's sound: buildStrumChord()
// must match v1's original handleNoteOn chord math for every input combination.
// `originalStrum` below is a verbatim copy of the strum body from the deleted
// chordtoy/chordtoy.ino (git show cdbeabf^:chordtoy/chordtoy.ino), minus the
// trig/cur/vel bookkeeping that doesn't affect which notes come out. The only
// substitution is bank = isMinor?minorBank:majorBank, which is exactly what the
// original's toggleBank()/assignBank() set that global to.
#include "../chordtoyv2/chordlogic.h"
#include <cassert>
#include <cstdio>

static int originalStrum(uint8_t pitch, int chordSelection, int inversionSelection,
                         bool isMinor, int key, uint8_t *thisChord) {
  const uint8_t *bank = isMinor ? minorBank : majorBank;
  int curLim = 0;
  int j = 0;
  for (int i = 0; i < VOICES; i ++ ) {
    if (chords[chordSelection][i] == 1) { curLim++; thisChord[j] = bank[i] + pitch; j++; }
  }
  int lowNote = int(pitch) % 12;
  if ((lowNote - key) < 0) {lowNote += 12;}
  int interval = lowNote - key;
  if (isMinor == true && interval == 3 || interval == 5 || interval == 10) {
    curLim = 0; j = 0;
    for (int i = 0; i < VOICES; i ++ ) {
      if (chords[chordSelection][i] == 1) { curLim++; thisChord[j] = majorBank[i] + pitch; j++; }
    }
  }
  if (isMinor == true && interval == 2) {
    curLim = 0; j = 0;
    for (int i = 0; i < VOICES; i ++ ) {
      if (chords[chordSelection][i] == 1) { curLim++; thisChord[j] = dimBank[i] + pitch; j++; }
    }
  }
  if (isMinor == false && interval == 2 || interval == 4 || interval == 9) {
    curLim = 0; j = 0;
    for (int i = 0; i < VOICES; i ++ ) {
      if (chords[chordSelection][i] == 1) { curLim++; thisChord[j] = minorBank[i] + pitch; j++; }
    }
  }
  if (isMinor == false && interval == 11) {
    curLim = 0; j = 0;
    for (int i = 0; i < VOICES; i ++ ) {
      if (chords[chordSelection][i] == 1) { curLim++; thisChord[j] = dimBank[i] + pitch; j++; }
    }
  }
  for (int k = 0; k < curLim; k++) {
    thisChord[k] = uint8_t(int(thisChord[k]) + inversions[inversionSelection][k]);
  }
  return curLim;
}

int main() {
  long checked = 0;
  for (int cs = 0; cs < CHORD_VARIATIONS; cs++)
    for (int key = 0; key < 12; key++)
      for (int m = 0; m < 2; m++)
        for (int inv = 0; inv < INVERSION_COUNT; inv++)
          for (int pitch = 24; pitch <= 96; pitch++) {
            uint8_t a[VOICES] = {0}, b[VOICES] = {0};
            int ca = originalStrum((uint8_t)pitch, cs, inv, (bool)m, key, a);
            int cb = buildStrumChord((uint8_t)pitch, cs, inv, (bool)m, key, b);
            assert(ca == cb && "strum voice count changed vs v1");
            for (int k = 0; k < ca; k++) assert(a[k] == b[k] && "strum note changed vs v1");
            checked++;
          }
  printf("ok   strum matches v1 exactly across %ld input combinations\n", checked);
  printf("\nequivalence test passed\n");
  return 0;
}
