#pragma once
#include <JuceHeader.h>
#include "../Common/IProcessModule.h"

enum class HPFSlope
{
    Slope12dB = 0, // 2nd ordre (1 Biquad)
    Slope24dB      // 4th ordre (2 Biquads en cascade)
};

class HPFModule : public IProcessModule
{
public:
    HPFModule();
    ~HPFModule() override = default;

    void prepare(const juce::dsp::ProcessSpec& spec) override;
    void reset() override;
    void process(juce::dsp::AudioBlock<float>& block) override;
    void setBypass(bool shouldBypass) override { bypassed = shouldBypass; }
    bool isBypassed() const override { return bypassed; }
    juce::String getName() const override { return "High-Pass Filter"; }

    /** Définit la fréquence de coupure en Hz (20Hz à 300Hz) */
    void setCutoffFrequency(float newFrequencyHz);

    /** Définit la pente du filtre (12 dB/oct ou 24 dB/oct) */
    void setSlope(HPFSlope newSlope);

private:
    void updateFilters();

    bool bypassed { false };
    double sampleRate { 44100.0 };

    float cutoffHz { 80.0f };
    HPFSlope currentSlope { HPFSlope::Slope12dB };

    // Duplex IIR Filter de JUCE (découple la logique DSP bas niveau)
    using Filter = juce::dsp::IIR::Filter<float>;
    

    Filter filterCascade1;
    Filter filterCascade2;
};