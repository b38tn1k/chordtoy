#include <MIDI.h>
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
#define MIDI_RANGE_WITH_SCALE 71
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

byte bank[VOICES];
const byte minorBank[] = {3, 7, 10, 14, 17, 21};
const byte dimBank[] = {3, 6, 12, 15, 18, 24};
const byte majorBank[] = {4, 7, 10, 12, 14, 17};
bool isMinor;
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

  if ((currentMillis - trigPrevMillis >= strumRate) && (cur < curLim)) {
    MIDI.sendNoteOn(thisChord[cur], vel, chan);
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
}


void toggleBank() {
  if (isMinor == true) {
    assignBank(minorBank);
    activeLED = LED_1;
    inactiveLED = LED_2;
  } else {
    inactiveLED = LED_1;
    activeLED = LED_2;
    assignBank(majorBank);
  }
  digitalWrite(activeLED, HIGH);
  digitalWrite(inactiveLED, LOW);
}

void pollInputs() {
  isMinor = digitalRead(MODE_SWITCH);
  
  analogRead(RATE_POT);
  strumRate = long(analogRead(RATE_POT)); 
  strumRate = strumRate/3;
  
  analogRead(CHORD_POT);
  float chordSensorVal = analogRead(CHORD_POT);
  chordSelection =  int(chordSensorVal * (CHORD_VARIATIONS/ANALOG_READ_RESOLUTION));
  
  analogRead(INVERSION_POT);
  float invSensorVal =analogRead(INVERSION_POT);
  inversionSelection = int(invSensorVal * (INVERSION_COUNT/ANALOG_READ_RESOLUTION));
  
  analogRead(KEY_POT);
  float keySensorVal = analogRead(KEY_POT);
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
  
  // rebuild in key maybe
  // bring it all the way to 0..12 range
  int lowNote = int(pitch) % 12;
  // figure out if it is in the scale;
  if ((lowNote - key) < 0) {lowNote += 12;}
  int interval = lowNote - key;
  
  // in a natural minor scale, the diatonic chords for iii, vi and vii are major
  if (isMinor == true && interval == 3 || interval == 5 || interval == 10) {
    curLim = 0;
    j = 0;
    for (int i = 0; i < VOICES; i ++ ) {
      if (chords[chordSelection][i] == 1) {
        curLim++;
        thisChord[j] = majorBank[i] + pitch;
        j++;
      }
    }
  }

    // in a natural minor scale, the ii is diminished
  if (isMinor == true && interval == 2) {
    curLim = 0;
    j = 0;
    for (int i = 0; i < VOICES; i ++ ) {
      if (chords[chordSelection][i] == 1) {
        curLim++;
        thisChord[j] = dimBank[i] + pitch;
        j++;
      }
    }
  }

  // in a major scale, the diatonic chords for ii, iii and vi are minor
  if (isMinor == false && interval == 2 || interval == 4 || interval == 9) {
    curLim = 0;
    j = 0;
    for (int i = 0; i < VOICES; i ++ ) {
      if (chords[chordSelection][i] == 1) {
        curLim++;
        thisChord[j] = minorBank[i] + pitch;
        j++;
      }
    }
  }

  // major has diminished vii
  if (isMinor == false && interval == 11) {
    curLim = 0;
    j = 0;
    for (int i = 0; i < VOICES; i ++ ) {
      if (chords[chordSelection][i] == 1) {
        curLim++;
        thisChord[j] = dimBank[i] + pitch;
        j++;
      }
    }
  }

  // both have diminished chords but icbf
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

void clearChord(byte pitch) {
  for (int i = 0; i < POLY; i++) {
    thisChord[i] = pitch;
  }
}

void clearNCopyChord(byte pitch) {
  for (int i = 0; i < POLY; i++) {
    prevChord[i] = thisChord[i];
    thisChord[i] = pitch;
  }
}

