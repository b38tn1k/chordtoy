#include <MIDI.h>
#include "largeConstants.h"
#define MODE_SWITCH 13
#define CHORD_POT 1
#define INVERSION_POT 2
#define KEY_POT 0
#define RATE_POT 3
#define LED_1 4
#define LED_2 5
#define ANALOG_READ_RESOLUTION 1053.0
#define CHORD_VARIATIONS 15 // DX polyphony limit :-/
#define VOICES 7 // how many notes (not including root)
#define POLY 4
#define INVERSION_COUNT 6//hmm
#define THERE_ARE_12_NOTES_IN_WESTERN_MUSIC 12
#define MIDI_ROOT_NOTE_OFFSET 9 //an octave below A0
#define MIDI_RANGE_WITH_SCALE 81 // approx how bit our scale map array should be (72 notes in 9 octaves + the onw below)
MIDI_CREATE_DEFAULT_INSTANCE();

int chordSelection = 0;
int inversionSelection = 0;
byte thisChord[] = {0, 0, 0}; // DX polyphony limit
byte prevChord[] = {0, 0, 0};
unsigned long strumRate = 0;
bool trig = false;
byte chan, vel;
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
// T S T T S T T
const byte minorScaleShape[] = {2, 1, 2, 2, 1, 2, 2};
const byte majorScaleShape[] = {2, 2, 1, 2, 2, 2, 1};
byte scaleShape[] = {2, 1, 2, 2, 1, 2, 2};
byte scale[MIDI_RANGE_WITH_SCALE];
const int inversions[INVERSION_COUNT][POLY-1] = {{0, 0, 0}, {0, 0, -12}, {-12, 0, 0}, {-12, -12, 0}, {0, -12, -12}, {-12, -12, -12}};

//Reface DX Polyphony
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

const byte minorBank[] = {3, 7, 10, 14, 17, 21};
byte bank[VOICES];
const byte majorBank[] = {4, 7, 10, 12, 14, 17};
bool modeState;
bool prevMode;

void setup() {
  pinMode(LED_1, OUTPUT);
  pinMode(LED_2, OUTPUT);
  pinMode(MODE_SWITCH, INPUT);
  MIDI.setHandleNoteOn(handleNoteOn);
  MIDI.setHandleNoteOff(handleNoteOff);
  MIDI.begin(1);
  pollInputs();
  toggleBank();
  prevMode = modeState;
  prevKey = key;
}

void loop() {  
  currentMillis= millis();
  MIDI.read();
  if (currentMillis - pollPrevMillis >= pollInterval) {
    pollInputs();
    pollPrevMillis = currentMillis;
  }

  if ((currentMillis - trigPrevMillis >= strumRate) && (cur < curLim)) {
    MIDI.sendNoteOn(thisChord[cur], vel, chan);
    trigPrevMillis = currentMillis;
    cur++;
  }
  if (prevMode != modeState) {
    toggleBank();
    prevMode = modeState;
  }
  // this is just to flash the stupid LED so you know what key!
  if (key != prevKey) {
    buildScale();
    keyCounter = (key + 1) * 2;
    prevKey = key;
    keyLEDStatus = false;
    digitalWrite(activeLED, keyLEDStatus);
    keyLEDPrevMillis = currentMillis + 1000;
    
  }
  if ((currentMillis-keyLEDPrevMillis>=200)&&(keyCounter >= 0)) {
    keyCounter -=1;
    keyLEDStatus = !keyLEDStatus;
    digitalWrite(activeLED, keyLEDStatus);
    keyLEDPrevMillis = currentMillis;
  }
}

int noteInScale(byte _note) {
  int note = int(_note);
  int pos = -1;
  for (int i = 0; i < MIDI_RANGE_WITH_SCALE; i++) {
    if (note == scale[i]) {
      pos = i;
      break;
    }
  }
  return pos;
}

void buildScale() {
  int root = key + MIDI_ROOT_NOTE_OFFSET;
  scale[0] = root;
  int j = 0;
  for (int i = 1; i < MIDI_RANGE_WITH_SCALE; i++) {
    scale[i] = scale[i-1] + scaleShape[j];
    j += 1;
    j = j % 7;
  }
}

void changeShape(byte _bank[]) {
    for (int i = 0; i < 7; i++) {
      scaleShape[i] = _bank[i];
    }
}

void toggleBank() {
  if (modeState == true) {
    assignBank(minorBank);
    changeShape(minorScaleShape);
    buildScale();
    activeLED = LED_1;
    inactiveLED = LED_2;
  } else {
    inactiveLED = LED_1;
    activeLED = LED_2;
    changeShape(majorScaleShape);
    buildScale();
    assignBank(majorBank);
  }
  digitalWrite(activeLED, HIGH);
  digitalWrite(inactiveLED, LOW);
}

void pollInputs() {
  modeState = digitalRead(MODE_SWITCH);
  strumRate = long(analogRead(RATE_POT));
  strumRate = long(analogRead(RATE_POT)); 
  strumRate = strumRate/3;
  float chordSensorVal = analogRead(CHORD_POT);
  chordSensorVal = analogRead(CHORD_POT);
  chordSelection =  int(chordSensorVal * (CHORD_VARIATIONS/ANALOG_READ_RESOLUTION));
  float invSensorVal = analogRead(INVERSION_POT);
  invSensorVal = analogRead(INVERSION_POT);
  inversionSelection = int(invSensorVal * (INVERSION_COUNT/ANALOG_READ_RESOLUTION));
  //testing key
  inversionSelection = 0;
  float keySensorVal = analogRead(KEY_POT);
  keySensorVal = analogRead(KEY_POT);
  key = int(keySensorVal * (THERE_ARE_12_NOTES_IN_WESTERN_MUSIC/ANALOG_READ_RESOLUTION));
}

void handleNoteOn(byte channel, byte pitch, byte velocity) {
  trigPrevMillis = millis();
  clearNCopyChord(pitch);
  cur = 0;
  curLim = 0;
  chan = channel;
  vel = velocity;
  trig = true;
  // create chord to be played in here
  // first in standard shape, pretend it not in the scale
  // note not in scale
  int j = 0;
  for (int i = 0; i < VOICES; i ++ ) {
    if (chords[chordSelection][i] == 1) {
      curLim++;
      thisChord[j] = bank[i] + pitch;
      j++;
    }
  }

  ///reshape the chord if it is in the scale
  int pos = noteInScale(pitch);
  if (pos != -1) {
    //note in scale
    pos += 2;
    for (int j = 0; j < VOICES; j++) {
      thisChord[j] = scale[pos];
      j+=1;
      pos += 2; //triads
    } 
  }
  // then apply inversion
  for (int k = 0; k < curLim; k++) {
    thisChord[k] = byte(int(thisChord[k]) + inversions[inversionSelection][k]);
  }
}

void handleNoteOff(byte channel, byte pitch, byte velocity) {
  cur = curLim;
  MIDI.sendNoteOff(pitch, 0, channel);
  for (int i = 0; i < VOICES; i ++ ) {
    MIDI.sendNoteOff(thisChord[i], 0, channel);
    MIDI.sendNoteOff(prevChord[i], 0, channel);
  }
}

void assignBank(byte _bank[]) {
    for (int i = 0; i < VOICES; i++) {
      bank[i] = _bank[i];
    }
}

void clearNCopyChord(byte pitch) {
  for (int i = 0; i < POLY; i++) {
    prevChord[i] = thisChord[i];
    thisChord[i] = pitch;
  }
}

//void chordSelectTest() {
//  int dur = 300;
//  if (digitalRead(TOGGLE_BUTTON) == HIGH) {
//    for (int i = 0; i < chordSelection; i++) {
//      digitalWrite(LED_1, HIGH);
//      digitalWrite(LED_2, HIGH);
//      digitalWrite(LED_3, HIGH);
//      delay(dur);
//      digitalWrite(LED_1, LOW);
//      digitalWrite(LED_2, LOW);
//      digitalWrite(LED_3, LOW);
//      delay(dur);
//    }
//    delay(5000);
//  }
//  
//}
//
//void potTest(int POT) {
//  int dur = analogRead(POT);
//  digitalWrite(LED_1, HIGH);
//  digitalWrite(LED_2, HIGH);
//  digitalWrite(LED_3, HIGH);
//  dur = analogRead(POT);
//  delay(dur);
//  dur = analogRead(POT);
//  digitalWrite(LED_1, LOW);
//  digitalWrite(LED_2, LOW);
//  digitalWrite(LED_3, LOW);
//  dur = analogRead(POT);
//  delay(dur);
//  dur = analogRead(POT);
//}

