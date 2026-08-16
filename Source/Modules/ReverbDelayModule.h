#pragma once
#include <JuceHeader.h>
#include "../Common/IProcessModule.h"

class ReverbDelayModule : public IProcessModule
{
public:
    ReverbDelayModule();
    ~ReverbDelayModule() override = default;

    void prepare(const juce::dsp::ProcessSpec& spec) override;
    void reset() override;
    void process(juce::dsp::AudioBlock<float>& block) override;
    void setBypass(bool shouldBypass) override { bypassed = shouldBypass; }
    bool isBypassed() const override { return bypassed; }
    juce::String getName() const override { return "Reverb & Delay"; }

    void setReverbWet(float wet) { rParams.wetLevel = wet; reverb.setParameters(rParams); }
    void setDelayTimeMs(float ms);
    void setDelayMix(float m) { delayMix = m; }

private:
    bool bypassed { false };
    double sampleRate { 44100.0 };

    juce::dsp::Reverb reverb;
    juce::dsp::Reverb::Parameters rParams;

    juce::AudioBuffer<float> delayBuffer;
    int delayWritePosition { 0 };
    int delaySampleLength { 0 };
    float delayMix { 0.15f };
};