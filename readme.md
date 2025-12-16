# cppMusicTools

A collection of C++ tools for MIDI and music development, built with the JUCE framework.

## MidiTools

A namespace containing a Chord class and a suite of utility functions for handling MIDI notes and chords.

### The Chord Class

The Chord class represents a musical chord. It can be constructed from a string like "Am7" or "F#M" and provides methods to access its constituent notes (degrees).

## Arpeggiator

The `Arpeggiator` class is a base for creating MIDI arpeggiators. It takes a `Chord`, an octave, and a pattern string to generate a sequence of MIDI notes.

### How It Works

The `processBlock()` method should be called from your audio processing loop. It generates MIDI note-on and note-off events based on a pattern string and the current tempo.

### Pattern String Syntax

The pattern string consists of characters that define the arpeggio's behavior at each step:

- **`0` to `6`**: Plays a specific degree of the chord (0=fundamental, 1=third, ..., 6=thirteenth).
- **`_`**: Sustains the previously played note.
- **`1` to `9`**: Plays a specific degree of the chord (1=fundamental, 2=second, ..., 7=seventh).
- **`.`**: A rest; no note is played.
- **`+`**: Plays the next degree in the chord (loops from 6 to 0).
- **`-`**: Plays the previous degree in the chord (loops from 0 to 6).
- **`#`**: Plays the degree two steps higher in the chord (e.g., from 1 to 3).
- **`=`**: Plays the degree two steps lower in the chord (e.g., from 3 to 1).
- **`*`**: Plays a random, valid note from the current chord.
- **`"`**: Repeats the last played degree.

#### Octave Modifiers

Octave modifiers are prefixed to a note command to change the octave for that step.

- **`oN`**: Sets the octave to `N` (where `N` is a digit from 0-7). Example: `"o30"` sets the octave to 3 and plays the root.
- **`o+`**: Increases the octave by one. Example: `"o+0"` plays the root one octave higher.
- **`o-`**: Decreases the octave by one. Example: `"o-0"` plays the root one octave lower.
