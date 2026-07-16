#ifndef CHORDLOGIC_H
#define CHORDLOGIC_H

// Pure chord math for chordtoy, extracted so it can be unit-tested on a host
// (see ../test/test_chord.cpp) and shared by both MIDI channels instead of
// being copy-pasted. No Arduino dependencies live in here on purpose.
#include <stdint.h>

#define VOICES 7                 // width of the chords[] rows (only 0..2 ever set)
#define CHORD_VARIATIONS 15      // DX polyphony limit :-/
#define POLY 4
#define INVERSION_COUNT 6
#define MAXIMUM_CHORD_SIZE 12    // block channel: root + voices + octave copies

// const at file scope has internal linkage in C++, so including this header in
// the .ino and in the test is fine (no multiple-definition).
const int chords[CHORD_VARIATIONS][VOICES] = {
    {0, 0, 0, 0, 0, 0},
    {0, 1, 0, 0, 0, 0},
    {1, 1, 0, 0, 0, 0},
    {1, 1, 1, 0, 0, 0},
    {1, 1, 0, 1, 0, 0},
    {1, 1, 0, 0, 1, 0},
    {0, 1, 1, 1, 0, 0},
    {0, 1, 1, 0, 1, 0},
    {1, 0, 1, 0, 1, 0},
    {0, 0, 1, 1, 1, 0},
    {0, 1, 1, 0, 1, 0},
    {0, 0, 0, 1, 1, 1},
    {0, 1, 1, 0, 0, 1},
    {1, 1, 0, 0, 0, 1},
    {0, 1, 0, 1, 0, 1}
};
const int inversions[INVERSION_COUNT][POLY - 1] = {
    {0, 0, 0}, {0, 0, -12}, {-12, 0, 0}, {-12, -12, 0}, {0, -12, -12}, {-12, -12, -12}
};
const uint8_t minorBank[] = {3, 7, 10, 14, 17, 21};
const uint8_t dimBank[]   = {3, 6, 12, 15, 18, 24};
const uint8_t majorBank[] = {4, 7, 10, 12, 14, 17};

// Build the diatonic chord voices for `pitch` (before any inversion is applied).
// Returns the voice count (0..3). `out` must have room for VOICES bytes.
//
// NOTE: the `&&`/`||` precedence here is deliberately preserved from v1 — it is
// NOT textbook diatonic (interval==5/10 override regardless of mode), but it is
// the sound the toy has always made. Do not "fix" it without meaning to.
static inline int buildChordVoices(uint8_t pitch, int chordSelection, bool isMinor,
                                   int key, uint8_t *out) {
    const uint8_t *sel = isMinor ? minorBank : majorBank;

    int lowNote = int(pitch) % 12;
    if ((lowNote - key) < 0) { lowNote += 12; }
    int interval = lowNote - key;

    // Sequential last-true-wins overrides, exactly as v1 rebuilt the chord. The
    // parens make v1's `&&`-binds-tighter grouping explicit (interval==5/10 fire
    // regardless of mode) — this reproduces the quirk on purpose, not by accident.
    if ((isMinor == true && interval == 3) || interval == 5 || interval == 10) { sel = majorBank; }
    if (isMinor == true && interval == 2)                                      { sel = dimBank; }
    if ((isMinor == false && interval == 2) || interval == 4 || interval == 9) { sel = minorBank; }
    if (isMinor == false && interval == 11)                                    { sel = dimBank; }

    int count = 0;
    for (int i = 0; i < VOICES; i++) {
        if (chords[chordSelection][i] == 1) {
            out[count] = sel[i] + pitch;
            count++;
        }
    }
    return count;
}

// Strum channel: voices with this channel's own inversion applied in place.
// Returns voice count. `out` must have room for VOICES bytes.
static inline int buildStrumChord(uint8_t pitch, int chordSelection, int inversionSelection,
                                  bool isMinor, int key, uint8_t *out) {
    int count = buildChordVoices(pitch, chordSelection, isMinor, key, out);
    for (int k = 0; k < count; k++) {
        out[k] = uint8_t(int(out[k]) + inversions[inversionSelection][k]);
    }
    return count;
}

// Block channel: the notes to ADD around the incoming note — this channel's chord
// voices (transposed, with 2x inversion) plus octave copies of the fundamental and
// of each voice, above and below. Returns how many were written (<= MAXIMUM_CHORD_SIZE);
// `out` must have room for MAXIMUM_CHORD_SIZE bytes.
//
// The fundamental itself (the incoming pitch) is deliberately NOT emitted: MIDI Thru
// passes the incoming note straight through, so sending it here too would double it.
// Voices come from THIS pitch via buildChordVoices (the block-buffer fix); depends
// only on its arguments.
static inline int buildBlockChord(uint8_t pitch, int chordSelection, int inversionSelection2,
                                  bool isMinor, int key, int transpose, uint8_t *out) {
    uint8_t voices[VOICES];
    int count = buildChordVoices(pitch, chordSelection, isMinor, key, voices);

    uint8_t tv[VOICES];                              // transposed, inverted voices
    for (int k = 0; k < count; k++) {
        tv[k] = uint8_t(int(voices[k]) + transpose + 2 * inversions[inversionSelection2][k]);
    }

    int w = 0;
    for (int k = 0; k < count && w < MAXIMUM_CHORD_SIZE; k++) out[w++] = tv[k];
    // octaves of the fundamental (the root note itself is Thru's job), then each voice
    if (w < MAXIMUM_CHORD_SIZE) out[w++] = uint8_t(int(pitch) + 12);
    if (w < MAXIMUM_CHORD_SIZE) out[w++] = uint8_t(int(pitch) - 12);
    for (int k = 0; k < count; k++) {
        if (w < MAXIMUM_CHORD_SIZE) out[w++] = uint8_t(int(tv[k]) + 12);
        if (w < MAXIMUM_CHORD_SIZE) out[w++] = uint8_t(int(tv[k]) - 12);
    }
    return w;
}

#endif
