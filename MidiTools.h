/*
  ==============================================================================

    MidiTools.h
    Created: 8 Nov 2025 3:02:03pm
    Author:  Olivier Doaré

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include <map>

namespace MidiTools
{
    /**
        Returns a map of note names (C, C#, Db, etc.) to their semitone offset from C.
        This is defined as a static function to ensure it's initialized only once.
    */
    static const std::map<juce::String, int>& getNoteNameOffsetMap()
    {
        static const std::map<juce::String, int> noteOffsets = {
            {"c", 0}, {"b#", 0},
            {"c#", 1}, {"db", 1},
            {"d", 2},
            {"d#", 3}, {"eb", 3},
            {"e", 4}, {"fb", 4},
            {"f", 5}, {"e#", 5},
            {"f#", 6}, {"gb", 6},
            {"g", 7},
            {"g#", 8}, {"ab", 8},
            {"a", 9},
            {"a#", 10}, {"bb", 10},
            {"b", 11}, {"cb", 11}
        };
        return noteOffsets;
    }

    /**
        Represents a musical chord, with properties like its name and the semitones it contains.
    */
    class Chord
    {
    public:
        /**
            Constructs a Chord object from a chord name string.
            The constructor parses the name to determine the root note and chord quality,
            then populates the set of semitones that define the chord.
            @param chordName The name of the chord, e.g., "C", "Am", "G7", "F#M7".
        */
        Chord(const juce::String& chordName) : name(chordName)
        {
            // Initialize all 7 degrees to -1 (absent)
            // [0] = fundamental, [1] = 3rd, [2] = 5th, [3] = 7th, [4] = 9th, [5] = 11th, [6] = 13th
            degrees.insertMultiple(0, -1, 7);

            juce::String input = name.trim();
            if (input.isEmpty())
                return;

            juce::String rootNoteStr;
            auto& noteMap = getNoteNameOffsetMap();
            int root = -1;

            // --- 1. Parse the chord string to find the root and quality ---
            if (input.endsWith("M7"))
            {
                rootNoteStr = input.dropLastCharacters(2).toLowerCase();
                if (noteMap.find(rootNoteStr) == noteMap.end()) return;
                root = noteMap.at(rootNoteStr);
                degrees.set(0, root);                  // Root
                degrees.set(1, (root + 4) % 12);       // Major Third
                degrees.set(2, (root + 7) % 12);       // Perfect Fifth
                degrees.set(3, (root + 11) % 12);      // Major Seventh
            }
            else if (input.endsWith("m7"))
            {
                rootNoteStr = input.dropLastCharacters(2).toLowerCase();
                if (noteMap.find(rootNoteStr) == noteMap.end()) return;
                root = noteMap.at(rootNoteStr);
                degrees.set(0, root);                  // Root
                degrees.set(1, (root + 3) % 12);       // Minor Third
                degrees.set(2, (root + 7) % 12);       // Perfect Fifth
                degrees.set(3, (root + 10) % 12);      // Minor Seventh
            }
            else if (input.endsWith("7"))
            {
                rootNoteStr = input.dropLastCharacters(1).toLowerCase();
                if (noteMap.find(rootNoteStr) == noteMap.end()) return;
                root = noteMap.at(rootNoteStr);
                degrees.set(0, root);                  // Root
                degrees.set(1, (root + 4) % 12);       // Major Third
                degrees.set(2, (root + 7) % 12);       // Perfect Fifth
                degrees.set(3, (root + 10) % 12);      // Minor Seventh
            }
            else if (input.endsWith("5"))
            {
                rootNoteStr = input.dropLastCharacters(1).toLowerCase();
                if (noteMap.find(rootNoteStr) == noteMap.end()) return;
                root = noteMap.at(rootNoteStr);
                degrees.set(0, root);                  // Root
                degrees.set(2, (root + 7) % 12);       // Perfect Fifth
            }
            else if (input.endsWith("m"))
            {
                rootNoteStr = input.dropLastCharacters(1).toLowerCase();
                if (noteMap.find(rootNoteStr) == noteMap.end()) return;
                root = noteMap.at(rootNoteStr);
                degrees.set(0, root);                  // Root
                degrees.set(1, (root + 3) % 12);       // Minor Third
                degrees.set(2, (root + 7) % 12);       // Perfect Fifth
            }
            else if (input.endsWith("M"))
            {
                rootNoteStr = input.dropLastCharacters(1).toLowerCase();
                if (noteMap.find(rootNoteStr) == noteMap.end()) return;
                root = noteMap.at(rootNoteStr);
                degrees.set(0, root);                  // Root
                degrees.set(1, (root + 4) % 12);       // Major Third
                degrees.set(2, (root + 7) % 12);       // Perfect Fifth
            }
            else // Assume single note
            {
                rootNoteStr = input.toLowerCase();
                if (noteMap.find(rootNoteStr) == noteMap.end()) return;
                root = noteMap.at(rootNoteStr);
                degrees.set(0, root); // Only the root note
            }
        }

        /** Returns an ordered array of 7 semitones representing the chord's degrees.
            The order is: fundamental, 3rd, 5th, 7th, 9th, 11th, 13th.
            An absent degree is represented by -1.
        */
        const juce::Array<int>& getDegrees() const { return degrees; }

        /** Returns the original name of the chord. */
        const juce::String& getName() const { return name; }

        /** Gets the semitone value of a specific degree of the chord.
            @param degreeIndex The index of the degree (0=fundamental, 1=third, etc.).
            @return The semitone value (0-11), or -1 if the degree is absent or the index is invalid.
        */
        int getDegree(int degreeIndex) const
        {
            if (juce::isPositiveAndBelow(degreeIndex, degrees.size()))
                return degrees[degreeIndex];
            return -1;
        }

        /** Returns a SortedSet of the present semitones (0-11) in the chord.
            This is useful for checking against a collection of played MIDI notes
            where order and octave do not matter.
        */
        juce::SortedSet<int> getSortedSet() const
        {
            juce::SortedSet<int> presentSemitones;
            for (int degree : degrees)
            {
                if (degree != -1)
                    presentSemitones.add(degree);
            }
            return presentSemitones;
        }

    private:
        juce::String name;
        juce::Array<int> degrees; // Stores 7 degrees: 1, 3, 5, 7, 9, 11, 13. -1 means absent.
    };

    /**
        Returns a map of French note names (Do, Ré b, etc.) to their semitone offset from C.
    */
    static const std::map<juce::String, int>& getFrenchNoteNameOffsetMap()
    {
        static const std::map<juce::String, int> noteOffsets = {
            {"do", 0},
            {"do#", 1}, {"reb", 1},
            {"re", 2}, {"ré", 2},
            {"re#", 3}, {"ré#", 3}, {"mib", 3},
            {"mi", 4},
            {"fa", 5},
            {"fa#", 6}, {"solb", 6},
            {"sol", 7},
            {"sol#", 8}, {"lab", 8},
            {"la", 9},
            {"la#", 10}, {"sib", 10},
            {"si", 11}
        };
        return noteOffsets;
    }


    /**
        Converts a MIDI note number into its string representation (e.g., 60 -> "C4").
        @param noteNumber The MIDI note number (0-127).
        @return A string representing the note name and octave.
    */
    static juce::String getNoteName(int noteNumber)
    {
        if (noteNumber < 0 || noteNumber > 127)
            return "Invalid";

        static const juce::String noteNames[] = { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };

        const int octave = (noteNumber / 12) - 1;
        const juce::String& note = noteNames[noteNumber % 12];

        return note + juce::String(octave);
    }

    /**
        Converts a note name string (e.g., "C#4") into a MIDI note number.
        Handles sharps (#), flats (b), and octave numbers. Case-insensitive.
        @param noteNameWithOctave The note string, e.g., "C4", "Db-1", "f#5".
        @return The MIDI note number (0-127), or -1 if the string is invalid.
    */
    static int getNoteNumber(const juce::String& noteNameWithOctave)
    {
        juce::String input = noteNameWithOctave.trim().toLowerCase();
        if (input.isEmpty())
            return -1;

        juce::String notePart;
        int octavePartStartIndex = 1;

        if (input.length() > 1 && (input[1] == '#' || input[1] == 'b'))
        {
            notePart = input.substring(0, 2);
            octavePartStartIndex = 2;
        }
        else
        {
            notePart = input.substring(0, 1);
        }

        auto& noteMap = getNoteNameOffsetMap();
        if (noteMap.find(notePart) == noteMap.end())
            return -1; // Note name not found

        int noteOffset = noteMap.at(notePart);

        juce::String octaveString = input.substring(octavePartStartIndex);
        if (octaveString.isEmpty() || !octaveString.containsOnly("-0123456789"))
            return -1; // Invalid or missing octave

        int octave = octaveString.getIntValue();

        int midiNote = (octave + 1) * 12 + noteOffset;

        if (midiNote >= 0 && midiNote <= 127)
            return midiNote;

        return -1; // Result is out of MIDI range
    }

    /**
        Checks if a MIDI note number corresponds to a note name, ignoring the octave.
        @param noteNumber The MIDI note number to check.
        @param noteName   The note name to compare against (e.g., "C", "Db", "F#"). Case-insensitive.
        @return True if the note number's pitch class matches the note name, false otherwise.
    */
    static bool isNoteEqual(int noteNumber, const juce::String& noteName)
    {
        if (noteNumber < 0 || noteNumber > 127)
            return false;

        juce::String cleanedNoteName = noteName.trim().toLowerCase();
        if (cleanedNoteName.isEmpty())
            return false;

        const int noteNumberSemitone = noteNumber % 12;

        auto& noteMap = getNoteNameOffsetMap();
        if (noteMap.find(cleanedNoteName) == noteMap.end())
            return false; // The provided note name is not valid

        return noteNumberSemitone == noteMap.at(cleanedNoteName);
    }

    /**
        Parses a chord name and returns the MIDI note number of its root, ignoring octave.
        @param chordName The chord name, e.g., "C", "Am", "G7", "F#5".
        @return The semitone of the root note (0-11), or 0 (C) if parsing fails.
    */
    static int getRootNoteFromChord(const juce::String& chordName)
    {
        juce::String input = chordName.trim();
        if (input.isEmpty())
            return 0;

        juce::String rootNoteStr;
        auto& noteMap = getNoteNameOffsetMap();

        // Check for multi-character suffixes first
        if (input.endsWith("M7") || input.endsWith("m7"))
        {
            rootNoteStr = input.dropLastCharacters(2).toLowerCase();
        }
        else if (input.endsWith("7") || input.endsWith("5") || input.endsWith("m") || input.endsWith("M"))
        {
            rootNoteStr = input.dropLastCharacters(1).toLowerCase();
        }
        else // Assume single note
        {
            rootNoteStr = input.toLowerCase();
        }

        // Handle sharp/flat in root note
        if (rootNoteStr.length() > 1 && (rootNoteStr.endsWith("#") || rootNoteStr.endsWith("b")))
        {
            // Already have the full root note string
        }
        else if (rootNoteStr.length() > 1) // e.g. "solb"
        {
             // keep as is for french notation
        }
        else {
            // single character root note
        }

        if (noteMap.count(rootNoteStr))
            return noteMap.at(rootNoteStr);

        return 0; // Default to C if parsing fails
    }

    /**
        Checks if a collection of MIDI notes forms a specific major or minor chord,
        regardless of octave or inversion.
        @param heldNotes          A collection of MIDI note numbers currently being played.
        @param chordName          The chord to check for, e.g., "CM", "F#m", "Ebm".
                                  Case-insensitive. 'M' or no suffix for major, 'm' for minor.
        @return True if the notes form the specified chord, false otherwise.
    */
    template <typename Collection>
    static bool isChordEqual(const Collection& heldNotes, const juce::String& chordName)
    {
        if (chordName.trim().isEmpty() || heldNotes.isEmpty())
            return false;
    
        // 1. Create a Chord object to get the target semitones.
        Chord targetChord(chordName);
        juce::SortedSet<int> targetSemitones = targetChord.getSortedSet();
    
        // If the chord name was invalid, the set of target notes will be empty.
        if (targetSemitones.isEmpty())
            return false;
    
        // 2. Build a set of the currently played semitones.
        juce::SortedSet<int> playedSemitones;
        for (const auto& noteNumber : heldNotes)
        {
            playedSemitones.add(noteNumber % 12);
        }
    
        // --- 3. Compare the sets ---
        return playedSemitones == targetSemitones;
    }

    /**
        Returns a random major or minor chord name string.
        Uses the same nomenclature as isChordEqual (e.g., "C#M", "Am").
        @return A string representing a random chord.
    */
    static juce::String getRandomChordName()
    {
        static const juce::String rootNotes[] = { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };
        
        juce::Random& random = juce::Random::getSystemRandom();
        
        // 1. Pick a random root note
        const juce::String& root = rootNotes[random.nextInt(12)];
        
        // 2. Pick a random quality (major or minor)
        const bool isMinor = random.nextBool();
        
        return root + (isMinor ? "m" : "M");
    }

    /**
        Returns a random single note name string (e.g., "C", "F#", "Bb").
        Note that "Bb" will be represented as "A#".
        @return A string representing a random note name.
    */
    static juce::String getRandomSingleNoteName()
    {
        static const juce::String noteNames[] = { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };

        juce::Random& random = juce::Random::getSystemRandom();

        return noteNames[random.nextInt(12)];
    }

    /**
        Returns a random fifth interval name string (e.g., "C5", "F#5").
        @return A string representing a random fifth interval.
    */
    static juce::String getRandomFifthInterval()
    {
        static const juce::String noteNames[] = { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };

        juce::Random& random = juce::Random::getSystemRandom();

        const juce::String& root = noteNames[random.nextInt(12)];
        return root + "5";
    }

    /**
        Returns a random 7th chord name string (e.g., "CM7", "Am7", "G7").
        @return A string representing a random 7th chord.
    */
    static juce::String getRandomSeventhChord()
    {
        static const juce::String rootNotes[] = { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };
        static const juce::String chordTypes[] = { "M7", "m7", "7" };

        juce::Random& random = juce::Random::getSystemRandom();

        // 1. Pick a random root note
        const juce::String& root = rootNotes[random.nextInt(12)];

        // 2. Pick a random 7th quality
        const juce::String& type = chordTypes[random.nextInt(3)];

        return root + type;
    }

    /**
        Converts a standard international note name (e.g., "C#") to its French equivalent (e.g., "Do#").
        @param standardNoteName The standard note name (C, C#, Db, etc.).
        @return The corresponding French note name as a string. Returns an empty string if not found.
    */
    static juce::String getFrenchNoteName(const juce::String& standardNoteName)
    {
        static const juce::String frenchNoteNames[] = { "Do", "Do#", "Re", "Re#", "Mi", "Fa", "Fa#", "Sol", "Sol#", "La", "La#", "Si" };

        auto& noteMap = getNoteNameOffsetMap();
        auto cleanedName = standardNoteName.trim().toLowerCase();

        if (noteMap.count(cleanedName))
        {
            int semitone = noteMap.at(cleanedName);
            return frenchNoteNames[semitone];
        }
        return {}; // Return empty string if not found
    }

    /**
        Converts a standard international chord name (e.g., "C", "Am", "G5") to its French equivalent.
        @param standardChordName The standard chord name.
        @return The corresponding French chord name as a string. Returns the original name if it can't be parsed.
    */
    static juce::String getFrenchChordName(const juce::String& standardChordName)
    {
        juce::String input = standardChordName.trim();
        if (input.isEmpty())
            return {};

        juce::String rootNoteStr;
        juce::String suffix;

        // Check for longer suffixes first
        if (input.endsWith("M7"))
        {
            rootNoteStr = input.dropLastCharacters(2);
            suffix = "M7"; // e.g., "DoM7"
        }
        else if (input.endsWith("m7"))
        {
            rootNoteStr = input.dropLastCharacters(2);
            suffix = "m7"; // e.g., "Lam7"
        }
        else if (input.endsWith("7"))
        {
            rootNoteStr = input.dropLastCharacters(1);
            suffix = "7"; // e.g., "Sol7"
        }
        else if (input.endsWith("5"))
        {
            rootNoteStr = input.dropLastCharacters(1);
            suffix = "5"; // e.g., "Sol5"
        }
        else if (input.endsWith("m"))
        {
            rootNoteStr = input.dropLastCharacters(1);
            suffix = "m";
        }
        else if (input.endsWith("M"))
        {
            rootNoteStr = input.dropLastCharacters(1);
            suffix = "M";
        }
        else // Handles single notes ("C")
        {
            rootNoteStr = input;
            suffix = ""; // This case now only handles single notes.
        }

        juce::String frenchRoot = getFrenchNoteName(rootNoteStr);
        return frenchRoot.isNotEmpty() ? frenchRoot + suffix : standardChordName;
    }
}
