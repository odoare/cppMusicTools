# cppMusicTools

A collection of C++ tools for MIDI and music development, built with the JUCE framework.

## Arpeggiator

The `Arpeggiator` class is a base for creating MIDI arpeggiators. It takes a `Chord`, an octave, and a pattern string to generate a sequence of MIDI notes.

### How It Works

The `processBlock()` method should be called from your audio processing loop. It generates MIDI note-on and note-off events based on a pattern string and the current tempo.

### Pattern String Syntax

The pattern string consists of characters that define the arpeggio's behavior at each step:

- **`0` to `6`**: Plays a specific degree of the chord (0=fundamental, 1=third, ..., 6=thirteenth).
  - The behavior for absent degrees can be configured ("Off", "Next", "Previous").
- **`_`**: Sustains the previously played note.
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

### Basic Usage

```cpp
#include "Arpeggiator.h"

void setup()
{
    // Define a C Major chord (Root at C4 = MIDI note 60)
    MidiTools::Chord cMajor({60, 64, 67}); // C, E, G

    // Create an arpeggiator with a pattern
    Arpeggiator arp(cMajor, "0121", 4); // Arpeggiate root, third, fifth, third

    // Prepare for playback
    arp.prepareToPlay(44100.0); // Set sample rate
    arp.setTempo(120.0);
}
```
