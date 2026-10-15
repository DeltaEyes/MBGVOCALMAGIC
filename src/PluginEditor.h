#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"

class AudioPluginAudioProcessorEditor : public juce::AudioProcessorEditor, public juce::Timer
{
public:
    AudioPluginAudioProcessorEditor(AudioPluginAudioProcessor &);
    ~AudioPluginAudioProcessorEditor() override;
    void paint(juce::Graphics &) override;
    void resized() override;
    void timerCallback() override;

private:
    AudioPluginAudioProcessor &audioProcessor;
    juce::Image bgImage;

    // UIパーツ
    juce::Slider speedSlider;
    juce::ComboBox keyComboBox;   // ★ キー選択用ドロップダウン
    juce::ComboBox scaleComboBox; // ★ スケール選択用ドロップダウン

    juce::Slider midAirSlider, hiAirSlider;
    juce::Slider threshSlider, makeupSlider;

    juce::Label speedLabel, keyLabel, scaleLabel, midAirLabel, hiAirLabel, threshLabel, makeupLabel;

    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> speedAttach;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> keyAttach;   // ★ 紐付け用
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> scaleAttach; // ★ 紐付け用
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> midAirAttach, hiAirAttach;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> threshAttach, makeupAttach;

    std::vector<float> pitchHistory;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AudioPluginAudioProcessorEditor)
};