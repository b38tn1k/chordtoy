#include <MIDI.h>
#define MODE_SWITCH 13
#define TOGGLE_BUTTON 12
#define CHORD_POT 1
#define INVERSION_POT 2
#define RATE_POT 3
#define LED_1 4
#define LED_2 5
#define LED_3 6
#define ANALOG_READ_RESOLUTION 1053.0
#define CHORD_VARIATIONS 9 // DX polyphony limit :-/
#define VOICES 6 // how many notes (not including root)
#define POLY 4
MIDI_CREATE_DEFAULT_INSTANCE();

int chordSelection = 0;
byte thisChord[] = {0, 0, 0}; // DX polyphony limit
byte prevChord[] = {0, 0, 0};
int strumRate = 0;
bool trig = false;
byte chan, vel;

unsigned long currentMillis;
unsigned long pollPrevMillis = 0;
const long pollInterval = 100;

//Reface DX Polyphony
const int chords[CHORD_VARIATIONS][7] = {
    {0, 1, 0, 0, 0, 0, 0}, 
    {1, 1, 0, 0, 0, 0, 0}, 
    {1, 1, 1, 0, 0, 0, 0}, 
    {1, 1, 0, 1, 0, 0, 0}, 
    {1, 1, 0, 0, 1, 0, 0}, 
    {1, 1, 0, 0, 0, 1, 0},  
    {0, 1, 1, 1, 0, 0, 0}, 
    {0, 1, 1, 0, 1, 0, 0},
    {0, 1, 1, 0, 0, 1, 0}}; 
const byte minorBank[] = {3, 7, 10, 14, 17, 21};
byte bank[] = {3, 7, 10, 14, 17, 21};
//const int majorBank[] = {4, 7, 10, 12, 14, 18, 21};

void setup() {
  pinMode(LED_1, OUTPUT);
  pinMode(LED_2, OUTPUT);
  pinMode(LED_3, OUTPUT);
  pinMode(MODE_SWITCH, INPUT);
  pinMode(TOGGLE_BUTTON, INPUT);
  MIDI.setHandleNoteOn(handleNoteOn);
  MIDI.setHandleNoteOff(handleNoteOff);
  MIDI.begin(MIDI_CHANNEL_OMNI);
  assignBank(minorBank);
  pollInputs();
}
unsigned long trigPrevMillis;
int cur = 0; 
int curLim = 0;

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
}

void pollInputs() {
  strumRate = analogRead(RATE_POT); // scaling seems fine
  float chordSensorVal = analogRead(CHORD_POT);
  chordSelection =  int(chordSensorVal * (CHORD_VARIATIONS/ANALOG_READ_RESOLUTION));
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
  int j = 0;
  for (int i = 0; i < VOICES; i ++ ) {
    if (chords[chordSelection][i] == 1) {
      curLim++;
      thisChord[j] = bank[i] + pitch;
      j++;
    }
  }
}

void handleNoteOff(byte channel, byte pitch, byte velocity) {
  MIDI.sendNoteOff(pitch, 0, channel);
  for (int i = 0; i < VOICES; i ++ ) {
    MIDI.sendNoteOff(thisChord[i], 0, channel);
    MIDI.sendNoteOff(prevChord[i], 0, channel);
  }
}

void assignBank(byte minor[]) {
  // placeholder
  if (minor == true) {
    for (int i = 0; i < VOICES; i++) {
      bank[i] = minorBank[i];
    }
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

