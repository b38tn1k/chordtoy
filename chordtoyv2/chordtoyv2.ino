#include <MIDI.h>
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
#define KEY_POT 5             //pot to scroll through western scale, enables selection of the root note for the major / minor mode
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
#define CHORD_VARIATIONS 15 // DX polyphony limit :-/
#define VOICES 7 // how many notes (not including root)
#define POLY 4
#define INVERSION_COUNT 6//hmm
#define THERE_ARE_12_NOTES_IN_WESTERN_MUSIC 12
#define MIDI_RANGE_WITH_SCALE 71
#define MAXIMUM_CHORD_SIZE 12 //that's pretty big though
MIDI_CREATE_DEFAULT_INSTANCE();

int chordSelection = 0;
int inversionSelection = 0;
int inversionSelection2 = 0;
byte thisChord[] = {0, 0, 0}; // DX polyphony limit //v2 update, turns up polyphony limit on the DX is more than 4, i cant remember how much more. 
byte thisChordSecond[] = {0, 0, 0}; // DX polyphony limit //v2 update, turns up polyphony limit on the DX is more than 4, i cant remember how much more. 
byte secondChord[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}; // 12 notes and then major/minor, chord value, inversion, size
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
bool bypass = false;
bool bypassDebounce = false;
bool legato = false;
bool legatoDebounce = false;
int chordSize = 0;
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
  if ((channel != MIDI_BLOCK_CHORD_CHANNEL && channel != MIDI_STRUM_CHANNEL) || bypass == true) {
    MIDI.sendNoteOn(pitch, velocity, channel);
  } else {
    if (channel == MIDI_STRUM_CHANNEL) {
      trigPrevMillis = millis();
      clearNCopyChord(pitch);
      cur = 0;
      curLim = 0;
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
      
      // apply inversion
    for (int k = 0; k < curLim; k++) {
        thisChord[k] = byte(int(thisChord[k]) + inversions[inversionSelection][k]);
      }
    }

    if (channel == MIDI_BLOCK_CHORD_CHANNEL) {
      ///NEW    
     int curLimSecond = 0;
     int j = 0;
     for (int i = 0; i < VOICES; i ++ ) {
       if (chords[chordSelection][i] == 1) {
         curLimSecond++;
         thisChordSecond[j] = bank[i] + pitch;
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
       curLimSecond = 0;
       j = 0;
       for (int i = 0; i < VOICES; i ++ ) {
         if (chords[chordSelection][i] == 1) {
           curLimSecond++;
           thisChordSecond[j] = majorBank[i] + pitch;
           j++;
         }
       }
     }
    
       // in a natural minor scale, the ii is diminished
     if (isMinor == true && interval == 2) {
       curLimSecond = 0;
       j = 0;
       for (int i = 0; i < VOICES; i ++ ) {
         if (chords[chordSelection][i] == 1) {
           curLimSecond++;
           thisChordSecond[j] = dimBank[i] + pitch;
           j++;
         }
       }
     }
    
     // in a major scale, the diatonic chords for ii, iii and vi are minor
     if (isMinor == false && interval == 2 || interval == 4 || interval == 9) {
       curLimSecond = 0;
       j = 0;
       for (int i = 0; i < VOICES; i ++ ) {
         if (chords[chordSelection][i] == 1) {
           curLimSecond++;
           thisChordSecond[j] = minorBank[i] + pitch;
           j++;
         }
       }
     }
    
     // major has diminished vii
     if (isMinor == false && interval == 11) {
       curLimSecond = 0;
       j = 0;
       for (int i = 0; i < VOICES; i ++ ) {
         if (chords[chordSelection][i] == 1) {
           curLimSecond++;
           thisChordSecond[j] = dimBank[i] + pitch;
           j++;
         }
       }
     }

        for (int k = 0; k < MAXIMUM_CHORD_SIZE; k++) {
          MIDI.sendNoteOff(secondChord[k], 0, MIDI_BLOCK_CHORD_CHANNEL); //clear out channel, faster than doing all notes every time which has a noticable lag
        }
        // second chord 
        secondChord[0] = pitch;
        // w/ inversion
        for (int k = 0; k < curLim; k++) {
          secondChord[k+1] = byte(int(thisChord[k] + TRNSPS_2ND_CHAN) + 2*(inversions[inversionSelection2][k]));
        }
        //this could be done in a loop but I like this arrangment so...
        secondChord[4] = byte(secondChord[0] + 12);
        secondChord[5] = byte(secondChord[0] - 12);
        secondChord[6] = byte(secondChord[1] + 12);
        secondChord[7] = byte(secondChord[1] - 12);
        secondChord[8] = byte(secondChord[2] + 12);
        secondChord[9] = byte(secondChord[2] - 12);
        secondChord[10] = byte(secondChord[3] + 12);
        secondChord[11] = byte(secondChord[3] - 12);
        //flags
        secondChord[12] = inversionSelection2;
        secondChord[13] = byte(isMinor);
        secondChord[14] = chordSize;
        secondChord[15] = chordSelection;

        for (int k = 0; k < chordSize; k++) {
          MIDI.sendNoteOn(secondChord[k], velocity, MIDI_BLOCK_CHORD_CHANNEL);
          Serial.flush();
        }
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
