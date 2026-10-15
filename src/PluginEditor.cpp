#include "PluginProcessor.h"
#include "PluginEditor.h"

class FLStudioLookAndFeel : public juce::LookAndFeel_V4
{
public:
    FLStudioLookAndFeel()
    {
        setColour(juce::Slider::textBoxTextColourId, juce::Colours::white);
        setColour(juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
        setColour(juce::Slider::textBoxBackgroundColourId, juce::Colour(0xcc101010));
        setColour(juce::Slider::textBoxHighlightColourId, juce::Colour(0xff4A525C));

        // ドロップダウン(ComboBox)のFL Studio風ダークデザイン設定
        setColour(juce::ComboBox::backgroundColourId, juce::Colour(0xff181B1F));
        setColour(juce::ComboBox::textColourId, juce::Colours::white);
        setColour(juce::ComboBox::outlineColourId, juce::Colour(0xff3D444A));
        setColour(juce::PopupMenu::backgroundColourId, juce::Colour(0xff181B1F));
        setColour(juce::PopupMenu::textColourId, juce::Colours::white);
        setColour(juce::PopupMenu::highlightedBackgroundColourId, juce::Colour(0xffFF6600).withAlpha(0.5f));
    }

    void drawRotarySlider(juce::Graphics &g, int x, int y, int width, int height, float sliderPos, float rotaryStartAngle, float rotaryEndAngle, juce::Slider &) override
    {
        auto radius = (float)juce::jmin(width / 2, height / 2) - 8.0f;
        auto centreX = (float)x + (float)width * 0.5f;
        auto centreY = (float)y + (float)height * 0.5f;
        auto angle = rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);

        juce::Path track;
        track.addCentredArc(centreX, centreY, radius, radius, 0.0f, rotaryStartAngle, rotaryEndAngle, true);
        g.setColour(juce::Colour(0xff14171A));
        g.strokePath(track, juce::PathStrokeType(5.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

        juce::Path valueArc;
        valueArc.addCentredArc(centreX, centreY, radius, radius, 0.0f, rotaryStartAngle, angle, true);
        g.setColour(juce::Colours::white);
        g.strokePath(valueArc, juce::PathStrokeType(5.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

        float innerRadius = radius - 7.0f;
        g.setColour(juce::Colour(0xff2F353C));
        g.fillEllipse(centreX - innerRadius, centreY - innerRadius, innerRadius * 2.0f, innerRadius * 2.0f);
        g.setColour(juce::Colour(0xff1A1D21));
        g.drawEllipse(centreX - innerRadius, centreY - innerRadius, innerRadius * 2.0f, innerRadius * 2.0f, 1.5f);

        juce::Path pointer;
        pointer.startNewSubPath(centreX + 4.0f * std::sin(angle), centreY - 4.0f * std::cos(angle));
        pointer.lineTo(centreX + (innerRadius - 2.0f) * std::sin(angle), centreY - (innerRadius - 2.0f) * std::cos(angle));
        g.setColour(juce::Colours::white);
        g.strokePath(pointer, juce::PathStrokeType(2.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }
};

static FLStudioLookAndFeel flLookAndFeel;

void setupSlider(juce::Slider &slider, juce::Label &label, const juce::String &text, juce::AudioProcessorValueTreeState &apvts, const juce::String &paramID, std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> &attachment, juce::Component *editor)
{
    slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 55, 20);
    editor->addAndMakeVisible(slider);
    attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvts, paramID, slider);

    label.setText(text, juce::dontSendNotification);
    label.setJustificationType(juce::Justification::centred);
    label.setColour(juce::Label::textColourId, juce::Colours::white);
    editor->addAndMakeVisible(label);
}

void setupComboBox(juce::ComboBox &box, juce::Label &label, const juce::String &text, juce::AudioProcessorValueTreeState &apvts, const juce::String &paramID, std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> &attachment, juce::Component *editor)
{
    editor->addAndMakeVisible(box);
    attachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(apvts, paramID, box);

    label.setText(text, juce::dontSendNotification);
    label.setJustificationType(juce::Justification::centred);
    label.setColour(juce::Label::textColourId, juce::Colours::white);
    editor->addAndMakeVisible(label);
}

AudioPluginAudioProcessorEditor::AudioPluginAudioProcessorEditor(AudioPluginAudioProcessor &p)
    : AudioProcessorEditor(&p), audioProcessor(p)
{
    bgImage = juce::ImageCache::getFromMemory(BinaryData::bg_image_jpg, BinaryData::bg_image_jpgSize);
    setLookAndFeel(&flLookAndFeel);

    setupSlider(speedSlider, speedLabel, "SPEED", audioProcessor.apvts, "SPEED", speedAttach, this);

    // コンボボックスのアイテム設定
    keyComboBox.addItemList({"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"}, 1);
    scaleComboBox.addItemList({"Chromatic", "Major", "Natural Minor", "Pentatonic"}, 1);

    setupComboBox(keyComboBox, keyLabel, "KEY", audioProcessor.apvts, "KEY", keyAttach, this);
    setupComboBox(scaleComboBox, scaleLabel, "SCALE", audioProcessor.apvts, "SCALE", scaleAttach, this);

    setupSlider(midAirSlider, midAirLabel, "MID AIR", audioProcessor.apvts, "MIDAIR", midAirAttach, this);
    setupSlider(hiAirSlider, hiAirLabel, "HI AIR", audioProcessor.apvts, "HIAIR", hiAirAttach, this);
    setupSlider(threshSlider, threshLabel, "THRESH", audioProcessor.apvts, "THRESH", threshAttach, this);
    setupSlider(makeupSlider, makeupLabel, "MAKEUP", audioProcessor.apvts, "MAKEUP", makeupAttach, this);

    pitchHistory.resize(610, 0.0f);
    startTimerHz(30);
    setSize(640, 480);
}

AudioPluginAudioProcessorEditor::~AudioPluginAudioProcessorEditor()
{
    stopTimer();
    setLookAndFeel(nullptr);
}

void AudioPluginAudioProcessorEditor::timerCallback()
{
    float currentPitch = audioProcessor.currentDetectedFreq.load();
    float rms = audioProcessor.currentRMS.load();
    if (rms < 0.01f)
        currentPitch = 0.0f;

    std::copy(pitchHistory.begin() + 1, pitchHistory.end(), pitchHistory.begin());
    pitchHistory.back() = currentPitch;
    repaint();
}

void AudioPluginAudioProcessorEditor::paint(juce::Graphics &g)
{
    g.fillAll(juce::Colour(0xff23282C));

    juce::Rectangle<float> headerArea(0.0f, 0.0f, (float)getWidth(), 35.0f);
    g.setColour(juce::Colour(0xff363D43));
    g.fillRect(headerArea);
    g.setColour(juce::Colour(0xff15181A));
    g.drawHorizontalLine(35, 0.0f, (float)getWidth());

    g.setColour(juce::Colours::white);
    g.fillEllipse(12.0f, 10.0f, 15.0f, 15.0f);
    g.setFont(juce::FontOptions(16.0f, juce::Font::bold));
    g.drawText("MBG VOCAL MAGIC", 38, 0, 200, 35, juce::Justification::centredLeft);
    g.setColour(juce::Colour(0xff8F959D));
    g.setFont(juce::FontOptions(11.0f));
    g.drawText("VOCAL CHAIN / VST3", getWidth() - 130, 0, 115, 35, juce::Justification::centredRight);

    // モニター領域
    juce::Rectangle<float> monitorArea(15.0f, 50.0f, 610.0f, 180.0f);
    g.setColour(juce::Colour(0xff0a0c10));
    g.fillRect(monitorArea);
    g.setColour(juce::Colour(0xff262B30));
    for (int i = 1; i < 4; ++i)
        g.drawHorizontalLine((int)(monitorArea.getY() + (monitorArea.getHeight() / 4.0f) * (float)i), monitorArea.getX(), monitorArea.getRight());

    juce::Path pitchPath;
    bool pathStarted = false;
    for (size_t i = 0; i < pitchHistory.size(); ++i)
    {
        if (pitchHistory[i] > 50.0f)
        {
            float y = juce::jmap(std::log10(pitchHistory[i]), std::log10(50.0f), std::log10(1000.0f), monitorArea.getBottom(), monitorArea.getY());
            float x = monitorArea.getX() + (float)i;
            if (!pathStarted)
            {
                pitchPath.startNewSubPath(x, y);
                pathStarted = true;
            }
            else
            {
                pitchPath.lineTo(x, y);
            }
        }
        else
        {
            pathStarted = false;
        }
    }
    g.setColour(juce::Colours::cyan);
    g.strokePath(pitchPath, juce::PathStrokeType(2.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    g.setColour(juce::Colour(0xff3D444A));
    g.drawRect(monitorArea, 1.0f);

    // コントロールパネル領域
    juce::Rectangle<float> controlArea(15.0f, 245.0f, 610.0f, 220.0f);
    if (bgImage.isValid())
    {
        g.drawImage(bgImage, controlArea, juce::RectanglePlacement::fillDestination);
        g.setColour(juce::Colours::black.withAlpha(0.5f));
        g.fillRect(controlArea);
    }
    else
    {
        g.setColour(juce::Colour(0xff1C2024));
        g.fillRect(controlArea);
    }

    g.setColour(juce::Colours::white.withAlpha(0.2f));
    g.drawVerticalLine(218, 255.0f, 455.0f);
    g.drawVerticalLine(421, 255.0f, 455.0f);

    g.setColour(juce::Colours::white.withAlpha(0.7f));
    g.setFont(juce::FontOptions(12.0f, juce::Font::bold));
    g.drawText("AUTO-TUNE", 15, 250, 203, 20, juce::Justification::centred);
    g.drawText("FRESH AIR", 218, 250, 203, 20, juce::Justification::centred);
    g.drawText("COMPRESSOR", 421, 250, 203, 20, juce::Justification::centred);

    g.setColour(juce::Colour(0xff3D444A));
    g.drawRect(controlArea, 1.0f);
}

void AudioPluginAudioProcessorEditor::resized()
{
    int yPos = 310;

    // Auto-Tune (Speedノブ + Keyコンボ + Scaleコンボ)
    speedSlider.setBounds(25, yPos - 15, 75, 75);
    speedLabel.setBounds(25, yPos - 35, 75, 20);
    keyComboBox.setBounds(105, yPos + 10, 45, 25);
    keyLabel.setBounds(105, yPos - 10, 45, 20);
    scaleComboBox.setBounds(155, yPos + 10, 55, 25);
    scaleLabel.setBounds(155, yPos - 10, 55, 20);

    // Fresh Air (Mid Air, Hi Air)
    midAirSlider.setBounds(235, yPos - 15, 75, 75);
    midAirLabel.setBounds(235, yPos - 35, 75, 20);
    hiAirSlider.setBounds(325, yPos - 15, 75, 75);
    hiAirLabel.setBounds(325, yPos - 35, 75, 20);

    // Compressor (Threshold, Makeup)
    threshSlider.setBounds(440, yPos - 15, 75, 75);
    threshLabel.setBounds(440, yPos - 35, 75, 20);
    makeupSlider.setBounds(530, yPos - 15, 75, 75);
    makeupLabel.setBounds(530, yPos - 35, 75, 20);
}