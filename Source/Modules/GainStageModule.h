#pragma once
#include <JuceHeader.h>
#include "../Common/IProcessModule.h"

class GainStageModule : public IProcessModule
{
public:
    GainStageModule();
    ~GainStageModule() override = default;

    // --- Implémentation de IProcessModule ---
    void prepare(const juce::dsp::ProcessSpec& spec) override;
    void reset() override;
    void process(juce::dsp::AudioBlock<float>& block) override;
    void setBypass(bool shouldBypass) override { bypassed = shouldBypass; }
    bool isBypassed() const override { return bypassed; }
    juce::String getName() const override { return "Input / Gain Stage"; }

    // --- Contrôles de paramètres ---
    /** Définit le gain du trim en dB (-24.0f à +24.0f) */
    void setGainDb(float gainInDb);
    
    /** Active/désactive l'inversion de phase */
    void setPhaseInvert(bool shouldInvert);

    // --- Télémesures / Métering ---
    /** Renvoie la valeur Crête (Peak) linéaire max courante */
    float getPeakLevel(int channel) const;
    
    /** Renvoie la valeur RMS lissée linéaire courante */
    float getRmsLevel(int channel) const;

private:
    bool bypassed { false };
    bool phaseInverted { false };
    
    double sampleRate { 44100.0 };
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> smoothedGain;

    // Variables pour le métering
    static constexpr int maxChannels = 2;
    std::array<float, maxChannels> peakLevels { 0.0f, 0.0f };
    std::array<float, maxChannels> rmsLevels { 0.0f, 0.0f };

    // Filtrage IIR pour lissage RMS
    float alphaRms { 0.0f };
    static constexpr float rmsIntegrationTimeSec = 0.3f; // 300 ms ballistique RMS
};