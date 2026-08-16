#pragma once
#include <JuceHeader.h>
#include "../Common/IProcessModule.h"

enum class CompMode
{
    Opto = 0, // Style LA-2A (Attaque douce, release 2-étages)
    FET,      // Style 1176 (Attaque ultra-rapide, punchy)
    VCA       // Style SSL/dBx (Précis, moderne, linéaire)
};

class CompOptoFETModule : public IProcessModule
{
public:
    CompOptoFETModule();
    ~CompOptoFETModule() override = default;

    void prepare(const juce::dsp::ProcessSpec& spec) override;
    void reset() override;
    void process(juce::dsp::AudioBlock<float>& block) override;
    void setBypass(bool shouldBypass) override { bypassed = shouldBypass; }
    bool isBypassed() const override { return bypassed; }
    juce::String getName() const override { return "Compressor #1 (Control)"; }

    // --- Paramètres ---
    void setThresholdDb(float thresholdDb);
    void setRatio(float ratio);
    void setAttackMs(float attackMs);
    void setReleaseMs(float releaseMs);
    void setKneeDb(float kneeDb);
    void setMakeupGainDb(float makeupDb);
    void setAutoMakeupEnabled(bool enableAutoMakeup);
    void setMode(CompMode newMode);

    /** Renvoie la Gain Reduction instantanée pour le VU-mètre (dB positif, ex: 6.0 = -6dB) */
    float getGainReductionDb() const { return currentGainReductionDb.load(); }

private:
    void updateBallistics();
    float computeGainReductionDb(float detectorDb);

    bool bypassed { false };
    double sampleRate { 44100.0 };

    // Regglages utilisateur
    float thresholdDb { -20.0f };
    float ratio { 4.0f };
    float attackMs { 10.0f };
    float releaseMs { 100.0f };
    float kneeDb { 6.0f };
    float manualMakeupDb { 0.0f };
    bool autoMakeup { false };
    CompMode mode { CompMode::Opto };

    // Enveloppe & Filtres DSP
    float detectorEnvelope { 0.0f };
    float grEnvelopeDb { 0.0f };

    float attackAlpha { 0.0f };
    float releaseAlpha { 0.0f };
    float optoReleaseSlowAlpha { 0.0f };

    // Métrique UI
    std::atomic<float> currentGainReductionDb { 0.0f };
};