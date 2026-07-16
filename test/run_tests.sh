#!/bin/sh
# Build and run the chordtoy tests on the host (no Arduino toolchain needed).
#   - test_chord.cpp:       unit tests for the shared chord math (chordlogic.h)
#   - test_equivalence.cpp: proves the strum channel is unchanged vs v1 (differential)
#   - test_integration.cpp: compiles the real chordtoyv2.ino against a fake
#                           Arduino/MIDI and checks the notes it emits
set -e
cd "$(dirname "$0")"

c++ -std=c++11 -Wall -o /tmp/chordtoy_unit test_chord.cpp
/tmp/chordtoy_unit

echo
c++ -std=c++11 -Wall -o /tmp/chordtoy_equivalence test_equivalence.cpp
/tmp/chordtoy_equivalence

echo
c++ -std=c++11 -Wall -I fakes -o /tmp/chordtoy_integration test_integration.cpp
/tmp/chordtoy_integration
