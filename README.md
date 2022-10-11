Proteus generative sequencer for the Daisy Patch.Init()

Proteus is a generative sequencer. It creates a melody, then loops that melody for a while, until it gets bored and creates a new one. While it's looping the melody, it may transpose by octave or mutate individual notes as controlled by the knobs or CV.

### Controls

**CV_1: Length** - Sets the sequence length from 1 to 32 steps. You can change the sequence length on the fly without disturbing the melody. For instance, you can make it shorter and then longer again without changing the underlying melody. There is always a 32 step sequence in memory whether you are looping the whole thing or not. 

**CV_2: Density** - This controls the density of the melodies. As you turn this knob towards 100, the probability of rest notes decreases to 0 and all the steps will be filled with notes. Density is changed dynamically; every time the parameter changes, each note's probabilty of becoming a rest is independently re-calculated according to this parameter. 

**CV_3: Scale** - Sets the scale that notes are drawn from. Currently scales are, from clockwise to counter-clockwise:
Major
Natural Minor
Harmonic Minor
Major Pentatonic
Minor Pentatonic
Dorian
Mixolydian
Chromatic

**CV_4: Gate** - This is the length of the gate output as a fraction of the incoming clock time (average trigger-to-trigger time).

**Switch** - When in the top position, Proteus always has some chance of creating a new melody based on the underlying probability function that increases with each loop. When switched to the bottom position, that probability will freeze in place and Proteus will not ever generate a new melody. 

**Button (New sequence)** - Pushing the center button will cause Proteus to immediately generate a new melody. 

### Inputs

Len, Lambda, Density, Mutate, Scale, Gate, and Octave are CV controls for their respective parameters. 

**B10/gate_in_1: CLOCK** - This is the clock input. Each trigger incoming on this port advances the sequence one step. 

**B9/gate_in_2: NEW MELODY** - Trigger into this channel generates a new melody. 

**CV_5: LENGTH** - Adjust sequence length with CV. 

**CV_6: NO FUNCTION YET** - TBD

**CV_7: DENSITY** - Adjust note density with CV. 

**CV_8: GATE LENGTH** - Adjust gate length with CV.

### Outputs

**CV_OUT_1: V/OCT** - Volt per octave out. 

**B5/gate_out_1: GATE OUT** - Gate output
