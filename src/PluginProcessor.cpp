#include "PluginProcessor.h"
#include "PluginEditor.h"

AudioPluginAudioProcessor::AudioPluginAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
    : AudioProcessor(BusesProperties().withInput("Input", juce::AudioChannelSet::stereo(), true).withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      apvts(*this, nullptr, "Parameters", createParameterLayout())
#endif
{
}

AudioPluginAudioProcessor::~AudioPluginAudioProcessor() {}

juce::AudioProcessorValueTreeState::ParameterLayout AudioPluginAudioProcessor::createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    layout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID("SPEED", 1), "Retune Speed", 0.0f, 100.0f, 50.0f));

    juce::StringArray keys = {"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"};
    layout.add(std::make_unique<juce::AudioParameterChoice>(juce::ParameterID("KEY", 1), "Key", keys, 0));

    juce::StringArray scales = {"Chromatic", "Major", "Natural Minor", "Pentatonic"};
    layout.add(std::make_unique<juce::AudioParameterChoice>(juce::ParameterID("SCALE", 1), "Scale", scales, 1));

    layout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID("MIDAIR", 1), "Mid Air", 0.0f, 10.0f, 0.0f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID("HIAIR", 1), "High Air", 0.0f, 10.0f, 0.0f));

    layout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID("THRESH", 1), "Threshold", -60.0f, 0.0f, -20.0f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID("MAKEUP", 1), "Makeup", -12.0f, 24.0f, 0.0f));

    return layout;
}

void AudioPluginAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    juce::dsp::ProcessSpec spec{sampleRate, (juce::uint32)samplesPerBlock, (juce::uint32)getTotalNumInputChannels()};

    compressor.prepare(spec);
    compressor.setRatio(4.0f);
    compressor.setAttack(5.0f);
    compressor.setRelease(100.0f);

    spec.numChannels = 1;
    midAirFilterL.prepare(spec);
    midAirFilterL.setType(juce::dsp::StateVariableTPTFilterType::highpass);
    midAirFilterL.setCutoffFrequency(3000.0f);
    midAirFilterR.prepare(spec);
    midAirFilterR.setType(juce::dsp::StateVariableTPTFilterType::highpass);
    midAirFilterR.setCutoffFrequency(3000.0f);
    hiAirFilterL.prepare(spec);
    hiAirFilterL.setType(juce::dsp::StateVariableTPTFilterType::highpass);
    hiAirFilterL.setCutoffFrequency(8000.0f);
    hiAirFilterR.prepare(spec);
    hiAirFilterR.setType(juce::dsp::StateVariableTPTFilterType::highpass);
    hiAirFilterR.setCutoffFrequency(8000.0f);

    pitchLpf.prepare(spec);
    pitchLpf.setType(juce::dsp::StateVariableTPTFilterType::lowpass);
    pitchLpf.setCutoffFrequency(600.0f);

    // 軽量ディレイバッファの初期化（最大0.1秒）
    delayBuffer.setSize(2, (int)(sampleRate * 0.1) + 1);
    delayBuffer.clear();
    writePosition = 0;
    readPhase = 0.0f;
}

float AudioPluginAudioProcessor::getTargetFrequency(float detectedFreq, int rootKey, int scaleType)
{
    if (detectedFreq < 60.0f || detectedFreq > 1000.0f)
        return detectedFreq;

    float midiNote = freqToMidiNote(detectedFreq);
    int noteInt = (int)std::round(midiNote);
    int interval = (noteInt - rootKey + 24) % 12;
    int adjustedInterval = interval;

    if (scaleType == 1)
    { // Major
        int majorMap[] = {0, 0, 2, 2, 4, 5, 5, 7, 7, 9, 9, 11};
        adjustedInterval = majorMap[interval];
    }
    else if (scaleType == 2)
    { // Natural Minor
        int minorMap[] = {0, 0, 2, 3, 3, 5, 5, 7, 8, 8, 10, 10};
        adjustedInterval = minorMap[interval];
    }
    else if (scaleType == 3)
    { // Pentatonic
        int pentaMap[] = {0, 0, 2, 2, 4, 4, 4, 7, 7, 9, 9, 9};
        adjustedInterval = pentaMap[interval];
    }

    int targetNoteInt = noteInt - interval + adjustedInterval;
    return midiNoteToFreq((float)targetNoteInt);
}

void AudioPluginAudioProcessor::processBlock(juce::AudioBuffer<float> &buffer, juce::MidiBuffer &)
{
    juce::ScopedNoDenormals noDenormals;
    int numSamples = buffer.getNumSamples();
    float sampleRate = (float)getSampleRate();
    auto *readPtr = buffer.getReadPointer(0);

    float rms = 0.0f;
    for (int i = 0; i < numSamples; ++i)
    {
        float filtered = pitchLpf.processSample(0, readPtr[i]);
        pitchHistoryBuffer[pitchHistoryPos] = filtered;
        pitchHistoryPos = (pitchHistoryPos + 1) % 1024; // バッファサイズを半分にして軽量化
        rms += readPtr[i] * readPtr[i];
    }
    rms = std::sqrt(rms / numSamples);
    currentRMS.store(rms);

    // 超軽量なゼロクロス間隔測定（CPU負荷ほぼゼロで音程の目安を検出）
    float detectedHz = 220.0f;
    if (rms > 0.02f)
    {
        int zeroCrossings = 0;
        for (int i = 1; i < numSamples; ++i)
        {
            if (readPtr[i] >= 0.0f && readPtr[i - 1] < 0.0f)
                zeroCrossings++;
        }
        if (zeroCrossings > 0)
        {
            detectedHz = (zeroCrossings * sampleRate) / (float)numSamples;
            if (detectedHz < 60.0f)
                detectedHz *= 2.0f; // オクターブ下補正
        }
    }

    float speedParam = apvts.getRawParameterValue("SPEED")->load() / 100.0f;
    int rootKey = (int)apvts.getRawParameterValue("KEY")->load();
    int scaleType = (int)apvts.getRawParameterValue("SCALE")->load();

    float targetHz = detectedHz;
    if (rms > 0.02f)
    {
        currentDetectedFreq.store(detectedHz);
        targetHz = getTargetFrequency(detectedHz, rootKey, scaleType);
        currentTargetFreq.store(targetHz);
    }
    else
    {
        currentTargetFreq.store(0.0f);
    }

    // ピッチシフト比率の計算とスムージング
    float targetRatio = (targetHz > 50.0f && detectedHz > 50.0f) ? (targetHz / detectedHz) : 1.0f;
    smoothedRatio = smoothedRatio + (targetRatio - smoothedRatio) * (0.05f + speedParam * 0.45f);

    // ==============================================================
    // 高速グラニュラー・ピッチシフト（CPU超軽量版）
    // ==============================================================
    int delayLength = delayBuffer.getNumSamples();
    float windowSize = sampleRate * 0.03f; // 30msのウインドウ（超高速・低遅延）

    for (int i = 0; i < numSamples; ++i)
    {
        for (int ch = 0; ch < getTotalNumInputChannels(); ++ch)
        {
            delayBuffer.setSample(ch, writePosition, buffer.getSample(ch, i));
        }

        readPhase += (1.0f - smoothedRatio);
        while (readPhase >= windowSize)
            readPhase -= windowSize;
        while (readPhase < 0.0f)
            readPhase += windowSize;

        float phaseNorm1 = readPhase / windowSize;
        float phaseNorm2 = phaseNorm1 + 0.5f;
        if (phaseNorm2 >= 1.0f)
            phaseNorm2 -= 1.0f;

        float readPos1 = (float)writePosition - phaseNorm1 * windowSize;
        if (readPos1 < 0)
            readPos1 += delayLength;
        float readPos2 = (float)writePosition - phaseNorm2 * windowSize;
        if (readPos2 < 0)
            readPos2 += delayLength;

        float fade1 = 0.5f * (1.0f - std::cos(phaseNorm1 * juce::MathConstants<float>::twoPi));
        float fade2 = 0.5f * (1.0f - std::cos(phaseNorm2 * juce::MathConstants<float>::twoPi));

        for (int ch = 0; ch < getTotalNumInputChannels(); ++ch)
        {
            float s1 = delayBuffer.getSample(ch, (int)readPos1);
            float s2 = delayBuffer.getSample(ch, (int)readPos2);
            buffer.setSample(ch, i, (s1 * fade1) + (s2 * fade2));
        }

        writePosition++;
        if (writePosition >= delayLength)
            writePosition = 0;
    }

    // ==============================================================
    // コンプレッサー & Fresh Air 処理
    // ==============================================================
    float thresh = apvts.getRawParameterValue("THRESH")->load();
    compressor.setThreshold(thresh);
    juce::dsp::AudioBlock<float> block(buffer);
    juce::dsp::ProcessContextReplacing<float> context(block);
    compressor.process(context);

    float makeup = juce::Decibels::decibelsToGain(apvts.getRawParameterValue("MAKEUP")->load());
    float midAirGain = apvts.getRawParameterValue("MIDAIR")->load() * 0.4f;
    float hiAirGain = apvts.getRawParameterValue("HIAIR")->load() * 0.4f;

    for (int ch = 0; ch < getTotalNumInputChannels(); ++ch)
    {
        auto *data = buffer.getWritePointer(ch);
        for (int i = 0; i < numSamples; ++i)
        {
            float in = data[i];
            float mid = (ch == 0) ? midAirFilterL.processSample(0, in) : midAirFilterR.processSample(0, in);
            float high = (ch == 0) ? hiAirFilterL.processSample(0, in) : hiAirFilterR.processSample(0, in);
            float addedAir = (std::tanh(mid * 2.5f) * midAirGain) + (std::tanh(high * 2.5f) * hiAirGain);
            data[i] = (in + addedAir) * makeup;
        }
    }
}

const juce::String AudioPluginAudioProcessor::getName() const { return JucePlugin_Name; }
bool AudioPluginAudioProcessor::acceptsMidi() const { return false; }
bool AudioPluginAudioProcessor::producesMidi() const { return false; }
bool AudioPluginAudioProcessor::isMidiEffect() const { return false; }
double AudioPluginAudioProcessor::getTailLengthSeconds() const { return 0.0; }
int AudioPluginAudioProcessor::getNumPrograms() { return 1; }
int AudioPluginAudioProcessor::getCurrentProgram() { return 0; }
void AudioPluginAudioProcessor::setCurrentProgram(int index) {}
const juce::String AudioPluginAudioProcessor::getProgramName(int index) { return {}; }
void AudioPluginAudioProcessor::changeProgramName(int index, const juce::String &newName) {}
void AudioPluginAudioProcessor::releaseResources() {}
#ifndef JucePlugin_PreferredChannelConfigurations
bool AudioPluginAudioProcessor::isBusesLayoutSupported(const BusesLayout &layouts) const { return true; }
#endif
bool AudioPluginAudioProcessor::hasEditor() const { return true; }
juce::AudioProcessorEditor *AudioPluginAudioProcessor::createEditor() { return new AudioPluginAudioProcessorEditor(*this); }
void AudioPluginAudioProcessor::getStateInformation(juce::MemoryBlock &destData) {}
void AudioPluginAudioProcessor::setStateInformation(const void *data, int sizeInBytes) {}
juce::AudioProcessor *JUCE_CALLTYPE createPluginFilter() { return new AudioPluginAudioProcessor(); }