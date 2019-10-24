# chordtoy

not an arpeggiator :-P

[videos are easier than reading](https://youtu.be/XsruSs-3y8w)

[how to install arduino midi library](https://github.com/FortySevenEffects/arduino_midi_library)

I wanted to play chords using the Arturia BeatStep Pro sequencer. This sits in the MIDI chain between the sequencer and the synth and uses the incoming MIDI note to stack a chord.*

1 Switch: Choose between Major and Minor modes. Major mode has a dominant 7th rather than major 7th chord, I haven't used it much cause major scales are basic.

3 Pots:

- Chord Selector: Starting with the root note, add more notes to create a more complex sound. I tried to group the less complex sounds at low values and the more extreme sounds at high values. So, in Minor mode you can turn up to go from one note to a power chord to a minor chord to a minor 7, 9th, 11th, 13th.. with a couple more variations.

- Inversion Mode: at zero the root note is at the bottom. At 100% the root note is at the top of the chord. Variations exist in between.

- Strum Rate: When I first made this I just wanted to play simple block chords for techno-y sounding stabs on the DX. By accident I realized that the strummed chord can sound really nice and give some movement to a beat. Rhythms like like Legowelt or Flying Lotus... so at one extreme the chord plays all at once. At the other extreme the notes are plucked out one at a time slowly. Yes, like an arpeggiator. but no repeating or syncing cause I have a BeatStep that can keep time and play arpeggios :-)

GPIOs per the .ino sketch:
![Circuit Diagram](https://github.com/b38tn1k/chordtoy/blob/master/chordtoy_bb.png)

Prototype using a Pro Trinket (same pinouts as UNO):
![cat tax](https://github.com/b38tn1k/chordtoy/blob/master/cabbagecat.JPG)

\* Akai and Oberheim used to make boxes that did this sort of thing and this functionality is now built into heaps of DAWs, possible some synths. my Yamaha DX doesn't
