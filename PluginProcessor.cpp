#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <cmath>
#include <vector>

namespace {
float db(float x) { return juce::Decibels::gainToDecibels(x, -100.f); }
float lin(float x) { return juce::Decibels::decibelsToGain(x); }
float softClip(float x, int type) {
    switch (type) {
        case 1: return std::tanh(x); // Tape-like smooth rounding
        case 2: return (2.f / juce::MathConstants<float>::pi) * std::atan(1.8f * x); // Tube-like
        case 3: return juce::jlimit(-1.f, 1.f, x); // Hard clip
        case 4: return x / (1.f + std::abs(x)); // Warm fold-free asymptote
        default: return x;
    }
}
}
VoxOneAudioProcessor::VoxOneAudioProcessor()
 : AudioProcessor(BusesProperties().withInput("Input", juce::AudioChannelSet::stereo(), true)
                                   .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
   parameters(*this, nullptr, "PARAMETERS", createLayout()) {}

juce::AudioProcessorValueTreeState::ParameterLayout VoxOneAudioProcessor::createLayout() {
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> p;
    p.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"gate", 1}, "Gate", juce::NormalisableRange<float>(-80.f, -20.f, 0.1f), -65.f, juce::AudioParameterFloatAttributes().withLabel("dB")));
    p.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"comp", 1}, "Comp", juce::NormalisableRange<float>(0.f, 24.f, 0.1f), 8.f, juce::AudioParameterFloatAttributes().withLabel("dB")));
    p.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"gain", 1}, "Gain", juce::NormalisableRange<float>(-24.f, 24.f, 0.1f), 0.f, juce::AudioParameterFloatAttributes().withLabel("dB")));
    p.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"saturation", 1}, "Saturation", juce::NormalisableRange<float>(0.f, 100.f, 0.1f), 0.f, juce::AudioParameterFloatAttributes().withLabel("%")));
    p.push_back(std::make_unique<juce::AudioParameterChoice>(juce::ParameterID{"satType", 1}, "Saturation Type", juce::StringArray{"Off", "Tape", "Tube", "Clip", "Warm"}, 0));
    p.push_back(std::make_unique<juce::AudioParameterChoice>(juce::ParameterID{"skin", 1}, "Skin", juce::StringArray{"Midnight", "Miami Vice", "Classic Vox", "Pastel", "SSL 4000"}, 0));
    return { p.begin(), p.end() };
}
bool VoxOneAudioProcessor::isBusesLayoutSupported(const BusesLayout& l) const {
    auto in = l.getMainInputChannelSet(), out = l.getMainOutputChannelSet();
    return in == out && (in == juce::AudioChannelSet::mono() || in == juce::AudioChannelSet::stereo());
}
void VoxOneAudioProcessor::prepareToPlay(double sr, int) {
    sampleRateHz = (float)sr; detector = gateDetector = gainDbSmoothed = 0.f;
    previousIn[0] = previousIn[1] = 0.f;
    gainReductionDb.store(0); inputLevelDb.store(-90); outputLevelDb.store(-90);
}
void VoxOneAudioProcessor::processBlock(juce::AudioBuffer<float>& b, juce::MidiBuffer&) {
    juce::ScopedNoDenormals noDenormals;
    const int channels = juce::jmin(2, juce::jmin(b.getNumChannels(), getTotalNumInputChannels()));
    if (channels == 0) { b.clear(); return; }
    const float gateDb = parameters.getRawParameterValue("gate")->load();
    const float amount = parameters.getRawParameterValue("comp")->load();
    const float gain = parameters.getRawParameterValue("gain")->load();
    const float sat = parameters.getRawParameterValue("saturation")->load() * .01f;
    const int satType = juce::jlimit(0, 4, (int)std::round(parameters.getRawParameterValue("satType")->load()));
    const auto coeff = [this](float ms) { return std::exp(-1000.f / (ms * sampleRateHz)); };
    const float attack = coeff(3.5f), release = coeff(95.f);
    const float gateAttack = coeff(2.f), gateRelease = coeff(170.f);
    const float gainAttack = coeff(3.f), gainRelease = coeff(75.f);
    float inputPeak = 0.f, outputPeak = 0.f, peakReduction = 0.f;
    for (int i = 0; i < b.getNumSamples(); ++i) {
        float peak = 0.f;
        for (int ch = 0; ch < channels; ++ch) peak = juce::jmax(peak, std::abs(b.getSample(ch, i)));
        inputPeak = juce::jmax(inputPeak, peak);
        detector = peak + (peak > detector ? attack : release) * (detector - peak);
        gateDetector = peak + (peak > gateDetector ? gateAttack : gateRelease) * (gateDetector - peak);
        // Single-control threshold/amount and compensating makeup: a vocal-forward approximation,
        // NOT an implementation of Waves' proprietary Renaissance Vox processing.
        const float threshold = -10.f - 1.25f * amount;
        const float above = db(detector) - threshold;
        const float knee = 12.f, slope = .77f;
        float reduction = 0.f;
        if (above >= knee * .5f) reduction = slope * above;
        else if (above > -knee * .5f) { const float k = above + knee * .5f; reduction = slope * k * k / (2.f * knee); }
        reduction = juce::jmax(0.f, reduction);
        const float expansion = juce::jlimit(-24.f, 0.f, (db(gateDetector) - gateDb) * 1.2f);
        const float makeup = amount * .55f;
        const float targetGainDb = -reduction + expansion + makeup + gain;
        const float smoothing = targetGainDb < gainDbSmoothed ? gainAttack : gainRelease;
        gainDbSmoothed = targetGainDb + smoothing * (gainDbSmoothed - targetGainDb);
        peakReduction = juce::jmax(peakReduction, reduction);
        const float levelGain = lin(gainDbSmoothed);
        for (int ch = 0; ch < channels; ++ch) {
            const float x = b.getSample(ch, i) * levelGain;
            float y = x;
            if (satType != 0 && sat > 0.f) {
                // Normalize the saturator around low levels; blend dry/wet continuously.
                const float drive = 1.f + 9.f * sat;
                const float wet = softClip(x * drive, satType) / juce::jmax(.001f, softClip(drive, satType));
                y = x * (1.f - sat) + wet * sat;
            }
            // A soft safety ceiling (not a lookahead true-peak limiter).
            const float limited = .99f * std::tanh(y / .99f);
            b.setSample(ch, i, limited);
            outputPeak = juce::jmax(outputPeak, std::abs(limited));
        }
    }
    for (int ch = channels; ch < b.getNumChannels(); ++ch) b.clear(ch, 0, b.getNumSamples());
    gainReductionDb.store(peakReduction);
    inputLevelDb.store(db(inputPeak)); outputLevelDb.store(db(outputPeak));
}
void VoxOneAudioProcessor::getStateInformation(juce::MemoryBlock& dest) {
    if (auto xml = parameters.copyState().createXml()) copyXmlToBinary(*xml, dest);
}
void VoxOneAudioProcessor::setStateInformation(const void* data, int size) {
    if (auto xml = getXmlFromBinary(data, size))
        if (xml->hasTagName(parameters.state.getType())) parameters.replaceState(juce::ValueTree::fromXml(*xml));
}
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new VoxOneAudioProcessor(); }
