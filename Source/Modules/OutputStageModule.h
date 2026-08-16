#pragma once
#include <JuceHeader.h>
#include "../Common/IProcessModule.h"

class OutputStageModule : public IProcessModule
{
public:
    OutputStageModule();
    ~OutputStageModule() override = default;

    void prepare(const juce::dsp::ProcessSpec& spec) override;
    void reset() override;
    void process(juce::dsp::AudioBlock<float>& block) override;
    void setBypass(bool shouldBypass) override { bypassed = shouldBypass; }
    bool isBypassed() const override { return bypassed; }
    juce::String getName() const override { return "Output Stage"; }

    void setOutputGainDb(float gainDb) { volume.setTargetValue(juce::Decibels::decibelsToGain(gainDb)); }
    float getPeakLevel(int ch) const { return peakLevels[ch]; }
    float getRmsLevel(int ch) const { return std::sqrt(rmsLevels[ch]); }
    float getLufsMomentary() const { return lufsMomentary.load(); }

private:
    bool bypassed { false };
    double sampleRate { 44100.0 };
    juce::SmoothedValue<float> volume { 1.0f };

    std::array<float, 2> peakLevels { 0.0f, 0.0f };
    std::array<float, 2> rmsLevels { 0.0f, 0.0f };
    float alphaRms { 0.0f };

    // K-Weighting Filter (LUFS Pre-Filter + High-Shelf)
    juce::dsp::IIR::Filter<float> kWeightHighShelf;
    juce::dsp::IIR::Filter<float> kWeightHighPass;
    std::atomic<float> lufsMomentary { -100.0f };
};