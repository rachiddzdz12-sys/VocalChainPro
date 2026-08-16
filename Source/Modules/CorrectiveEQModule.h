#pragma once
#include <JuceHeader.h>
#include "../Common/IProcessModule.h"

class CorrectiveEQModule : public IProcessModule
{
public:
    enum FilterType
    {
        LowShelf = 0,
        Bell,
        HighShelf
    };

    struct BandConfig
    {
        FilterType type { Bell };
        float frequencyHz { 1000.0f };
        float gainDb { 0.0f };
        float Q { 0.7071f };
        bool enabled { true };
    };

    CorrectiveEQModule();
    ~CorrectiveEQModule() override = default;

    void prepare(const juce::dsp::ProcessSpec& spec) override;
    void reset() override;
    void process(juce::dsp::AudioBlock<float>& block) override;
    void setBypass(bool shouldBypass) override { bypassed = shouldBypass; }
    bool isBypassed() const override { return bypassed; }
    juce::String getName() const override { return "Corrective EQ (Pre-Comp)"; }

    // --- Configuration des bandes ---
    void setBandParameters(size_t bandIndex, float freqHz, float gainDb, float Q);
    void setBandEnabled(size_t bandIndex, bool enabled);

    static constexpr size_t numBands = 4;

private:
    void updateBandCoefficients(size_t bandIndex);

    bool bypassed { false };
    double sampleRate { 44100.0 };

    std::array<BandConfig, numBands> bandConfigs;

    using Filter = juce::dsp::IIR::Filter<float>;
    

    std::array<Filter, numBands> filters;
};