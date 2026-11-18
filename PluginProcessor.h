#pragma once
#include <JuceHeader.h>
#include <atomic>

class VoxOneAudioProcessor final : public juce::AudioProcessor {
public:
    VoxOneAudioProcessor();
    void prepareToPlay(double, int) override;
    void releaseResources() override {}
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }
    const juce::String getName() const override { return "VoxOne"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0; }
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return {}; }
    void changeProgramName(int, const juce::String&) override {}
    void getStateInformation(juce::MemoryBlock&) override;
    void setStateInformation(const void*, int) override;
    bool isBusesLayoutSupported(const BusesLayout&) const override;
    juce::AudioProcessorValueTreeState parameters;
    std::atomic<float> gainReductionDb { 0.0f }, inputLevelDb { -90.0f }, outputLevelDb { -90.0f };
private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();
    float sampleRateHz = 44100.f;
    float detector = 0.f, gateDetector = 0.f, gainDbSmoothed = 0.f;
    float previousIn[2] { 0.f, 0.f };
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(VoxOneAudioProcessor)
};
