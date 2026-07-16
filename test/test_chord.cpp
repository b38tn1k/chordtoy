// Host-side tests for chordtoy's chord math. No framework: assert + a main().
//   Build & run:  cc/test/run_tests.sh   (or: c++ -std=c++11 test/test_chord.cpp -o /tmp/t && /tmp/t)
//
// Two jobs:
//   1. Characterize the STRUM channel so its (v1) sound is locked and can't drift.
//   2. Prove the BLOCK channel now derives from its own pitch — the bug this PR fixes.
#include "../chordtoyv2/chordlogic.h"
#include <cassert>
#include <cstdio>

static int failures = 0;

static void expect(const char *name, const uint8_t *got, int gotN,
                   const uint8_t *want, int wantN) {
    bool ok = (gotN == wantN);
    for (int i = 0; ok && i < wantN; i++) { ok = (got[i] == want[i]); }
    if (!ok) {
        failures++;
        printf("FAIL %s\n  got [", name);
        for (int i = 0; i < gotN; i++) printf("%d%s", got[i], i + 1 < gotN ? " " : "");
        printf("]\n  want[");
        for (int i = 0; i < wantN; i++) printf("%d%s", want[i], i + 1 < wantN ? " " : "");
        printf("]\n");
    } else {
        printf("ok   %s\n", name);
    }
}

int main() {
    uint8_t buf[MAXIMUM_CHORD_SIZE];

    // --- STRUM characterization (C major-mode voicing, no inversion) -------------
    // chords[3] = {1,1,1} over majorBank {4,7,10} + pitch 60 -> upper structure, no root.
    { const uint8_t want[] = {64, 67, 70};
      int n = buildStrumChord(60, 3, 0, /*isMinor*/false, /*key*/0, buf);
      expect("strum C major, inv0", buf, n, want, 3); }

    // Inversion 1 drops the 3rd voice an octave: {0,0,-12}.
    { const uint8_t want[] = {64, 67, 58};
      int n = buildStrumChord(60, 3, 1, false, 0, buf);
      expect("strum C major, inv1 (top voice -12)", buf, n, want, 3); }

    // Minor mode, root chord (interval 0, no override): minorBank {3,7,10}.
    { const uint8_t want[] = {63, 67, 70};
      int n = buildStrumChord(60, 3, 0, /*isMinor*/true, 0, buf);
      expect("strum C minor, inv0", buf, n, want, 3); }

    // Precedence quirk lock-in: minor mode, F (interval 5) still selects majorBank
    // because of v1's `&& || ||`. This is intentional — the test guards the sound.
    { const uint8_t want[] = {69, 72, 75};
      int n = buildStrumChord(65, 3, 0, /*isMinor*/true, 0, buf);
      expect("strum minor+interval5 -> majorBank (v1 quirk)", buf, n, want, 3); }

    // --- BLOCK channel: characterization ----------------------------------------
    // Voices from 60's own chord {64,67,70} transposed -24, then octaves of the
    // fundamental and each voice. The root (60) itself is NOT here — Thru sends it.
    { const uint8_t want[] = {40, 43, 46, 72, 48, 52, 28, 55, 31, 58, 34};
      int n = buildBlockChord(60, 3, /*inv2*/0, false, 0, /*transpose*/-24, buf);
      expect("block C major, root omitted (Thru owns it)", buf, n, want, 11); }

    // Small chord (1 voice) packs with NO gaps/garbage.
    { const uint8_t want[] = {43, 72, 48, 55, 31};
      int n = buildBlockChord(60, /*chordSelection*/1, 0, false, 0, -24, buf);
      expect("block 1-voice packs cleanly (5 notes)", buf, n, want, 5);
      for (int i = 0; i < n; i++) assert(buf[i] <= 127 && "valid MIDI note"); }

    // --- THE FIX: block chord depends only on its own pitch ----------------------
    // Reproduce the old failure mode: strum a DIFFERENT pitch first. Pre-fix, the
    // block voices read the strum buffer, so they'd be 72's chord (-24): {52,55,58}.
    // Post-fix they must be 60's own chord: {40, 43, 46}.
    (void)buildStrumChord(72, 3, 0, false, 0, buf);       // pollute any shared state
    { uint8_t block[MAXIMUM_CHORD_SIZE];
      int n = buildBlockChord(60, 3, 0, false, 0, -24, block);
      const uint8_t buggy[] = {52, 55, 58};               // what reading strum's buffer gave
      bool matchesBuggy = true;
      for (int i = 0; i < 3; i++) matchesBuggy = matchesBuggy && (block[i] == buggy[i]);
      assert(!matchesBuggy && "block voices must NOT track the strum channel");
      const uint8_t want[] = {40, 43, 46, 72, 48, 52, 28, 55, 31, 58, 34};
      expect("block independent of prior strum (regression proof)", block, n, want, 11); }

    if (failures) { printf("\n%d test(s) FAILED\n", failures); return 1; }
    printf("\nall tests passed\n");
    return 0;
}
