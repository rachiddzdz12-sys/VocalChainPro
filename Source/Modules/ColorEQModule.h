#pragma once
#include <JuceHeader.h>
#include "../Common/IProcessModule.h"

class ColorEQModule : public IProcessModule
{
public:
    static constexpr size_t numBands = 6;
    struct Band { float freq { 1000.0f }; float gainDb { 0.0f }; float Q { 0.7071f }; bool enabled { true }; };

    ColorEQModule();
    ~ColorEQModule() override = default;

    void prepare(const juce::dsp::ProcessSpec& spec) override;
    void reset() override;
    void process(juce::dsp::AudioBlock<float>& block) override;
    void setBypass(bool shouldBypass) override { bypassed = shouldBypass; }
    bool isBypassed() const override { return bypassed; }
    juce::String getName() const override { return "Color EQ"; }

    void setBand(size_t index, float f, float g, float q) {
        if (index < numBands) { bands[index].freq = f; bands[index].gainDb = g; bands[index].Q = q; updateBand(index); }
    }

private:
    void updateBand(size_t index);

    bool bypassed { false };
    double sampleRate { 44100.0 };
    std::array<Band, numBands> bands;

    using Filter = juce::dsp::IIR::Filter<float>;
    std::array<juce::dsp::ProcessorDuplex<Filter>, numBands> filters;
};