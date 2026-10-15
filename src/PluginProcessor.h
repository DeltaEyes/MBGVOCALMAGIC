#pragma once
#include <JuceHeader.h>

class AudioPluginAudioProcessor : public juce::AudioProcessor
{
public:
    AudioPluginAudioProcessor();
    ~AudioPluginAudioProcessor() override;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
#ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported(const BusesLayout &layouts) const override;
#endif
    void processBlock(juce::AudioBuffer<float> &, juce::MidiBuffer &) override;
    juce::AudioProcessorEditor *createEditor() override;
    bool hasEditor() const override;
    const juce::String getName() const override;
    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram(int index) override;
    const juce::String getProgramName(int index) override;
    void changeProgramName(int index, const juce::String &newName) override;
    void getStateInformation(juce::MemoryBlock &destData) override;
    void setStateInformation(const void *data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState apvts;

    std::atomic<float> currentDetectedFreq{0.0f};
    std::atomic<float> currentTargetFreq{0.0f};
    std::atomic<float> currentRMS{0.0f};

private:
    juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    juce::dsp::Compressor<float> compressor;
    juce::dsp::StateVariableTPTFilter<float> midAirFilterL, midAirFilterR;
    juce::dsp::StateVariableTPTFilter<float> hiAirFilterL, hiAirFilterR;
    juce::dsp::StateVariableTPTFilter<float> pitchLpf;

    juce::AudioBuffer<float> delayBuffer;
    int writePosition = 0;
    float readPhase = 0.0f;
    float smoothedRatio = 1.0f;

    float pitchHistoryBuffer[2048] = {0.0f};
    int pitchHistoryPos = 0;
    float lastDetectedHz = 0.0f;

    float getTargetFrequency(float detectedFreq, int rootKey, int scaleType);
    float midiNoteToFreq(float midiNote) { return 440.0f * std::pow(2.0f, (midiNote - 69.0f) / 12.0f); }
    float freqToMidiNote(float freq) { return 69.0f + 12.0f * std::log2(freq / 440.0f); }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AudioPluginAudioProcessor)
};