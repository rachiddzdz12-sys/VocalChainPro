#pragma once
#include <JuceHeader.h>
#include "../Common/IProcessModule.h"

class LimiterModule : public IProcessModule
{
public:
    LimiterModule() = default;
    ~LimiterModule() override = default;

    void prepare(const juce::dsp::ProcessSpec& spec) override {
        limiter.prepare(spec);
        limiter.setThreshold(0.0f); // Ceiling -0.1 dB
        limiter.setRelease(50.0f);
    }
    void reset() override { limiter.reset(); }
    void process(juce::dsp::AudioBlock<float>& block) override {
        if (bypassed) return;
        juce::dsp::ProcessContextReplacing<float> ctx(block);
        limiter.process(ctx);
    }
    void setBypass(bool shouldBypass) override { bypassed = shouldBypass; }
    bool isBypassed() const override { return bypassed; }
    juce::String getName() const override { return "Output Limiter"; }

    void setCeilingDb(float ceilingDb) { limiter.setThreshold(ceilingDb); }

private:
    bool bypassed { false };
    juce::dsp::Limiter<float> limiter;
};