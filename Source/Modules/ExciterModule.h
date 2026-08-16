#pragma once
#include <JuceHeader.h>
#include "../Common/IProcessModule.h"

class ExciterModule : public IProcessModule
{
public:
    ExciterModule() = default;
    ~ExciterModule() override = default;

    void prepare(const juce::dsp::ProcessSpec& spec) override { sampleRate = spec.sampleRate; }
    void reset() override {}
    void process(juce::dsp::AudioBlock<float>& block) override;
    void setBypass(bool shouldBypass) override { bypassed = shouldBypass; }
    bool isBypassed() const override { return bypassed; }
    juce::String getName() const override { return "Saturation / Exciter"; }

    void setEvenHarmonics(float e) { evenAmount = juce::jlimit(0.0f, 1.0f, e); }
    void setOddHarmonics(float o) { oddAmount = juce::jlimit(0.0f, 1.0f, o); }
    void setMix(float m) { mix = juce::jlimit(0.0f, 1.0f, m); }

private:
    bool bypassed { false };
    double sampleRate { 44100.0 };
    float evenAmount { 0.2f };
    float oddAmount { 0.2f };
    float mix { 0.3f };
};