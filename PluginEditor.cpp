#include "PluginEditor.h"
#include <cmath>

void VoxOneAudioProcessorEditor::ThemeLookAndFeel::drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height,
    float position, float startAngle, float endAngle, juce::Slider&) {
    auto bounds = juce::Rectangle<float>((float)x, (float)y, (float)width, (float)height).reduced(12.f);
    const auto radius = juce::jmin(bounds.getWidth(), bounds.getHeight()) * .43f;
    auto centre = bounds.getCentre();
    juce::Path track, progress;
    track.addCentredArc(centre.x, centre.y, radius, radius, 0.f, startAngle, endAngle, true);
    progress.addCentredArc(centre.x, centre.y, radius, radius, 0.f, startAngle,
                           startAngle + position * (endAngle - startAngle), true);
    g.setColour(surface.brighter(.14f)); g.strokePath(track, juce::PathStrokeType(7.f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    g.setColour(accent); g.strokePath(progress, juce::PathStrokeType(7.f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    g.setColour(surface.brighter(.08f)); g.fillEllipse(centre.x - radius + 12.f, centre.y - radius + 12.f, 2.f * (radius - 12.f), 2.f * (radius - 12.f));
    const auto angle = startAngle + position * (endAngle - startAngle);
    const auto tip = centre.getPointOnCircumference(radius - 18.f, angle);
    g.setColour(foreground); g.drawLine(centre.x, centre.y, tip.x, tip.y, 3.f);
}

VoxOneAudioProcessorEditor::VoxOneAudioProcessorEditor(VoxOneAudioProcessor& p) : AudioProcessorEditor(&p), processor(p) {
    setLookAndFeel(&theme);
    auto setup = [this](juce::Slider& s, juce::Label& l, const juce::String& title) {
        s.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        s.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 100, 27);
        l.setText(title, juce::dontSendNotification);
        l.setJustificationType(juce::Justification::centred);
        addAndMakeVisible(s); addAndMakeVisible(l);
    };
    setup(gate, gateLabel, "GATE"); setup(comp, compLabel, "COMP");
    setup(gain, gainLabel, "GAIN"); setup(saturation, satLabel, "SATURATION");
    gateAttachment = std::make_unique<SliderAttachment>(processor.parameters, "gate", gate);
    compAttachment = std::make_unique<SliderAttachment>(processor.parameters, "comp", comp);
    gainAttachment = std::make_unique<SliderAttachment>(processor.parameters, "gain", gain);
    satAttachment = std::make_unique<SliderAttachment>(processor.parameters, "saturation", saturation);
    skin.addItemList({ "Midnight", "Miami Vice", "Classic Vox", "Pastel", "SSL 4000" }, 1);
    meterMode.addItemList({ "Input", "Output", "Gain Reduction" }, 1);
    meterMode.setSelectedId(2, juce::dontSendNotification);
    addAndMakeVisible(meterMode);
    satType.addItemList({ "Off", "Tape", "Tube", "Clip", "Warm" }, 1);
    skinAttachment = std::make_unique<ComboAttachment>(processor.parameters, "skin", skin);
    typeAttachment = std::make_unique<ComboAttachment>(processor.parameters, "satType", satType);
    skinLabel.setText("SKIN", juce::dontSendNotification);
    typeLabel.setText("SATURATION TYPE", juce::dontSendNotification);
    for (auto* c : { &skin, &satType }) addAndMakeVisible(*c);
    for (auto* c : { &skinLabel, &typeLabel, &meterLabel }) { c->setJustificationType(juce::Justification::centredLeft); addAndMakeVisible(*c); }
    setSize(900, 520);
    setResizable(true, true);
    setResizeLimits(700, 410, 1600, 920);
    applyTheme(); startTimerHz(20);
}
VoxOneAudioProcessorEditor::~VoxOneAudioProcessorEditor() { setLookAndFeel(nullptr); }
void VoxOneAudioProcessorEditor::applyTheme() {
    const int id = juce::jlimit(0, 4, (int)std::round(processor.parameters.getRawParameterValue("skin")->load()));
    if (id == currentSkin) return;
    currentSkin = id;
    switch (id) {
        case 1: bg = juce::Colour(0xff15132b); surface = juce::Colour(0xff292143); ink = juce::Colour(0xfff9ecff); muted = juce::Colour(0xffb7a5d4); accent = juce::Colour(0xffff58b8); break;
        case 2: bg = juce::Colour(0xff33383c); surface = juce::Colour(0xff555d62); ink = juce::Colour(0xfff4f4ef); muted = juce::Colour(0xffc1c8ca); accent = juce::Colour(0xffe8b964); break;
        case 4: bg = juce::Colour(0xff292d32); surface = juce::Colour(0xff353a40); ink = juce::Colour(0xfff2f0e9); muted = juce::Colour(0xffb5b7b4); accent = juce::Colour(0xffc23535); break;
        case 3: bg = juce::Colour(0xfff7eaf1); surface = juce::Colour(0xffe4d9e8); ink = juce::Colour(0xff4e4259); muted = juce::Colour(0xff7d7185); accent = juce::Colour(0xffb78bcf); break;
        default: bg = juce::Colour(0xff111820); surface = juce::Colour(0xff1b2530); ink = juce::Colour(0xfff1f4f8); muted = juce::Colour(0xff96a5b1); accent = juce::Colour(0xff64e2ca); break;
    }
    theme.accent = accent; theme.surface = surface; theme.foreground = ink;
    for (auto* s : { &gate, &comp, &gain, &saturation }) {
        s->setColour(juce::Slider::textBoxTextColourId, ink);
        s->setColour(juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
        s->setColour(juce::Slider::textBoxBackgroundColourId, juce::Colours::transparentBlack);
    }
    for (auto* l : { &gateLabel, &compLabel, &gainLabel, &satLabel, &skinLabel, &typeLabel }) l->setColour(juce::Label::textColourId, muted);
    meterLabel.setColour(juce::Label::textColourId, accent);
    for (auto* c : { &skin, &satType, &meterMode }) {
        c->setColour(juce::ComboBox::backgroundColourId, surface);
        c->setColour(juce::ComboBox::textColourId, ink);
        c->setColour(juce::ComboBox::outlineColourId, accent.withAlpha(.5f));
        c->setColour(juce::ComboBox::arrowColourId, accent);
    }
    repaint();
}
void VoxOneAudioProcessorEditor::paint(juce::Graphics& g) {
    g.fillAll(bg);
    const float sx = getWidth() / 900.f, sy = getHeight() / 520.f;
    g.saveState();
    g.addTransform(juce::AffineTransform::scale(sx, sy));
    g.setColour(accent); g.setFont(juce::Font(juce::FontOptions(39.f, juce::Font::bold)));
    g.drawText("VoxOne", 30, 16, 260, 55, juce::Justification::centredLeft);
    g.setColour(muted); g.setFont(juce::Font(juce::FontOptions(13.f)));
    g.drawText("VOCAL DYNAMICS", 34, 69, 280, 22, juce::Justification::centredLeft);
    g.setColour(surface); g.fillRoundedRectangle(24.f, 110.f, 852.f, 356.f, 14.f);
    g.setColour(accent.withAlpha(.5f)); g.drawRoundedRectangle(24.f, 110.f, 852.f, 356.f, 14.f, 1.f);
    for (int x : { 235, 450, 660 }) { g.setColour(muted.withAlpha(.28f)); g.drawLine((float)x, 132.f, (float)x, 443.f, 1.f); }
    g.setColour(ink); g.setFont(juce::Font(juce::FontOptions(14.f, juce::Font::bold)));
    g.drawText("VU METER", 678, 131, 178, 22, juce::Justification::centred);
    auto box = juce::Rectangle<float>(679.f, 209.f, 174.f, 98.f);
    const bool analog = currentSkin == 4;
    if (analog) {
        g.setColour(juce::Colour(0xffddc79b)); g.fillRoundedRectangle(box, 8.f);
        g.setColour(juce::Colour(0xff292d32)); g.drawRoundedRectangle(box, 8.f, 2.f);
        const auto cx = box.getCentreX(), cy = box.getBottom() - 13.f;
        for (int i = 0; i <= 10; ++i) {
            const float angle = juce::MathConstants<float>::pi * (1.20f + .60f * i / 10.f);
            auto p1 = juce::Point<float>(cx + 69.f * std::cos(angle), cy + 69.f * std::sin(angle));
            auto p2 = juce::Point<float>(cx + 78.f * std::cos(angle), cy + 78.f * std::sin(angle));
            g.drawLine(p1.x, p1.y, p2.x, p2.y, 1.5f);
        }
        float normalized = juce::jlimit(0.f, 1.f, (displayedMeter + 60.f) / 66.f);
        float angle = juce::MathConstants<float>::pi * (1.20f + .60f * normalized);
        g.setColour(juce::Colour(0xffa72e2e));
        g.drawLine(cx, cy, cx + 74.f * std::cos(angle), cy + 74.f * std::sin(angle), 2.4f);
        g.setColour(juce::Colour(0xff333333)); g.fillEllipse(cx - 4.f, cy - 4.f, 8.f, 8.f);
    } else {
        g.setColour(bg); g.fillRoundedRectangle(box, 7.f);
        const float norm = juce::jlimit(0.f, 1.f, (displayedMeter + 60.f) / 66.f);
        for (int i = 0; i < 22; ++i) {
            g.setColour(i / 22.f < norm ? (i > 18 ? juce::Colours::orange : accent) : surface.brighter(.18f));
            g.fillRoundedRectangle(684.f + i * 7.4f, 242.f, 5.3f, 28.f, 1.4f);
        }
    }
    g.setColour(muted); g.setFont(juce::Font(juce::FontOptions(11.f)));
    g.drawText(meterMode.getSelectedId() == 3 ? "0 to 24 dB reduction" : "-60 to +6 dBFS", 683, 311, 170, 20, juce::Justification::centred);
    g.drawText("VOCAL COMPRESSOR / EXPANDER  •  VoxOne v0.3", 25, 480, 850, 25, juce::Justification::centred);
    g.restoreState();
}
void VoxOneAudioProcessorEditor::resized() {
    const float sx = getWidth() / 900.f, sy = getHeight() / 520.f;
    auto place = [sx,sy](juce::Component& component, int x, int y, int w, int h) {
        component.setBounds(juce::roundToInt(x*sx), juce::roundToInt(y*sy), juce::roundToInt(w*sx), juce::roundToInt(h*sy));
    };
    place(skinLabel, 545, 22, 55, 25); place(skin, 606, 21, 263, 33);
    auto slider = [&](juce::Slider& s, juce::Label& l, int x) {
        place(l, x, 135, 184, 30); place(s, x, 175, 184, 247);
    };
    slider(gate, gateLabel, 35); slider(comp, compLabel, 247);
    slider(gain, gainLabel, 460);
    place(meterMode, 685, 165, 165, 33);
    place(satLabel, 685, 335, 165, 24);
    place(saturation, 708, 359, 119, 90);
    place(typeLabel, 675, 452, 80, 20); place(satType, 754, 447, 110, 27);
    place(meterLabel, 675, 102, 190, 24);
}
void VoxOneAudioProcessorEditor::timerCallback() {
    applyTheme();
    const int mode = meterMode.getSelectedId();
    const float raw = mode == 1 ? processor.inputLevelDb.load() : mode == 3 ? -processor.gainReductionDb.load() : processor.outputLevelDb.load();
    const float target = mode == 3 ? juce::jlimit(-60.f, 6.f, raw + 6.f) : juce::jlimit(-60.f, 6.f, raw);
    displayedMeter += (target - displayedMeter) * (target > displayedMeter ? .52f : .16f);
    meterLabel.setText(mode == 3 ? "GR " + juce::String(processor.gainReductionDb.load(), 1) + " dB" :
                       (mode == 1 ? "IN " : "OUT ") + juce::String(raw, 1) + " dBFS", juce::dontSendNotification);
    repaint();
}
juce::AudioProcessorEditor* VoxOneAudioProcessor::createEditor() { return new VoxOneAudioProcessorEditor(*this); }
