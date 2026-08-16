#pragma once
#include <JuceHeader.h>
#include "../Common/IProcessModule.h"

class GlueCompModule : public IProcessModule
{
public:
    GlueCompModule();
    ~GlueCompModule() override = default;

    void prepare(const juce::dsp::ProcessSpec& spec) override;
    void reset() override;
    void process(juce::dsp::AudioBlock<float>& block) override;
    void setBypass(bool shouldBypass) override { bypassed = shouldBypass; }
    bool isBypassed() const override { return bypassed; }
    juce::String getName() const override { return "Glue Compressor"; }

    void setThresholdDb(float t) { thresholdDb = t; }
    void setRatio(float r) { ratio = r; }
    void setAttackMs(float a) { attackMs = a; updateCoeffs(); }
    void setReleaseMs(float r) { releaseMs = r; updateCoeffs(); }
    float getGainReductionDb() const { return currentGR.load(); }

private:
    void updateCoeffs();

    bool bypassed { false };
    double sampleRate { 44100.0 };
    float thresholdDb { -10.0f };
    float ratio { 2.0f };
    float attackMs { 30.0f }, releaseMs { 100.0f };

    float envDb { 0.0f };
    float attAlpha { 0.0f }, relAlpha { 0.0f };
    std::atomic<float> currentGR { 0.0f };
};