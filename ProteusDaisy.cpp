#include "daisy_patch_sm.h"
#include "daisysp.h"
#include <string>
#include <vector>
#include <random>
#include "Note.hpp"
#include "Scales.hpp"
#include "utils.hpp"
#include <time.h> 
#include "Gate.cpp"

using namespace daisy;
using namespace daisysp;
using namespace patch_sm;

DaisyPatchSM hw;

enum sequenceMutationOptions {
    DONT_MUTATE,
    CHANGE_NOTES,
    OCTAVE_UP,
    OCTAVE_DOWN,
    MUTATION_OPTIONS_LEN
};
enum newMelodyNoteOptions {
    NM_REPEAT,
    NM_UP,
    NM_DOWN,
    NM_NEW
};

//Clock trigger
bool triggerState = false;
bool prevTriggerState = false;
float triggerValue;

//New melody CV trigger
bool triggerStateNewMelody = false;
bool prevTriggerStateNewMelody = false;
float triggerValueNewMelody;

//boolean options
bool regen = true;
bool mutate = true;
bool accumulate = true;

//The sequence
static const int maxSteps = 32;
Note sequence[maxSteps]; 

//Melody creation
int noteOptionWeights[4] = {5,5,5,10};
std::string baseKey = "C";
std::string scale = "Major";
uint8_t scaleNum = 1;
uint8_t baseOctave = 4;
Note rootNote = Note(baseKey,baseOctave);
std::vector<int> validTones = scaleTones.at(scale);
std::vector<int> validToneWeights = scaleToneWeights.at(scale);


bool gateState = false;
uint8_t repetitionCount = 0;
uint8_t restingCount = 0;
int octaveOffset = 0;
uint8_t mutationOption = DONT_MUTATE;
uint8_t repeatsInARow = 0;
uint8_t maxOctaveOffsetUp = 2;
uint8_t maxOctaveOffsetDown = 2;
int sequenceLength = 16;
uint8_t sequenceGap = 0;
double restProbability = 20; //out of 100
float octaveChangeProbability = 30;
float noteChangeProbability = 30;
uint8_t currentNote = 0;
float gateDuration = .05f;
uint8_t activeLED = 0;
float LEDbrightness = 0;
bool downOnly, upOnly, upDown;
double trigToTrigTime = 0;
double prevTrigTime = 0;
double prevFrame = 0;
double currentFrame = 0;
int numRecentTriggers = 0;
int numTotalTriggers = 0;
double triggerGapAccumulator = 0;
double triggerGapAverage = 0;
double triggerGapSeconds = 0;
static const int numTriggersToAverage = 6;
float gateLengthKnobPosition;
float octaveCV;
float scaleCV;
float gateLengthCV;
float noteChangeCV;
float lambdaCV;
float densityCV;
float modulateCV;
float lengthCV;
float poisson_lambda = 12;
Note prevNote; 
int noteKind;
int noteOn;
int numChannels;
float sampleRate;
bool oscControls;
float formant_freq_factor;
int selectedOsc;
bool gate_out_state;
float gate_onset_time;
uint32_t now;

Switch button, toggle;
Gate output_gate;

Note getRandomNote() {

    //Choose a random note from the current scale

    int ourSemitone;
    int newNoteMIDI;
    int ourChoice;

    int num_choices = validToneWeights.size();
    ourChoice = weightedRandom(&validToneWeights[0],num_choices);

    ourSemitone = validTones[ourChoice];
    newNoteMIDI = rootNote.noteNumMIDI + ourSemitone - 1;
    Note newNote = Note(newNoteMIDI);
    return newNote;

};

void changeOctave(int octaveChange) {
    
    //transpose by octave

    hw.PrintLine("---------Octave change---------");
    if (octaveOffset+octaveChange > maxOctaveOffsetUp) {

    } else if (octaveOffset+octaveChange <  -1*maxOctaveOffsetDown) {

    } else {

        for (int t=0; t < maxSteps; ++t) {
            sequence[t].octave += octaveChange;
            sequence[t].setVoltage();
            sequence[t].setMIDInum();

        }
        octaveOffset += octaveChange;

    }

}

void changeNotes(int amount) {
    
    hw.PrintLine("---------Note change---------");
    //substitute notes in the melody with new notes

    int noteToChange = std::rand() % sequenceLength;
    Note newNote = getRandomNote();
    sequence[noteToChange] = newNote;

}


void newMelody() {

    //Generate a new melody sequence

    hw.PrintLine("----NEW MELODY----");

    hw.PrintLine("Getting scale %d",scaleNum);

    scale = scaleNames.at(scaleNum);
    validTones = scaleTones.at(scale);
    validToneWeights = scaleToneWeights.at(scale);
    octaveOffset = 0;
    repetitionCount = 0;

    for (int x = 0; x < maxSteps; x++ ){
        
        if (x>0) {

            //Use rest probability (density?) to decide if we have a rest
            int noteOnChoice = std::rand() % 100;
            if (noteOnChoice < restProbability) {
                noteOn = 0;
            } else {
                noteOn = 1;
            }

        } else {
            //first note is never a rest
            noteOn = 1;
        }
        if (noteOn) {

            //We have an actual note, not a rest
            //Decide what kind of note
            //First note is always a new random note
            
            if (x>0){
                noteKind = weightedRandom(noteOptionWeights,4);
                hw.PrintLine("NoteKind: %d",noteKind);
        
            } else {
                noteKind = NM_NEW;
                hw.PrintLine("Note kind: NEW");
            }
            
            if (noteKind == NM_REPEAT) {
                sequence[x] = prevNote;
                hw.PrintLine("Note kind: REPEAT");

            } else if (noteKind == NM_DOWN) {

                hw.PrintLine("Note kind: DOWN");
                //find tone of previous note in the scale, find index of toneNum in validTones
                std::vector<int>::iterator it = std::find(validTones.begin(),validTones.end(),prevNote.toneNum);
                int toneIndex = std::distance(validTones.begin(), it);

                toneIndex--;
                int newOctave = prevNote.octave;
                if (toneIndex < 0) {
                    toneIndex = validTones.size()-1;
                    newOctave--;
                }
                int newTone = validTones[toneIndex];
            
                std::string newNoteName = prevNote.getNoteNameFromNum(newTone);

                Note aNewNote = Note(newNoteName,newOctave);
                sequence[x] = aNewNote;

            } else if (noteKind == NM_UP) {

                hw.PrintLine("Note kind: UP");
                //find tone of previous note in the scale, find index of toneNum in validTones
                std::vector<int>::iterator it = std::find(validTones.begin(),validTones.end(),prevNote.toneNum);
                int toneIndex = std::distance(validTones.begin(), it);

                toneIndex++;
                int newOctave = prevNote.octave;
                if (toneIndex >= int(validTones.size())) {
                    toneIndex = 0;
                    newOctave++;
                }
                int newTone = validTones[toneIndex];
            
                std::string newNoteName = prevNote.getNoteNameFromNum(newTone);

                Note aNewNote = Note(newNoteName,newOctave);
                sequence[x] = aNewNote;


            } else if (noteKind == NM_NEW) {
                hw.PrintLine("Note kind: NEW");
                Note newNote = getRandomNote();
                sequence[x] = newNote;
                prevNote = newNote;
            }
            prevNote = sequence[x];

        } else {
            //We have a rest beat
            Note newNote = Note("rest");
            sequence[x] = newNote;
        }

    }
    //log new seqence
    hw.PrintLine("NEW MELODY:--");
    for (int i = 0; i <=31; i++){
        hw.PrintLine("--: %s %d",sequence[i].noteName.c_str(),sequence[i].octave);
    }

};


void doStep() {

	bool melodyChanged = false;

    Note noteToPlay = sequence[currentNote];

    hw.PrintLine("Current note is %s%d voltage %.3f",sequence[currentNote].noteName.c_str(),sequence[currentNote].octave,sequence[currentNote].voltage);

    if (noteToPlay.noteName != "rest") {
        //turn gate out on
        hw.WriteCvOut(CV_OUT_1,sequence[currentNote].voltage);
        output_gate.ReTrigger();
    } 

    if (currentNote == 0) {
        float ledlevel = repetitionCount/poisson_lambda * 3 + 2;
        hw.WriteCvOut(CV_OUT_2,ledlevel);
        hw.PrintLine("Rep count %d Led level %.2f",repetitionCount,ledlevel);
    } else {
        hw.WriteCvOut(CV_OUT_2,0);
    }

    ++currentNote;

    if (currentNote >= sequenceLength) {
			
        //We are at end of loop, so we need to check for various changes
        //accumulate means we are accruing repetitions
        if (accumulate) {
            repetitionCount++;

            //decide if we get a new melody
            int choice = std::rand() % 100;
            float criterion = float(repetitionCount)/float(poisson_lambda) * 50;
            
            if (choice < criterion) {
                newMelody();
                melodyChanged = true;
            } else {
                melodyChanged = false;
            }
        }

        if (!melodyChanged && mutate) {

            int octaveChoice = std::rand() % 100;
            if (octaveChoice < octaveChangeProbability) {
                
                if (downOnly) {
                    changeOctave(-1);
                } else if (upOnly) {
                    changeOctave(1);
                } else {
                    int coinFlip = std::rand() %100;
                    if (coinFlip < 50) {
                        if (octaveOffset <=  -1*maxOctaveOffsetDown) {
                            //We're already at the min octave so we bounce up
                            changeOctave(1);
                        } else {
                            changeOctave(-1);
                        }
                    } else {
                        if (octaveOffset >= maxOctaveOffsetUp) {
                            //We're already at the max octave so we bounce down
                            changeOctave(-1);
                        } else {
                            changeOctave(1);
                        }
                    }
                }


            } else {         
                int noteChoice = std::rand() % 100;
                if (noteChoice < noteChangeProbability) {
                    changeNotes(1);
                }
            };
        }
        currentNote = 0;
    }

};


int processKnobValue(float value, int maxValue) {

    float voltsPerNum = 1.0/maxValue;
    float rawVal = value/voltsPerNum;
    return std::ceil(rawVal);

}

// void DacCallback(uint16_t **output, size_t size)
// {
//     for(size_t i = 0; i < size; i++)
//     {
//         output_gate.Process(); //advance the gate
//         //output[0][i]   = sequence[currentNote].voltage * 4095; /// 4095 scaling? 
        
//     }
// }

static void AudioCallback(AudioHandle::InputBuffer  in,
                          AudioHandle::OutputBuffer out,
                          size_t                    size)
{
   for(size_t i = 0; i < size; i++)
    {
        output_gate.Process(); //advance the gate
        //output[0][i]   = sequence[currentNote].voltage * 4095; /// 4095 scaling? 
        
    }
}

int main(void)
{

    //Seed random number generator
    srand(time(NULL));

    // Initialize Versio hardware and start audio, ADC
    hw.Init();
    hw.StartLog(false);
    hw.StartAdc();
    hw.StartAudio(AudioCallback);

    button.Init(hw.B7);
    toggle.Init(hw.B8);

    hw.PrintLine("Logging enabled.");

    hw.WriteCvOut(CV_OUT_2,5);

    //Initial read of parameters for first sequence
    hw.ProcessAllControls(); 
    button.Debounce();
    toggle.Debounce();

    //Initialize Gate
    output_gate.Init(48000);

    //Process toggle switch
    if (toggle.Pressed()) {
        mutate = true;
        accumulate = true;
    } else  {
        mutate = false;
        accumulate = false;
    } 

    //Process knobs
    sequenceLength = processKnobValue(hw.GetAdcValue(CV_1),maxSteps);
    scaleNum = processKnobValue(hw.GetAdcValue(CV_3),scaleNames.size());
    if (scaleNum < 1) {scaleNum = 1;}
    restProbability = 100 - hw.GetAdcValue(CV_2) * 100;
    poisson_lambda = ceil(320/sequenceLength);

    //Make first melody
    newMelody();

    //Start the DAC callback
    //hw.StartDac(DacCallback);

    while(1)
    {
        hw.ProcessAllControls(); 
        button.Debounce();
        toggle.Debounce();
 
        //Handle button press
        if (button.RisingEdge()) {
                hw.PrintLine("Button pressed.");
                newMelody();
        }

        //Process toggle switch
        if (toggle.Pressed()) {
            mutate = true;
            accumulate = true;
        } else  {
            mutate = false;
            accumulate = false;
        } 

		//Process knobs
        sequenceLength = processKnobValue(hw.GetAdcValue(CV_1),maxSteps);
        scaleNum = processKnobValue(hw.GetAdcValue(CV_3),scaleNames.size());
        if (scaleNum < 1) {scaleNum = 1;}
        restProbability = 100 - hw.GetAdcValue(CV_2) * 100;
        poisson_lambda = ceil(320/sequenceLength);

        //update the gate output
        dsy_gpio_write(&hw.gate_out_1, output_gate.GetCurrentState());
        

        //check for trigger in
        if (hw.gate_in_1.Trig()) {

            //Calculate time since last trigger to adjust gate times
            //(Output gate is a fraction of clock time)
            currentFrame = daisy::System::GetNow();
            trigToTrigTime = currentFrame - prevFrame;
            prevFrame = currentFrame;
            numRecentTriggers += 1;
            numTotalTriggers += 1;
            if (numTotalTriggers > 2) {
                triggerGapAccumulator += trigToTrigTime;
                triggerGapAverage = triggerGapAccumulator/numRecentTriggers;
                triggerGapSeconds = triggerGapAverage / 1000;
                gateDuration = triggerGapSeconds* hw.GetAdcValue(CV_4);
                if (gateDuration < 0.01f) { gateDuration = 0.01f;}
            } else {
                 gateDuration = 0.125f;
            }
            
            output_gate.SetDuration(gateDuration);

            if (numRecentTriggers > numTriggersToAverage) {
                numRecentTriggers = 0;
                triggerGapAccumulator = 0;
            }

            doStep();

        }

        //Gate on the FSU input triggers new melody
        triggerStateNewMelody = hw.gate_in_2.State();
        if (triggerStateNewMelody && !prevTriggerStateNewMelody) {
            hw.PrintLine("Gate In 2 - New Melody");
            newMelody();
        }
        prevTriggerStateNewMelody = triggerStateNewMelody;
   
    }
}