#include <MIDI.h>
//So this one does two MIDI channels, just connect two MIDI plugs to the output of the arduino, it should provide enough power
//or look for a blog on how to do it properly :-D

// MIDI SETTINGS - Change these as desired - These are now variables rather than constants so they can be changed by the switches
int TRNSPS_2ND_CHAN = 0;
//the strum Inut channel
int MIDI_STRUM_CHANNEL_IN = 2;
//the pad or arp Input channel
int MIDI_BLOCK_CHORD_CHANNEL_IN = 3;

//the strum Output channel
int MIDI_STRUM_CHANNEL = 2;
//the pad or arp Output channel
int MIDI_BLOCK_CHORD_CHANNEL = 3;

//switch to set MIDI Thru to On or Off and Strum and Chord channels to 16 or to rotary switch values - effectively turning On or Off all MIDI processing
int MIDI_IGNORE = false;

//the bypass Output channel
int MIDI_BYPASS_CHANNEL = 1;

//IO for pots and switches
#define MAJOR_MINOR 2        //switch for major / minor
#define CHORD_POT 4           //pot to scroll through various chord shapes
#define INV_POT1 0            //pot to scroll through inversions for first channel
#define KEY_POT 5             //pot to scroll through Root Note
#define RATE_POT 1            //strum rate
#define LED_1 13              //major or minor signifier? IDK
//#define LED_2 12              //major or minor signifier? IDK
#define BYPASS_SWITCH 3      //just pass normal MIDI through
#define LEGATO_SWITCH 4       //change note off mode a bit, also its not really legato, is something else 
#define INV_POT2 2            //pot to scroll through inversions for second channel
#define SIZE_POT 3            //pot to increase size of chord for second channel through addition of octaves above and below
// #define TRNSPS_2ND_CHAN -24   //just my taste

//I dont think you need to worry about any other settings unless you wanna make some improvements
#define ANALOG_READ_RESOLUTION 1053.0
#define CHORD_VARIATIONS 15 // DX polyphony limit :-/
#define VOICES 7 // how many notes (not including root)
#define POLY 4
#define INVERSION_COUNT 6//hmm
#define THERE_ARE_12_NOTES_IN_WESTERN_MUSIC 12
#define MIDI_RANGE_WITH_SCALE 71
#define MAXIMUM_CHORD_SIZE 12 //that's pretty big though
//MIDI_CREATE_DEFAULT_INSTANCE();
MIDI_CREATE_INSTANCE(HardwareSerial, Serial1, MIDI);

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
const int inversions[INVERSION_COUNT][POLY - 1] = {{0, 0, 0}, {0, 0, -12}, { -12, 0, 0}, { -12, -12, 0}, {0, -12, -12}, { -12, -12, -12}};
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
// Dave's Interval Idea
int strumInterval = 3; // attach this one to a rotary also
int strumOnIntervalCounter = 0;
int strumOffIntervalCounter = 0;
int blockInterval = 5; // attach this one to a rotary in void pollInputs() {
int blockOnIntervalCounter = 0;
int blockOffIntervalCounter = 0;

// duration stuff
#define DURATION_POT 12 // TBD
int noteOffDelay = 0;
int noteOffTimes[128];
float durationScale = 1.0;
int minimumDuration = 50;
int lowest = 128;
int highest = 0;
// end duration stuff

int returnLowest(int a, int b) {
  int c = a;
  if (b < a) {
    c = b;
  }
  return c;
}

int returnHighest(int a, int b) {
  int c = a;
  if (b > a) {
    c = b;
  }
  return c;
}

void setup() {
  pinMode(LED_1, OUTPUT);
//  pinMode(LED_2, OUTPUT);
  pinMode(BYPASS_SWITCH, INPUT);
  pinMode(MAJOR_MINOR, INPUT);
  pinMode(LEGATO_SWITCH, INPUT);

  for (int i=5; i<13; i++){ 
  pinMode(i, INPUT_PULLUP);
  }
  for (int i=22; i<50; i++){ 
  pinMode(i, INPUT_PULLUP);
  }
  
  MIDI.setHandleNoteOn(handleNoteOn);
  MIDI.setHandleNoteOff(handleNoteOff);
  //  MIDI.begin(MIDI_STRUM_CHANNEL); //bypass only works with this one channel. TODO: research MIDI Library more
  MIDI.begin(MIDI_CHANNEL_OMNI);
  pollInputs();
  toggleBank();
  prevMode = isMinor;
  prevKey = key;
 //Serial.begin(9600);
}

void loop() {


  currentMillis = millis();


  MIDI.read();
  if (currentMillis - pollPrevMillis >= pollInterval) {
    pollInputs();
    pollPrevMillis = currentMillis;   
  }

  if (MIDI_IGNORE == true){
    MIDI.turnThruOff();
    MIDI_STRUM_CHANNEL = 16;
    MIDI_BLOCK_CHORD_CHANNEL = 16;
    MIDI_BYPASS_CHANNEL = 16; 
  }
  
  if (bypass == false) {
    // duration stuff
    for (int i = lowest; i < highest; i++) {
      if (noteOffTimes[i] != 0) {
        if (noteOffTimes[i] <= currentMillis) {
          MIDI.sendNoteOff(i, 0, MIDI_STRUM_CHANNEL);
          noteOffTimes[i] = 0;
        }
      }
    }
    // end duration stuff
    if ((currentMillis - trigPrevMillis >= strumRate) && (cur < curLim)) {
      MIDI.sendNoteOn(thisChord[cur], vel, MIDI_STRUM_CHANNEL);
      noteOffTimes[thisChord[cur]] = currentMillis + noteOffDelay;
      lowest = returnLowest(lowest, thisChord[cur]) - 1;
      highest = returnHighest(highest, thisChord[cur]);
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
    if ((currentMillis - keyLEDPrevMillis >= 200) && (keyCounter >= 0)) {
      keyCounter -= 1;
      keyLEDStatus = !keyLEDStatus;
      digitalWrite(activeLED, keyLEDStatus);
      keyLEDPrevMillis = currentMillis;
    }
  } else {
    if (currentMillis - keyLEDPrevMillis >= 200) {
      keyLEDStatus = !keyLEDStatus;
      digitalWrite(LED_1, keyLEDStatus);
//      digitalWrite(LED_2, keyLEDStatus);
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
//    inactiveLED = LED_2;
  } else {
    inactiveLED = LED_1;
//    activeLED = LED_2;
    assignBank(majorBank);
  }
  digitalWrite(activeLED, HIGH);
  digitalWrite(inactiveLED, LOW);
}

void pollInputs() {
  pollSwitches();
  isMinor = digitalRead(MAJOR_MINOR);
  bypass = digitalRead(BYPASS_SWITCH);
  legato = digitalRead(LEGATO_SWITCH);

  analogRead(RATE_POT);
  strumRate = long(ANALOG_READ_RESOLUTION - analogRead(RATE_POT));
  strumRate = strumRate / 3;

  analogRead(CHORD_POT);
  float chordSensorVal = analogRead(CHORD_POT);
  chordSelection =  int(chordSensorVal * (CHORD_VARIATIONS / ANALOG_READ_RESOLUTION));

  analogRead(INV_POT1);
  float invSensorVal = analogRead(INV_POT1);
  inversionSelection = int(invSensorVal * (INVERSION_COUNT / ANALOG_READ_RESOLUTION));

  analogRead(INV_POT2);
  invSensorVal = analogRead(INV_POT2);
  inversionSelection2 = int(invSensorVal * (INVERSION_COUNT / ANALOG_READ_RESOLUTION));

  analogRead(KEY_POT);
  float keySensorVal = analogRead(KEY_POT);
  key = int(keySensorVal * (THERE_ARE_12_NOTES_IN_WESTERN_MUSIC / ANALOG_READ_RESOLUTION));

  analogRead(SIZE_POT);
  float chordSizeVal = analogRead(SIZE_POT);
  chordSize = int(chordSizeVal * (MAXIMUM_CHORD_SIZE / ANALOG_READ_RESOLUTION));

  // duration stuff
  analogRead(DURATION_POT);
  noteOffDelay = analogRead(SIZE_POT);
  noteOffDelay = int( (durationScale * noteOffDelay)) + minimumDuration;
  // end duration stuff
}

void handleNoteOn(byte channel, byte pitch, byte velocity) {
  if (strumOnIntervalCounter % strumInterval == 0) {
    bypass = false;
  } else {
    bypass = true;
  }

  if ((channel != MIDI_BLOCK_CHORD_CHANNEL_IN && channel != MIDI_STRUM_CHANNEL_IN) || bypass == true) {
    //MIDI.sendNoteOn(pitch, velocity, channel); // I removed this because I am making up for it by turning the MIDI.thru on and off using MIDI_IGNORE and selectively letting the BYPASS_CHANNEL through here.
    MIDI.sendNoteOn(pitch, velocity, MIDI_BYPASS_CHANNEL); // I added these lines to all NoteOn and NoteOff sections except Line 185 above where it does the Chordblock 'clearing'
  } else {
    if (channel == MIDI_STRUM_CHANNEL_IN) {
      strumOnIntervalCounter++;
      if (strumOnIntervalCounter % strumInterval == 0) {
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
        if ((lowNote - key) < 0) {
          lowNote += 12;
        }
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
      } else {
        MIDI.sendNoteOn(pitch, velocity, channel);
        MIDI.sendNoteOn(pitch, velocity, MIDI_BYPASS_CHANNEL);
      }
    }


    if (channel == MIDI_BLOCK_CHORD_CHANNEL_IN) {
      blockOnIntervalCounter++;
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
      if ((lowNote - key) < 0) {
        lowNote += 12;
      }
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
        secondChord[k + 1] = byte(int(thisChord[k] + TRNSPS_2ND_CHAN) + 2 * (inversions[inversionSelection2][k]));
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
      if (blockOnIntervalCounter % blockInterval == 0) {
        for (int k = 0; k < chordSize; k++) {
          MIDI.sendNoteOn(secondChord[k], velocity, MIDI_BLOCK_CHORD_CHANNEL);
          Serial.flush();
        }
      } else {
        MIDI.sendNoteOn(pitch, velocity, channel);
        MIDI.sendNoteOn(pitch, velocity, MIDI_BYPASS_CHANNEL);
      }
    }
  }
}

void handleNoteOff(byte channel, byte pitch, byte velocity) {
  if (bypass == true) {
    MIDI.sendNoteOff(pitch, velocity, channel);
    MIDI.sendNoteOff(pitch, velocity, MIDI_BYPASS_CHANNEL);
  } else {
    if (legato == false) {
      cur = curLim;
    }
    if (channel == MIDI_STRUM_CHANNEL_IN) {
      MIDI.sendNoteOff(pitch, 0, MIDI_STRUM_CHANNEL);
      if (strumOnIntervalCounter % strumInterval == 0) {
        for (int i = 0; i < VOICES; i ++ ) {
          MIDI.sendNoteOff(thisChord[i], 0, MIDI_STRUM_CHANNEL);
          MIDI.sendNoteOff(prevChord[i], 0, MIDI_STRUM_CHANNEL);
        }
      }
    } else if (channel == MIDI_BLOCK_CHORD_CHANNEL_IN) {
      MIDI.sendNoteOff(pitch, 0, MIDI_BLOCK_CHORD_CHANNEL);
      if (blockOnIntervalCounter % blockInterval == 0) {
        for (int i = 0; i < MAXIMUM_CHORD_SIZE; i ++ ) {
          MIDI.sendNoteOff(secondChord[i], 0, MIDI_BLOCK_CHORD_CHANNEL);
        }
      }
    } else {
      MIDI.sendNoteOff(pitch, velocity, channel);
      MIDI.sendNoteOff(pitch, velocity, MIDI_BYPASS_CHANNEL);
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

void pollSwitches() {
  //read the switch value into a variable
  int switch_position_5_status = digitalRead(5);
  int switch_position_6_status = digitalRead(6);
  int switch_position_7_status = digitalRead(7);
  
  int switch_position_8_status = digitalRead(8);
  int switch_position_9_status = digitalRead(9);
  int switch_position_10_status = digitalRead(10);
  int switch_position_11_status = digitalRead(11);
  int switch_position_12_status = digitalRead(12);

  int switch_position_22_status = digitalRead(22);
  int switch_position_23_status = digitalRead(23);
  int switch_position_24_status = digitalRead(24);
  int switch_position_25_status = digitalRead(25);

  int switch_position_26_status = digitalRead(26);
  int switch_position_27_status = digitalRead(27);
  int switch_position_28_status = digitalRead(28);
  int switch_position_29_status = digitalRead(29);

  int switch_position_30_status = digitalRead(30);
  int switch_position_31_status = digitalRead(31);
  int switch_position_32_status = digitalRead(32);
  int switch_position_33_status = digitalRead(33);

  int switch_position_34_status = digitalRead(34);
  int switch_position_35_status = digitalRead(35);
  int switch_position_36_status = digitalRead(36);
  int switch_position_37_status = digitalRead(37);
  int switch_position_38_status = digitalRead(38);
  int switch_position_39_status = digitalRead(39);
  int switch_position_40_status = digitalRead(40);
  int switch_position_41_status = digitalRead(41);

  int switch_position_42_status = digitalRead(42);
  int switch_position_43_status = digitalRead(43);
  int switch_position_44_status = digitalRead(44);
  int switch_position_45_status = digitalRead(45);
  int switch_position_46_status = digitalRead(46);
  int switch_position_47_status = digitalRead(47);
  int switch_position_48_status = digitalRead(48);
  int switch_position_49_status = digitalRead(49);        
  
  //execute switch functions
  //execute toggle switches 4, 5 qnd 6

    if (switch_position_5_status == LOW) {
     MIDI_STRUM_CHANNEL_IN = 2; // sets which incoming MIDI channel will go to the Blockchord MIDI output

  } else {
     MIDI_STRUM_CHANNEL_IN = 3; // sets which incoming MIDI channel will go to the Blockchord MIDI output

  }
    if (switch_position_6_status == LOW) {
     MIDI_BLOCK_CHORD_CHANNEL_IN = 2; // sets which incoming MIDI channel will go to the Blockchord MIDI output

  } else {
     MIDI_BLOCK_CHORD_CHANNEL_IN = 3; // sets which incoming MIDI channel will go to the Blockchord MIDI output
  }

    if (switch_position_7_status == LOW) {
     MIDI_IGNORE = false; // sets MIDI Thru to On and Strum and Chord channels to switch values - effectively turning on all MIDI processing

  } else {
    MIDI_IGNORE = true; // sets MIDI Thru to Off and Strum and Chord channels to 16 - effectively turning off all MIDI processing


  }
  
  //execute rotary switch 1 - 5 locations
  if (switch_position_8_status == LOW) {
  TRNSPS_2ND_CHAN = -24;

  } else if (switch_position_9_status == LOW) {
  TRNSPS_2ND_CHAN = -12;

  } else if (switch_position_10_status == LOW) {
  TRNSPS_2ND_CHAN = 0;

  } else if (switch_position_11_status == LOW) {
  TRNSPS_2ND_CHAN = 12;

  } else if (switch_position_12_status == LOW) {
  TRNSPS_2ND_CHAN = 24;
  }

  //execute rotary switch 2 - 4 locations - to set outgoing MIDI Strum channel
    if (switch_position_22_status == LOW) {
  MIDI_STRUM_CHANNEL = 1;

  } else if (switch_position_23_status == LOW) {
  MIDI_STRUM_CHANNEL = 2;

  } else if (switch_position_24_status == LOW) {
  MIDI_STRUM_CHANNEL = 3;

  } else if (switch_position_25_status == LOW) {
  MIDI_STRUM_CHANNEL = 4;
  }
  //execute rotary switch 3 - 4 locations - to set outgoing MIDI Blockchord channel
    if (switch_position_26_status == LOW) {
  MIDI_BLOCK_CHORD_CHANNEL = 1;

  } else if (switch_position_27_status == LOW) {
  MIDI_BLOCK_CHORD_CHANNEL = 2;

  } else if (switch_position_28_status == LOW) {
  MIDI_BLOCK_CHORD_CHANNEL = 3;

  } else if (switch_position_29_status == LOW) {
  MIDI_BLOCK_CHORD_CHANNEL = 4;
  }
  //execute rotary switch 4 - 4 locations - to set outgoing MIDI Bypass channel
    if (switch_position_30_status == LOW) {
  MIDI_BYPASS_CHANNEL = 1;

  } else if (switch_position_31_status == LOW) {
  MIDI_BYPASS_CHANNEL = 2;

  } else if (switch_position_32_status == LOW) {
  MIDI_BYPASS_CHANNEL = 3;

  } else if (switch_position_33_status == LOW) {
  MIDI_BYPASS_CHANNEL = 4;
  }
  //execute rotary switch 5 - 8 locations
    if (switch_position_34_status == LOW) {
  strumInterval = 0;

  } else if (switch_position_35_status == LOW) {
  strumInterval = 1;

  } else if (switch_position_36_status == LOW) {
  strumInterval = 2;

  } else if (switch_position_37_status == LOW) {
  strumInterval = 3;

  } else  if (switch_position_38_status == LOW) {
  strumInterval = 4;

  } else if (switch_position_39_status == LOW) {
  strumInterval = 5;

  } else if (switch_position_40_status == LOW) {
  strumInterval = 6;

  } else if (switch_position_41_status == LOW) {
  strumInterval = 7;
  }

    //execute rtary switch 6 - 8 locations
    if (switch_position_42_status == LOW) {
  blockInterval = 0;

  } else if (switch_position_43_status == LOW) {
  blockInterval = 1;

  } else if (switch_position_44_status == LOW) {
  blockInterval = 2;

  } else if (switch_position_45_status == LOW) {
  blockInterval = 3;

  } else  if (switch_position_46_status == LOW) {
  blockInterval = 4;

  } else if (switch_position_47_status == LOW) {
  blockInterval = 5;

  } else if (switch_position_48_status == LOW) {
  blockInterval = 6;

  } else if (switch_position_49_status == LOW) {
  blockInterval = 7;
  }
}
