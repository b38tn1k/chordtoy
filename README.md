# chordtoy

not an arpeggiator :-P

A dual channel MIDI chord generator with 'strumming' on one channel

I am not that great at building cases, here are my v1 and v2 prototypes.
![no cat :(](https://github.com/b38tn1k/chordtoy/blob/master/nocabbagecat.jpg)

[videos are easier than reading](https://www.youtube.com/playlist?list=PLZttbibA79ov0Y0J_pEOQnuLViuv8ho1e)

[how to install arduino midi library](https://github.com/FortySevenEffects/arduino_midi_library)

***built on a tested with MIDI Library version 4.3.1***

I wanted to play chords using the Arturia BeatStep Pro sequencer. This sits in the MIDI chain between the sequencer and the synth and uses the incoming MIDI note to stack a chord.*

## Features

- **Major / Minor Switch**. Major mode has a dominant 7th rather than major 7th chord, I haven't used it much.

- **Key Selector Pot:** Choose the key, other chords are mapped to scale for diatonic chords (I hope/think)

- **Chord Selector Pot:** Starting with the root note, add more notes to create a more complex sound. I tried to group the less complex sounds at low values and the more extreme sounds at high values. So, in Minor mode you can turn up to go from one note to a power chord to a minor chord to a minor 7, 9th, 11th, 13th.. with a couple more variations.

- **Inversion Mode Pot:** at zero the root note is at the bottom. At 100% the root note is at the top of the chord. Variations exist in between.

- **Strum Rate Pot:** When I first made this I just wanted to play simple block chords for techno-y sounding stabs on the DX. By accident I realized that the strummed chord can sound really nice and give some movement to a beat. Rhythms like [Legowelt](https://youtu.be/TGCxBVu1-sE) or [Flying Lotus](https://youtu.be/Pc0OaUQJtd4)... so at one extreme the chord plays all at once. At the other extreme the notes are plucked out one at a time slowly. Yes, like an arpeggiator. but no repeating or syncing cause I have a BeatStep that can keep time and play arpeggios :-)

- a **Legato Switch** changes the way MIDI noteoff messages are sent, allowing the strummed chord to ring out from a trigger or be cut short.

- a **Bypass Switch**, turn off the effect. More useful than turning everything down as in V1.

- a **Second MIDI Channel**. The ```MIDI_STRUM_CHANNEL``` does the 'strumming' chord effect. The ```MIDI_BLOCK_CHORD_CHANNEL``` is used to transmit block chords with the same notes as the Master chord, but with its own unique settings. It works nicely with my (newish) Behringer Crave in arp mode, giving me a 'free time' chord flourish from the DX and accompanying in time Arpeggios from the Crave. If you dont want the second channel stuff just don't let any devices listen on that channel and it should be fine, maybe tie the 'chord size' analog pin to ground :)

- **Second Inversion Mode**: as above but for the ```MIDI_BLOCK_CHORD_CHANNEL```, and with notes shifted 2 octaves cause I like it more.

- **Chord Size**: increase the number of notes in the ```MIDI_BLOCK_CHORD_CHANNEL``` by adding octaves above and below.

## Building

See below for my early prototypes, there is an ***in development*** kicad project included in this repo. Maybe someone good at EE can help? hmu!

Basically, you want to connect MIDI DINs to the Tx and Rx of the Arduino [I used this instructable](https://www.instructables.com/Send-and-Receive-MIDI-with-Arduino/). There are 6 pots, 3 switches and 2 LEDS in the current prototype, you can see the pin assignments in the .ino file.

Version 1 GPIOs per the .ino sketch:
![Circuit Diagram](https://github.com/b38tn1k/chordtoy/blob/master/chordtoy_bb.png)
Version 2 is similar with pot and switch IOs listed at top of the .ino file.

Prototype using a Pro Trinket (same pinouts as UNO) before I made it diatonic:
![cat tax](https://github.com/b38tn1k/chordtoy/blob/master/cabbagecat.JPG)

## Almost TODOs, Maybe just ideas

- MIDI CC Control for the various pots and switches
- Does [FortySevenEffect](https://github.com/FortySevenEffects/arduino_midi_library) MIDI library support transport, tempo, etc pass through?

\* Akai and Oberheim used to make boxes that did this sort of thing and this functionality is now built into heaps of DAWs, possible some synths. my Yamaha DX doesn't


Thank you to David Fodel for testing, advice and motivation!
