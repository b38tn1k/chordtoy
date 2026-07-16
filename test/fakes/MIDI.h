#ifndef FAKE_MIDI_H
#define FAKE_MIDI_H
// Minimal Arduino + FortySevenEffects-MIDI test double: just enough surface to
// compile chordtoyv2.ino on a host and record the MIDI it emits. Not hardware.
#include <cstdint>
#include <vector>

typedef uint8_t byte;
#define INPUT 0
#define OUTPUT 1
#define LOW 0
#define HIGH 1

inline unsigned long millis() { return 0; }
inline int  digitalRead(int) { return 0; }   // LOW -> major mode, not bypassed, not legato
inline void digitalWrite(int, int) {}
inline void pinMode(int, int) {}
inline int  analogRead(int) { return 0; }

struct FakeSerial { void flush() {} };
static FakeSerial Serial;

struct NoteEvent { int note; int velocity; int channel; bool on; };
static std::vector<NoteEvent> midiLog;

struct FakeMidi {
  void setHandleNoteOn(void (*)(byte, byte, byte)) {}
  void setHandleNoteOff(void (*)(byte, byte, byte)) {}
  void begin(int) {}
  void read() {}
  void sendNoteOn(byte n, byte v, byte c)  { midiLog.push_back({n, v, c, true}); }
  void sendNoteOff(byte n, byte v, byte c) { midiLog.push_back({n, v, c, false}); }
};

#define MIDI_CHANNEL_OMNI 0
#define MIDI_CREATE_DEFAULT_INSTANCE() FakeMidi MIDI;

#endif
