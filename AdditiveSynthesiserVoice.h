#include "AdditiveSynthesiserSound.h"  // Includi il file AdditiveSynthesiserSound.h


class AdditiveSynthesiserVoice : public juce::SynthesiserVoice
{
public:
    AdditiveSynthesiserVoice() {}
    // Metodo di inizializzazione (aggiunto)
    void initialize(double sampleRate)
    {
        oscillator.setSampleRate(sampleRate);
    }
    bool canPlaySound(juce::SynthesiserSound* sound) override
    {
        return dynamic_cast<AdditiveSynthesiserSound*>(sound) != nullptr;
    }

    void startNote(int midiNoteNumber, float velocity, juce::SynthesiserSound* sound, int currentPitchWheelPosition) override
    {
        frequency = juce::MidiMessage::getMidiNoteInHertz(midiNoteNumber);
        oscillator.setFrequency(frequency);
        oscillator.activate();
        gain = velocity;

        DBG("Voice started: " << midiNoteNumber << ", frequency: " << frequency << ", velocity: " << velocity);
    }

    void stopNote(float velocity, bool allowTailOff) override
    {
        // Disattiva l'oscillatore quando la nota è terminata
        oscillator.deactivate();
    }

    void pitchWheelMoved(int newPitchWheelValue) override {}
    void controllerMoved(int controllerNumber, int newControllerValue) override {}

    void renderNextBlock(juce::AudioBuffer<float>& outputBuffer, int startSample, int numSamples) override
    {
        DBG("Rendering voice for " << numSamples << " samples");

        for (int sample = startSample; sample < startSample + numSamples; ++sample)
        {
            float generatedSample = oscillator.getNextSample() * gain;

            if (sample == startSample)  // Log solo il primo campione
                DBG("Generated sample: " << generatedSample);

            outputBuffer.addSample(0, sample, generatedSample);
            outputBuffer.addSample(1, sample, generatedSample);
        }
    }

private:
    AdditiveOscillator oscillator;  // Il tuo oscillatore additivo
    float frequency = 440.0f;  // Frequenza della nota
    float gain = 0.0f;  // Guadagno per la velocità
};

