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
byte thisChord[VOICES];               // strum channel notes (zero-init)
byte secondChord[MAXIMUM_CHORD_SIZE]; // block channel notes
byte prevChord[VOICES];               // previous strum, so NoteOff can clear a strum in progress
unsigned long strumRate = 0;
byte vel;
unsigned long currentMillis;
unsigned long pollPrevMillis = 0;
const long pollInterval = 100;
unsigned long trigPrevMillis;
int cur = 0;
int curLim = 0;
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

void setup() {
  pinMode(LED_1, OUTPUT);
  pinMode(LED_2, OUTPUT);
  pinMode(BYPASS_SWITCH, INPUT);
  pinMode(MAJOR_MINOR, INPUT);
  pinMode(LEGATO_SWITCH, INPUT);
  MIDI.setHandleNoteOn(handleNoteOn);
  MIDI.setHandleNoteOff(handleNoteOff);
//  MIDI.begin(MIDI_STRUM_CHANNEL); //bypass only works with this one channel. TODO: research MIDI Library more
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
    if ((currentMillis - trigPrevMillis >= strumRate) && (cur < curLim)) {
      MIDI.sendNoteOn(thisChord[cur], vel, MIDI_STRUM_CHANNEL);
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
    for (int k = 0; k < MAXIMUM_CHORD_SIZE; k++) {
      MIDI.sendNoteOff(secondChord[k], 0, MIDI_BLOCK_CHORD_CHANNEL); //clear out channel, faster than doing all notes every time which has a noticable lag
      secondChord[k] = 0;
    }
  }
  if (legatoDebounce != legato) {
    handleNoteOff(MIDI_STRUM_CHANNEL, 0, 0); //hmm, just try clear it out
    legatoDebounce = legato;
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
  // Pass through anything we don't handle (and everything when bypassed).
  if ((channel != MIDI_BLOCK_CHORD_CHANNEL && channel != MIDI_STRUM_CHANNEL) || bypass == true) {
    MIDI.sendNoteOn(pitch, velocity, channel);
    return;
  }

  if (channel == MIDI_STRUM_CHANNEL) {
    trigPrevMillis = millis();
    clearNCopyChord(pitch);
    cur = 0;
    vel = velocity;
    // loop() strums thisChord[0..curLim) out over time at strumRate.
    curLim = buildStrumChord(pitch, chordSelection, inversionSelection, isMinor, key, thisChord);
  }

  if (channel == MIDI_BLOCK_CHORD_CHANNEL) {
    // Clear the chord that's currently sounding (cheaper than an all-notes-off).
    for (int k = 0; k < MAXIMUM_CHORD_SIZE; k++) {
      MIDI.sendNoteOff(secondChord[k], 0, MIDI_BLOCK_CHORD_CHANNEL);
    }
    // Build this channel's OWN chord from the incoming pitch (the fix — it used to
    // read the strum channel's leftover buffer), then send up to the Size pot.
    int notes = buildBlockChord(pitch, chordSelection, inversionSelection2, isMinor, key,
                                TRNSPS_2ND_CHAN, secondChord);
    if (chordSize < notes) { notes = chordSize; }
    for (int k = 0; k < notes; k++) {
      MIDI.sendNoteOn(secondChord[k], velocity, MIDI_BLOCK_CHORD_CHANNEL);
      Serial.flush();
    }
  }
}

void handleNoteOff(byte channel, byte pitch, byte velocity) {

  if (bypass == true) {
    MIDI.sendNoteOff(pitch, velocity, channel);
  } else {
    if (legato == false) {
      cur = curLim;
    }
    if (channel == MIDI_STRUM_CHANNEL) {
      MIDI.sendNoteOff(pitch, 0, MIDI_STRUM_CHANNEL);
      for (int i = 0; i < VOICES; i ++ ) {
        MIDI.sendNoteOff(thisChord[i], 0, MIDI_STRUM_CHANNEL);
        MIDI.sendNoteOff(prevChord[i], 0, MIDI_STRUM_CHANNEL);
      }
    } else if (channel == MIDI_BLOCK_CHORD_CHANNEL) {
      MIDI.sendNoteOff(pitch, 0, MIDI_BLOCK_CHORD_CHANNEL);
      for (int i = 0; i < MAXIMUM_CHORD_SIZE; i ++ ) {
        MIDI.sendNoteOff(secondChord[i], 0, MIDI_BLOCK_CHORD_CHANNEL);
      }
    } else {
      MIDI.sendNoteOff(pitch, velocity, channel);
    }
  }
}


void clearNCopyChord(byte pitch) {
  for (int i = 0; i < POLY; i++) {
    prevChord[i] = thisChord[i];
    thisChord[i] = pitch;
  }
}
