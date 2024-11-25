#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <JuceHeader.h>

AdditiveSynthWithWaveshapingAudioProcessor::AdditiveSynthWithWaveshapingAudioProcessor()
    : AudioProcessor (BusesProperties()
                      .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      .withOutput ("Output", juce::AudioChannelSet::stereo(), true))

{
    mode = ProcessingMode::InputOnly; // Imposta la modalità di default
    waveshapingIntensity = 1.0f;      // Imposta l'intensità di default
    //equalizer.setSampleRate(getSampleRate());
}



AdditiveSynthWithWaveshapingAudioProcessor::~AdditiveSynthWithWaveshapingAudioProcessor() {}

void AdditiveSynthWithWaveshapingAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    synthesiser.setCurrentPlaybackSampleRate(sampleRate);
    
    synthesiser.clearVoices();
    synthesiser.addVoice(new AdditiveSynthesiserVoice());
    for (int i = 0; i < synthesiser.getNumVoices(); ++i)
        {
            if (auto* voice = dynamic_cast<AdditiveSynthesiserVoice*>(synthesiser.getVoice(i)))
            {
                // Inizializza eventuali parametri specifici delle voci
                voice->initialize(sampleRate);
            }
        }
    oscillator.setSampleRate(getSampleRate());
    oscillator.setFrequency(frequency);
    oscillator.setNumHarmonics(numHarmonics);
    oscillator.resetPhase();

    waveshaper.setIntensity(waveshapingIntensity);
    waveshaper.setDrive(drive);
    waveshaper.setSymmetry(symmetry);
    waveshaper.setAmount(amount);

    equalizer.setSampleRate(getSampleRate());
    equalizer.prepare();
}



void AdditiveSynthWithWaveshapingAudioProcessor::releaseResources() {}

void AdditiveSynthWithWaveshapingAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    auto numInputChannels = getTotalNumInputChannels();
    auto numOutputChannels = getTotalNumOutputChannels();
    auto numSamples = buffer.getNumSamples();

    // In modalità MIDI Only, prima svuotiamo il buffer per assicurarci che non ci siano vecchi dati
    if (mode == ProcessingMode::MidiOnly)
    {
        buffer.clear();  // Puliamo il buffer
    }

    // Gestione degli eventi MIDI per alimentare il sintetizzatore
    for (auto metadata : midiMessages)
    {
        auto message = metadata.getMessage();
        if (message.isNoteOn())
            noteOn(message.getChannel(), message.getNoteNumber(), message.getVelocity());
        else if (message.isNoteOff())
            noteOff(message.getChannel(), message.getNoteNumber());
    }

    // Aggiungere il suono generato dal sintetizzatore (solo nella modalità MIDI Only)
    if (mode == ProcessingMode::MidiOnly)
    {
        // Qui passiamo il buffer al sintetizzatore
        synthesiser.renderNextBlock(buffer, midiMessages, 0, buffer.getNumSamples());
    }
    else
    {
        for (int channel = 0; channel < numInputChannels; ++channel)
        {
            auto* channelData = buffer.getWritePointer(channel);

            for (int sample = 0; sample < numSamples; ++sample)
            {
                float inputSample = channelData[sample];
                float generatedSample = 0.0f;

                // Modalità generazione suono con l'oscillatore
                switch (mode)
                {
                    case ProcessingMode::GeneratedOnly:
                        oscillator.activate();
                        generatedSample = oscillator.getNextSample();
                        break;

                    case ProcessingMode::InputOnly:
                        oscillator.deactivate();
                        generatedSample = inputSample;
                        break;

                    case ProcessingMode::InputAndGenerated:
                        oscillator.activate();
                        generatedSample = inputSample + oscillator.getNextSample();
                        break;
                }

                // Waveshaping
                float finalSample = waveshaper.waveshapingState ? waveshaper.processSample(generatedSample) : generatedSample;
                finalSample *= outputGain;
                channelData[sample] = finalSample;  // Scriviamo il campione nel buffer
            }
        }
    }

    // Pulisci i canali extra (ad esempio, se hai più canali di output)
    for (int channel = numInputChannels; channel < numOutputChannels; ++channel)
    {
        buffer.clear(channel, 0, numSamples);
    }

    // Passa il buffer all'equalizzatore
    if (!dontCallProcess)
    {
        equalizer.process(buffer);
    }

    // Analizzatore di spettro
    for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
    {
        auto* channelData = buffer.getReadPointer(channel);
        for (int smp = 0; smp < buffer.getNumSamples(); ++smp)
        {
            spectrumAnalyzer.pushNextSampleIntoFifo(channelData[smp]);
        }
    }
}







void AdditiveSynthWithWaveshapingAudioProcessor::noteOn(int midiChannel, int midiNoteNumber, float velocity)
{
    isOscillatorActive = true;
    oscillator.setFrequency(juce::MidiMessage::getMidiNoteInHertz(midiNoteNumber));  // Aggiorna la frequenza
}

void AdditiveSynthWithWaveshapingAudioProcessor::noteOff(int midiChannel, int midiNoteNumber)
{
    isOscillatorActive = false;  // Disattiva l'oscillatore
}



void AdditiveSynthWithWaveshapingAudioProcessor::handleMidiMessage(const juce::MidiMessage& message)
{
    if (message.isNoteOn())
    {
        noteOn(message.getChannel(), message.getNoteNumber(), message.getVelocity());
    }
    else if (message.isNoteOff())
    {
        noteOff(message.getChannel(), message.getNoteNumber());
    }
}


juce::AudioProcessorEditor* AdditiveSynthWithWaveshapingAudioProcessor::createEditor()
{
    return new AdditiveSynthWithWaveshapingAudioProcessorEditor(*this);
}
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new AdditiveSynthWithWaveshapingAudioProcessor();
}



