#pragma once

#include <JuceHeader.h>

class AdditiveSynthesiserSound : public juce::SynthesiserSound
{
public:
    AdditiveSynthesiserSound() {}

    bool appliesToNote (int midiNoteNumber) override { return true; }
    bool appliesToChannel (int midiChannel) override { return true; }
};

