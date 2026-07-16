#include <MIDI.h>
#include "chordlogic.h"
//So this one does two MIDI channels, just connect two MIDI plugs to the output of the arduino, it should provide enough power
//or look for a blog on how to do it properly :-D

// MIDI SETTINGS - Change these as desired
//the strum channel
#define MIDI_STRUM_CHANNEL 3
//the pad or arp channel
#define MIDI_BLOCK_CHORD_CHANNEL 5

//IO for pots and switches
#define MAJOR_MINOR 11        //switch for major / minor
#define CHORD_POT 4           //pot to scroll throguh various chord shapes
#define INV_POT1 0            //pot to scroll through inversions for first channel
#define KEY_POT 5             //pot to scroll through western scale, enables selection of the root note for the major / minor mode, A = 0, Bb = 1, B = 2, ...
#define RATE_POT 1            //strum rate
#define LED_1 13              //major or minor signifier? IDK
#define LED_2 12              //major or minor signifier? IDK
#define BYPASS_SWITCH 10      //just pass normal MIDI through
#define LEGATO_SWITCH 9       //change note off mode a bit, also its not really legato, is something else
#define INV_POT2 2            //pot to scroll through inversions for second channel
#define SIZE_POT 3            //pot to increase size of chord for second channel through addition of octaves above and below
#define TRNSPS_2ND_CHAN -24   //just my taste



//I dont think you need to worry about any other settings unless you wanna make some improvements
#define ANALOG_READ_RESOLUTION 1053.0
#define THERE_ARE_12_NOTES_IN_WESTERN_MUSIC 12
#define MIDI_RANGE_WITH_SCALE 71
// VOICES, POLY, MAXIMUM_CHORD_SIZE, the chord/inversion/bank tables and the
// buildStrumChord()/buildBlockChord() math now live in chordlogic.h, which is
// host-tested (see ../test/). Both channels share it instead of copy-pasting.
MIDI_CREATE_DEFAULT_INSTANCE();

int chordSelection = 0;
int inversionSelection = 0;
int inversionSelection2 = 0;

// Each channel owns at most one active chord at a time, tracked by the exact
// notes we sent and the input note (root) that started it. NoteOff turns off
// precisely those notes, so nothing sticks and a superseded note can't cut the
// chord that replaced it. (This replaced a shared buffer + 1-deep prevChord that
// leaked stuck notes once notes overlapped.)
byte strumNotes[VOICES];               // strum chord: planned, strummed out over time
int  strumCount = 0;
byte strumRoot = 0;
bool strumActive = false;
byte blockNotes[MAXIMUM_CHORD_SIZE];   // block chord: sent all at once
int  blockCount = 0;
byte blockRoot = 0;
bool blockActive = false;

unsigned long strumRate = 0;
byte vel;
unsigned long currentMillis;
unsigned long pollPrevMillis = 0;
const long pollInterval = 100;
unsigned long trigPrevMillis;
int cur = 0;
int activeLED;
int inactiveLED;
int prevKey = 0;
int key = 0; //A = 0, Bb = 1, B = 2, ...
int keyCounter = 0;
long keyLEDPrevMillis = 0;
bool keyLEDStatus = false;
bool bypass = false;
bool bypassDebounce = false;
bool legato = false;
bool legatoDebounce = false;
int chordSize = 0;
bool isMinor;
bool prevMode;

// Turn off exactly the notes we sent (nothing more), so nothing can stick on.
void chordOff(byte notes[], int count, byte channel) {
  for (int i = 0; i < count; i++) {
    MIDI.sendNoteOff(notes[i], 0, channel);
  }
}

void setup() {
  pinMode(LED_1, OUTPUT);
  pinMode(LED_2, OUTPUT);
  pinMode(BYPASS_SWITCH, INPUT);
  pinMode(MAJOR_MINOR, INPUT);
  pinMode(LEGATO_SWITCH, INPUT);
  MIDI.setHandleNoteOn(handleNoteOn);
  MIDI.setHandleNoteOff(handleNoteOff);
//  MIDI.begin(MIDI_STRUM_CHANNEL); //bypass only works with this one channel. TODO: research MIDI Library more
  // OMNI so we see (and can transform) notes on every channel. MIDI Thru is left ON
  // (library default, Thru::Full): it copies every incoming message straight to the
  // output. That is what provides ALL pass-through (bypass and unhandled channels)
  // and the fundamental root note under each chord. The handlers only ADD generated
  // voices on top, so nothing gets doubled. Do NOT turnThruOff() without also
  // re-adding the roots and pass-through by hand. (Closes issue #2.)
  MIDI.begin(MIDI_CHANNEL_OMNI);
  pollInputs();
  toggleBank();
  prevMode = isMinor;
  prevKey = key;
}

void loop() {
  currentMillis= millis();
  MIDI.read();
  if (currentMillis - pollPrevMillis >= pollInterval) {
    pollInputs();
    pollPrevMillis = currentMillis;
  }
  if (bypass == false) {
    if ((currentMillis - trigPrevMillis >= strumRate) && (cur < strumCount)) {
      MIDI.sendNoteOn(strumNotes[cur], vel, MIDI_STRUM_CHANNEL);
      trigPrevMillis = currentMillis;
      cur++;
    }
    if (prevMode != isMinor) {
      toggleBank();
      prevMode = isMinor;
    }
    // this is just to flash the stupid LED so you know what key!
    if (key != prevKey) {
      keyCounter = (key + 1) * 2;
      prevKey = key;
      keyLEDStatus = false;
      digitalWrite(activeLED, keyLEDStatus);
      keyLEDPrevMillis = currentMillis + 200;

    }
    if ((currentMillis-keyLEDPrevMillis>=200)&&(keyCounter >= 0)) {
      keyCounter -=1;
      keyLEDStatus = !keyLEDStatus;
      digitalWrite(activeLED, keyLEDStatus);
      keyLEDPrevMillis = currentMillis;
    }
  } else {
    if (currentMillis-keyLEDPrevMillis>=200) {
      keyLEDStatus = !keyLEDStatus;
      digitalWrite(LED_1, keyLEDStatus);
      digitalWrite(LED_2, keyLEDStatus);
      keyLEDPrevMillis = currentMillis;
    }
  }
  if (bypassDebounce != bypass) {
    bypassDebounce = bypass;
    digitalWrite(activeLED, HIGH);
    digitalWrite(inactiveLED, LOW);
    // toggling bypass: silence whatever we had sounding so it can't stick on
    chordOff(strumNotes, strumCount, MIDI_STRUM_CHANNEL);
    chordOff(blockNotes, blockCount, MIDI_BLOCK_CHORD_CHANNEL);
    strumCount = 0; strumActive = false;
    blockCount = 0; blockActive = false;
    cur = 0;
  }
  if (legatoDebounce != legato) {
    legatoDebounce = legato;
    // flipping the legato switch resets any ringing strum
    chordOff(strumNotes, strumCount, MIDI_STRUM_CHANNEL);
    strumCount = 0; strumActive = false;
    cur = 0;
  }
}


void toggleBank() {
  // LED just signals major vs minor; the chord math derives its bank from isMinor.
  if (isMinor == true) {
    activeLED = LED_1;
    inactiveLED = LED_2;
  } else {
    inactiveLED = LED_1;
    activeLED = LED_2;
  }
  digitalWrite(activeLED, HIGH);
  digitalWrite(inactiveLED, LOW);
}

void pollInputs() {
  isMinor = digitalRead(MAJOR_MINOR);
  bypass = digitalRead(BYPASS_SWITCH);
  legato = digitalRead(LEGATO_SWITCH);

  analogRead(RATE_POT);
  strumRate = long(ANALOG_READ_RESOLUTION - analogRead(RATE_POT));
  strumRate = strumRate/3;

  analogRead(CHORD_POT);
  float chordSensorVal = analogRead(CHORD_POT);
  chordSelection =  int(chordSensorVal * (CHORD_VARIATIONS/ANALOG_READ_RESOLUTION));

  analogRead(INV_POT1);
  float invSensorVal =analogRead(INV_POT1);
  inversionSelection = int(invSensorVal * (INVERSION_COUNT/ANALOG_READ_RESOLUTION));

  analogRead(INV_POT2);
  invSensorVal =analogRead(INV_POT2);
  inversionSelection2 = int(invSensorVal * (INVERSION_COUNT/ANALOG_READ_RESOLUTION));

  analogRead(KEY_POT);
  float keySensorVal = analogRead(KEY_POT);
  key = int(keySensorVal * (THERE_ARE_12_NOTES_IN_WESTERN_MUSIC/ANALOG_READ_RESOLUTION));

  analogRead(SIZE_POT);
  float chordSizeVal = analogRead(SIZE_POT);
  chordSize =int(chordSizeVal * (MAXIMUM_CHORD_SIZE/ANALOG_READ_RESOLUTION));
}

void handleNoteOn(byte channel, byte pitch, byte velocity) {
  // In bypass we generate nothing; MIDI Thru passes the raw note straight through.
  if (bypass == true) return;

  if (channel == MIDI_STRUM_CHANNEL) {
    // a new trigger replaces the sounding strum chord: turn the old voices off first
    chordOff(strumNotes, strumCount, MIDI_STRUM_CHANNEL);
    strumCount = buildStrumChord(pitch, chordSelection, inversionSelection, isMinor, key, strumNotes);
    strumRoot = pitch;
    strumActive = true;
    cur = 0;                 // loop() strums strumNotes[0..strumCount) out over time
    vel = velocity;
    trigPrevMillis = millis();
  } else if (channel == MIDI_BLOCK_CHORD_CHANNEL) {
    // replace the sounding block chord
    chordOff(blockNotes, blockCount, MIDI_BLOCK_CHORD_CHANNEL);
    int added = buildBlockChord(pitch, chordSelection, inversionSelection2, isMinor, key,
                                TRNSPS_2ND_CHAN, blockNotes);
    // Size pot is total voices INCLUDING the Thru-provided root, so we add size-1.
    blockCount = chordSize - 1;
    if (blockCount > added) { blockCount = added; }
    if (blockCount < 0)     { blockCount = 0; }
    blockRoot = pitch;
    blockActive = true;
    for (int k = 0; k < blockCount; k++) {
      MIDI.sendNoteOn(blockNotes[k], velocity, MIDI_BLOCK_CHORD_CHANNEL);
      Serial.flush();
    }
  }
  // other channels: MIDI Thru handles pass-through; nothing to do
}

void handleNoteOff(byte channel, byte pitch, byte velocity) {
  // In bypass, Thru passes the raw NoteOff through; nothing to do here.
  if (bypass == true) return;

  if (channel == MIDI_STRUM_CHANNEL) {
    // Only the note that started the current strum can end it. A release for a
    // note that was already superseded is ignored (its chord is already off).
    if (strumActive && pitch == strumRoot) {
      if (legato == false) {
        cur = strumCount;                                   // stop strumming the rest
        chordOff(strumNotes, strumCount, MIDI_STRUM_CHANNEL);
        strumCount = 0;
        strumActive = false;
      }
      // legato == true: let the chord ring out; the next trigger turns it off
    }
  } else if (channel == MIDI_BLOCK_CHORD_CHANNEL) {
    if (blockActive && pitch == blockRoot) {
      chordOff(blockNotes, blockCount, MIDI_BLOCK_CHORD_CHANNEL);
      blockCount = 0;
      blockActive = false;
    }
  }
  // other channels: MIDI Thru handles pass-through; nothing to do
}
