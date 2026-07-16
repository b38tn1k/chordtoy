#!/bin/sh
# Build and run the chordtoy tests on the host (no Arduino toolchain needed).
#   - test_chord.cpp:       unit tests for the shared chord math (chordlogic.h)
#   - test_equivalence.cpp: proves the strum channel is unchanged vs v1 (differential)
#   - test_integration.cpp: block chord is built from its own pitch (real .ino)
#   - test_noteoff.cpp:     no stuck notes under overlap/control-change/legato/bypass
set -e
cd "$(dirname "$0")"

for t in test_chord test_equivalence; do
  c++ -std=c++11 -Wall -o "/tmp/chordtoy_$t" "$t.cpp"
  "/tmp/chordtoy_$t"
  echo
done

for t in test_integration test_noteoff; do
  c++ -std=c++11 -Wall -I fakes -o "/tmp/chordtoy_$t" "$t.cpp"
  "/tmp/chordtoy_$t"
  echo
done
