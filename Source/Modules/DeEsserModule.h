#pragma once
#include <JuceHeader.h>
#include "../Common/IProcessModule.h"

enum class DeEsserMode { Wideband, SplitBand };

class DeEsserModule : public IProcessModule
{
public:
    DeEsserModule();
    ~DeEsserModule() override = default;

    void prepare(const juce::dsp::ProcessSpec& spec) override;
    void reset() override;
    void process(juce::dsp::AudioBlock<float>& block) override;
    void setBypass(bool shouldBypass) override { bypassed = shouldBypass; }
    bool isBypassed() const override { return bypassed; }
    juce::String getName() const override { return "De-Esser"; }

    void setFrequency(float freqHz) { frequencyHz = juce::jlimit(4000.0f, 10000.0f, freqHz); updateFilters(); }
    void setThresholdDb(float thresh) { thresholdDb = juce::jlimit(-60.0f, 0.0f, thresh); }
    void setAmountDb(float amount) { maxReductionDb = juce::jlimit(0.0f, 24.0f, amount); }
    void setMode(DeEsserMode newMode) { mode = newMode; }
    float getGainReductionDb() const { return currentGR.load(); }

private:
    void updateFilters();

    bool bypassed { false };
    double sampleRate { 44100.0 };

    float frequencyHz { 6000.0f };
    float thresholdDb { -20.0f };
    float maxReductionDb { 12.0f };
    DeEsserMode mode { DeEsserMode::SplitBand };

    using Filter = juce::dsp::IIR::Filter<float>;
    Filter bandpassFilter;
    Filter highpassCrossover;
    Filter lowpassCrossover;

    float envelope { 0.0f };
    float attackAlpha { 0.0f }, releaseAlpha { 0.0f };
    std::atomic<float> currentGR { 0.0f };
};