/*
  ==============================================================================

    Arpeggiator.h
    Created: 3 Dec 2025 5:55:02pm
    Author:  Olivier Doaré

  ==============================================================================
*/

#pragma once

#include "MidiTools.h"
#include <JuceHeader.h>

/**
    A base class for creating MIDI arpeggiators.

    This class takes a Chord, an octave, and a pattern string to generate
    a sequence of MIDI notes. The getNext() method processes the pattern
    and returns MIDI messages for note-on and note-off events.

    The pattern string consists of characters that define the arpeggio's behavior at each step:
    - '0' to '6': Plays a specific degree of the chord (0=fundamental, 1=third, ..., 6=thirteenth).
      - The `playNoteOff` property determines behavior for absent degrees ("Off", "Next", "Previous").
    - '_': Sustains the previously played note.
    - '.': A rest; no note is played.
    - '+': Plays the next degree in the chord (loops from 6 to 0).
    - '-': Plays the previous degree in the chord (loops from 0 to 6).
    - '#': Plays the degree two steps higher in the chord (e.g., from 1 to 3).
    - '=': Plays the degree two steps lower in the chord (e.g., from 3 to 1).
    - '*': Plays a random, valid note from the current chord.
    - '"': Repeats the last played degree.

    Octave Modifiers (prefixed to a note command):
    - 'oN': Sets the octave to N (where N is a digit from 0-7). Example: "o30" sets octave to 3 and plays the root.
    - 'o+': Increases the octave by one. Example: "o+0" plays the root one octave higher.
    - 'o-': Decreases the octave by one. Example: "o-0" plays the root one octave lower.

    Note: Octave modifiers are prefixes. "o-o-" means "decrease octave, then decrease octave again".
    To decrease the octave and then play the previous degree, you would use "o--".
*/
class Arpeggiator
{
public:
    /**
        Default constructor.
        Initializes with a default C Major chord, a simple pattern, and a base octave.
    */
    Arpeggiator()
        : chord(MidiTools::Chord("CM")), pattern("012"), octave(baseOctave)
    {
    }
public:
    /**
        Constructs an Arpeggiator.
        @param initialChord The chord to be arpeggiated.
        @param arpPattern The string pattern defining the arpeggio.
        @param baseOctave The starting MIDI octave.
    */
    Arpeggiator(const MidiTools::Chord& initialChord, const juce::String& arpPattern, int baseOctave)
        : chord(initialChord), pattern(arpPattern), octave(baseOctave)
    {
    }

    virtual ~Arpeggiator() = default;

    /**
        Call this before playback to set the sample rate.
        @param rate The host's sample rate.
    */
    void prepareToPlay(double rate)
    {
        sampleRate = rate;
        updateSamplesPerNote();
    }

    /**
        Generates MIDI events for the current block of audio samples.
        @param numSamples The number of samples in the current audio block.
        @return A juce::MidiBuffer containing any events generated during this block.
    */
    juce::MidiBuffer processBlock(int numSamples)
    {
        // std::cout << "In Arpeggiator::processBlock()" << std::endl;
        // std::cout << "samplesPerNote = " << samplesPerNote << std::endl;        
        // std::cout << "samplesUntilNextNote = " << samplesUntilNextNote << std::endl;

        // std::cout << "numSamples = " << numSamples << std::endl;
        // std::cout << "pattern = " << pattern << std::endl;
        // std::cout << "sampleRate = " << sampleRate << std::endl;
        
        juce::MidiBuffer generatedMidi;
        if (sampleRate <= 0.0 || samplesPerNote <= 0.0 || pattern.isEmpty())
            return generatedMidi;

        int time = 0;
        while (time < numSamples)
        {
            if (samplesUntilNextNote <= 0.0)
            {
                generatedMidi.addEvents(getNext(), 0, -1, time);
                // Use 'while' to handle cases where the block size is larger than the note duration.
                while (samplesUntilNextNote <= 0.0)
                    samplesUntilNextNote += samplesPerNote;
            }
 
            // Ensure we always advance time, even if samplesUntilNextNote is 0.
            const int samplesToAdvance = (int)std::ceil(samplesUntilNextNote);
            const int samplesThisStep = juce::jmin(numSamples - time, juce::jmax(1, samplesToAdvance));
 
            time += samplesThisStep;
            samplesUntilNextNote -= samplesThisStep;
        }
        // std::cout << "Num events = " << generatedMidi.getNumEvents() << std::endl;
        return generatedMidi;
    }

private:
    /**
        Processes the next step in the arpeggio pattern and returns MIDI messages.
        @return A juce::MidiBuffer containing note-on and/or note-off messages.
    */
    juce::MidiBuffer getNext()
    {
        juce::MidiBuffer midiBuffer;
        int samplePosition = 0; // All events happen at the start of the block

        if (pattern.isEmpty())
            return midiBuffer;


        // --- 2. Parse the pattern using a robust loop ---
        int noteToPlay = -1;
        int currentDegreeIndex = lastPlayedDegreeIndex;
        bool noteCommandFound = false;

        while (!noteCommandFound)
        {
            if (pattern.isEmpty()) return {}; // Safety break

            char command = pattern[pos];
            pos = (pos + 1) % pattern.length(); // Consume character

            // If sustain, do nothing and exit immediately. The previous note will continue playing.
            if (command == '_')
            {
                return midiBuffer; // Return an empty buffer, sustaining the note.
            }
            if (command == 'o') // Octave prefix
            {
                char octaveCommand = pattern[pos];
                pos = (pos + 1) % pattern.length(); // Consume octave command

                if (octaveCommand == '+')
                    octave = juce::jmin(7, octave + 1);
                else if (octaveCommand == '-')
                    octave = juce::jmax(0, octave - 1);
                else if (juce::CharacterFunctions::isDigit(octaveCommand))
                    octave = octaveCommand - '0'; // Correctly convert char '0'-'7' to int 0-7
                // Continue loop to find the note command
            }
            else if (juce::CharacterFunctions::isDigit(command))
            {
                currentDegreeIndex = command - '0'; // Correctly convert char '0'-'9' to int 0-9
                noteCommandFound = true;
            }
            else // It's a non-octave command
            {
                switch (command)
                {
                    case '+': currentDegreeIndex = (currentDegreeIndex + 1) % 7; noteCommandFound = true; break;
                    case '-': currentDegreeIndex = (currentDegreeIndex + 6) % 7; noteCommandFound = true; break;
                    case '#': currentDegreeIndex = (currentDegreeIndex + 2) % 7; noteCommandFound = true; break;
                    case '=': currentDegreeIndex = (currentDegreeIndex + 5) % 7; noteCommandFound = true; break;
                    case '*': currentDegreeIndex = getRandomPresentDegree(); noteCommandFound = true; break;
                    case '"': /* currentDegreeIndex remains the same */ noteCommandFound = true; break;
                    case '.': currentDegreeIndex = -1; /* Rest */ noteCommandFound = true; break;
                    default:  // Invalid characters are ignored.
                        // Do nothing, just loop to the next character.
                        break;
                }
            }
        }

        // --- Turn off the previous note ---
        // This now happens *after* we've decided what the next command is.
        if (lastPlayedMidiNote != -1)
        {
            midiBuffer.addEvent(juce::MidiMessage::noteOff(1, lastPlayedMidiNote), samplePosition);
            lastPlayedMidiNote = -1;
        }

        // --- 3. Determine the final MIDI note to play ---
        if (noteToPlay == -1 && currentDegreeIndex != -1) // If not sustaining and not a rest
        {
            // std::cout << "We should play a note!" << std::endl;
            // std::cout << "currentDegreeIndex = " << currentDegreeIndex << std::endl;

            int semitone = getSemitoneForDegree(currentDegreeIndex);
            // std::cout << "     semitone = " << semitone << std::endl;

            if (semitone != -1)
            {
                noteToPlay = semitone + (octave * 12);
            }
        }
        
        // --- 4. Generate MIDI event ---
        if (noteToPlay != -1 )
        {
            midiBuffer.addEvent(juce::MidiMessage::noteOn(1, noteToPlay, (juce::uint8)100), samplePosition);
            lastPlayedMidiNote = noteToPlay;
            lastPlayedDegreeIndex = currentDegreeIndex;
        }

        // std::cout << "     noteToplay = " << noteToPlay << std::endl;
        // std::cout << "     Num events = " << midiBuffer.getNumEvents() << std::endl;

        return midiBuffer;
    }
public:
    // --- Setters for properties ---
    void setChord(const MidiTools::Chord& newChord) { chord = newChord; }
    void setPattern(const juce::String& newPattern) { pattern = newPattern; pos = 0; }
    void setOctave(int newOctave) { octave = juce::jlimit(0, 7, newOctave); }
    void setPlayNoteOffMode(const juce::String& mode) { playNoteOff = mode; }
    void setTempo(double newTempoBPM)
    {
        tempoBPM = newTempoBPM > 0 ? newTempoBPM : 120.0;
        updateSamplesPerNote();
    }

    void setSubdivision(int subdivisionIndex)
    {
        subdivision = subdivisionIndex;
        updateSamplesPerNote();
    }

    /** Calculates the number of musical steps in the pattern string. */
    int numSteps() const
    {
        if (pattern.isEmpty())
            return 0;

        int steps = 0;
        int i = 0;
        while (i < pattern.length())
        {
            char command = pattern[i];
            if (command == 'o')
            {
                i += 2; // Skip 'o' and its argument
            }
            else if (juce::CharacterFunctions::isDigit(command) ||
                     command == '+' || command == '-' || command == '#' ||
                     command == '=' || command == '*' || command == '"' ||
                     command == '.' || command == '_')
            {
                steps++; // This is a valid step command
                i++; // Consume the command
            }
            else
            {
                i++; // Ignore and consume other characters (like spaces)
            }
        }
        return steps;
    }

    /** Given a step index (0, 1, 2...), find the corresponding character index in the pattern string. */
    int getPatternIndexForStep(int stepIndex) const
    {
        if (pattern.isEmpty())
            return 0;

        int currentStepCount = 0;
        int i = 0;
        while (i < pattern.length())
        {
            if (currentStepCount == stepIndex)
                return i; // Found the start of the desired step

            char command = pattern[i];
            if (command == 'o')
            {
                i += 2; // Skip 'o' and its argument
            }
            else if (juce::CharacterFunctions::isDigit(command) ||
                     command == '+' || command == '-' || command == '#' ||
                     command == '=' || command == '*' || command == '"' ||
                     command == '.' || command == '_')
            {
                currentStepCount++;
                i++; // Consume the command
            }
            else
            {
                i++; // Ignore and consume other characters (like spaces)
            }
        }
        return 0; // Fallback if stepIndex is out of bounds
    }

    /** Returns the total duration of one full pattern loop in PPQ. */
    double ppqDuration() const
    {
        const int steps = numSteps();
        if (steps == 0)
            return 0.0;

        // The duration of one step in PPQ is 1.0 / notesPerQuarter.
        return steps / getNoteDivisor();
    }

    /**
        Synchronizes the arpeggiator's internal clock to the host's transport position.
        This should be called on every process block while the host is playing.
        @param positionInfo The host's current position information.
    */
    void syncToPlayHead(const juce::AudioPlayHead::CurrentPositionInfo& positionInfo)
    {
        if (samplesPerNote <= 0.0 || positionInfo.ppqPosition < 0.0 || pattern.isEmpty())
            return;
    
        const double patternDurationPPQ = ppqDuration();
        if (patternDurationPPQ <= 0.0)
            return;
    
        const double stepDurationPPQ = 1.0 / getNoteDivisor();
        const double songPosInSteps = positionInfo.ppqPosition / stepDurationPPQ;
        const double patternDurationInSteps = patternDurationPPQ / stepDurationPPQ;
    
        // Calculate the current step index within the pattern loop
        // We want to find the NEXT step to be played.
        const int nextStepIndex = static_cast<int>(std::ceil(songPosInSteps)) % static_cast<int>(patternDurationInSteps);
        pos = getPatternIndexForStep(nextStepIndex);
    
        // Calculate how many samples until the next step boundary in the host timeline
        const double nextStepInSong = std::ceil(songPosInSteps);
        const double stepsUntilNext = nextStepInSong - songPosInSteps;
        const double ppqUntilNext = stepsUntilNext * stepDurationPPQ;
        const double secondsPerPPQ = 60.0 / (tempoBPM * 1.0); // 1.0 is quarter note
        samplesUntilNextNote = ppqUntilNext * secondsPerPPQ * sampleRate;
    }

    /** Resets the arpeggiator's position to the beginning of the pattern. */
    juce::MidiBuffer reset()
    {
        juce::MidiBuffer noteOffBuffer;
        if (lastPlayedMidiNote != -1)
        {
            noteOffBuffer.addEvent(juce::MidiMessage::noteOff(1, lastPlayedMidiNote), 0);
            lastPlayedMidiNote = -1;
        }

        octave = baseOctave;
        pos = 0;
        lastPlayedDegreeIndex = 0;
        samplesUntilNextNote = 0;

        return noteOffBuffer;
    }

    /** Generates a note-off for the last played note and resets the state. */
    juce::MidiBuffer turnOff()
    {
        juce::MidiBuffer noteOffBuffer;
        if (lastPlayedMidiNote != -1)
        {
            noteOffBuffer.addEvent(juce::MidiMessage::noteOff(1, lastPlayedMidiNote), 0);
            lastPlayedMidiNote = -1;
        }
        // Also reset pattern position and other state variables for a clean start next time.
        pos = 0;
        lastPlayedDegreeIndex = 0;
        return noteOffBuffer;
    }

protected:
    /**
        Finds a valid semitone for a given degree index, handling absent notes.
    */
    int getSemitoneForDegree(int degreeIndex)
    {
        if (!juce::isPositiveAndBelow(degreeIndex, 7))
            return -1;

        // If we are using a "Custom" chord from played notes, we should loop within the number of notes played.
        if (chord.getName() == "Custom")
        {
            juce::SortedSet<int> playedNotes = chord.getSortedSet();
            int numPlayedNotes = playedNotes.size();

            if (numPlayedNotes > 0)
            {
                // Use modulo to wrap the degree index around the number of notes being held.
                return playedNotes[degreeIndex % numPlayedNotes];
            }
        }

        int semitone = chord.getDegree(degreeIndex);

        if (semitone != -1)
            return semitone;

        // If the degree is absent, handle it based on the mode
        if (playNoteOff == "Off")
        {
            return -1;
        }
        else if (playNoteOff == "Next")
        {
            for (int i = 1; i < 7; ++i)
            {
                semitone = chord.getDegree((degreeIndex + i) % 7);
                if (semitone != -1) return semitone;
            }
        }
        else if (playNoteOff == "Previous")
        {
            for (int i = 1; i < 7; ++i)
            {
                semitone = chord.getDegree((degreeIndex + 7 - i) % 7);
                if (semitone != -1) return semitone;
            }
        }
        
        // Fallback: return the root note if no other note is found
        return chord.getDegree(0);
    }

    /**
        Selects a random degree index that is actually present in the chord.
    */
    int getRandomPresentDegree()
    {
        juce::Array<int> presentDegrees;
        for (int i = 0; i < 7; ++i)
        {
            if (chord.getDegree(i) != -1)
                presentDegrees.add(i);
        }

        if (presentDegrees.isEmpty())
            return -1;

        return presentDegrees[juce::Random::getSystemRandom().nextInt(presentDegrees.size())];
    }

    MidiTools::Chord chord;
    juce::String pattern;
    int baseOctave = 4;
    int octave = baseOctave;
    juce::String playNoteOff = "Next"; // "Off", "Next", "Previous"

    int pos = 0;
    int lastPlayedMidiNote = -1;
    int lastPlayedDegreeIndex = 0;
private:
    double getNoteDivisor() const
    {
        switch (subdivision)
        {
            case 0: return 1.0;  // 1/4
            case 1: return 1.5;  // 1/4T
            case 2: return 2.0;  // 1/8
            case 3: return 3.0;  // 1/8T
            case 4: return 4.0;  // 1/16
            case 5: return 6.0;  // 1/16T
            case 6: return 8.0;  // 1/32
            case 7: return 12.0; // 1/32T
            default: return 4.0;
        }
    }

    void updateSamplesPerNote()
    {
        if (sampleRate > 0 && tempoBPM > 0)
        {
            const double noteDivisor = getNoteDivisor();
            double quarterNoteDurationSeconds = 60.0 / tempoBPM;
            samplesPerNote = sampleRate * quarterNoteDurationSeconds / noteDivisor;
        }
    }
    double sampleRate = 0.0;
    double tempoBPM = 120.0;
    int subdivision = 4; // Default to 1/16
    double samplesPerNote = 0.0;
    double samplesUntilNextNote = 0.0;
};
