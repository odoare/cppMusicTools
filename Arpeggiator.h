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
            //std::cout << "time = " << time << std::endl;

            if (samplesUntilNextNote <= 0.0)
            {
                // std::cout << "Should call getNext()" << std::endl;
                // Time to generate the next note from the pattern.
                // The events are timestamped relative to the start of this block.
                // The `addEvents` signature is:
                // addEvents (sourceBuffer, startSampleInSource, numSamplesInSource, timeOffsetToAdd)
                // Our getNext() buffer has events at time 0, so we start copying from sample 0
                // and add the current `time` as an offset.
                generatedMidi.addEvents(getNext(), 0, -1, time);
                samplesUntilNextNote += samplesPerNote;
            }

            int samplesThisStep = juce::jmin(numSamples - time, (int)std::ceil(samplesUntilNextNote));
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
                    case '+': currentDegreeIndex = (currentDegreeIndex + 1) % 7; break;
                    case '-': currentDegreeIndex = (currentDegreeIndex + 6) % 7; break;
                    case '#': currentDegreeIndex = (currentDegreeIndex + 2) % 7; break;
                    case '=': currentDegreeIndex = (currentDegreeIndex + 5) % 7; break;
                    case '*': currentDegreeIndex = getRandomPresentDegree(); break;
                    case '"': /* currentDegreeIndex remains the same as lastPlayedDegreeIndex */ break;
                    case '.': currentDegreeIndex = -1; /* Rest */ break;
                    default:  // Invalid characters are ignored.
                        noteCommandFound = false; // Ensure we continue the loop
                        continue; // Skip to the next character in the pattern.
                }
                noteCommandFound = true;
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
    /**
        Synchronizes the arpeggiator's internal clock to the host's transport position.
        This should be called on every process block while the host is playing.
        @param positionInfo The host's current position information.
    */
    void syncToPlayHead(const juce::AudioPlayHead::CurrentPositionInfo& positionInfo)
    {
        if (samplesPerNote <= 0.0 || positionInfo.ppqPosition < 0.0)
            return;

        // Calculate the host's position in terms of 16th notes.
        // A quarter note (ppq) contains `noteDivisor` number of notes.
        const double notesPerQuarter = getNoteDivisor();
        const double positionInNotes = positionInfo.ppqPosition * notesPerQuarter;

        // Find the position of the *next* 16th note.
        const double nextNotePosition = std::ceil(positionInNotes);

        // Calculate the distance (in 16th notes) to the next grid line.
        const double notesUntilNext = nextNotePosition - positionInNotes;

        // Convert that distance into samples.
        // samplesPerNote is the duration of one 16th note in samples.
        samplesUntilNextNote = notesUntilNext * samplesPerNote;
    }

    /** Resets the arpeggiator's position to the beginning of the pattern. */
    juce::MidiBuffer reset()
    {
        juce::MidiBuffer noteOffBuffer;
        if (lastPlayedMidiNote != -1)
        {
            noteOffBuffer.addEvent(juce::MidiMessage::noteOff(1, lastPlayedMidiNote), 0);
        }

        octave = baseOctave;

        pos = 0;
        lastPlayedMidiNote = -1;
        lastPlayedDegreeIndex = 0;
        // samplesUntilNextNote = 0; // This should not be reset here.
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
