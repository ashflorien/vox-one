#pragma once
#include "PluginProcessor.h"

class VoxOneAudioProcessorEditor final : public juce::AudioProcessorEditor, private juce::Timer {
public:
    explicit VoxOneAudioProcessorEditor(VoxOneAudioProcessor&);
    ~VoxOneAudioProcessorEditor() override;
    void paint(juce::Graphics&) override;
    void resized() override;
private:
    class ThemeLookAndFeel : public juce::LookAndFeel_V4 {
    public:
        juce::Colour accent { 0xff64e2ca }, surface { 0xff1b2530 }, foreground { 0xfff1f4f8 };
        void drawRotarySlider(juce::Graphics&, int, int, int, int, float, float, float, juce::Slider&) override;
    } theme;
    void timerCallback() override;
    void applyTheme();
    VoxOneAudioProcessor& processor;
    juce::Slider gate, comp, gain, saturation;
    juce::ComboBox skin, satType, meterMode;
    juce::Label gateLabel, compLabel, gainLabel, satLabel, meterLabel, skinLabel, typeLabel;
    juce::Rectangle<int> meterBounds;
    float displayedMeter = -60.f;
    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ComboAttachment = juce::AudioProcessorValueTreeState::ComboBoxAttachment;
    std::unique_ptr<SliderAttachment> gateAttachment, compAttachment, gainAttachment, satAttachment;
    std::unique_ptr<ComboAttachment> skinAttachment, typeAttachment;
    int currentSkin = -1;
    juce::Colour bg { 0xff111820 }, surface { 0xff1b2530 }, ink { 0xfff1f4f8 }, muted { 0xff96a5b1 }, accent { 0xff64e2ca };
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(VoxOneAudioProcessorEditor)
};
