#pragma once
#include <JuceHeader.h>
#include "../Common/IProcessModule.h"

class GateModule : public IProcessModule
{
public:
    enum class GateState
    {
        Closed,
        Attack,
        Open,
        Hold,
        Release
    };

    GateModule();
    ~GateModule() override = default;

    void prepare(const juce::dsp::ProcessSpec& spec) override;
    void reset() override;
    void process(juce::dsp::AudioBlock<float>& block) override;
    void setBypass(bool shouldBypass) override { bypassed = shouldBypass; }
    bool isBypassed() const override { return bypassed; }
    juce::String getName() const override { return "Noise Gate / Expander"; }

    // --- Réglages des Paramètres ---
    void setThresholdDb(float thresholdDb);
    void setRatio(float ratio);
    void setAttackMs(float attackMs);
    void setReleaseMs(float releaseMs);
    void setHoldMs(float holdMs);

    /** Renvoie la Gain Reduction instantanée en dB (valeur positive, ex: 12.0dB de réduction) */
    float getGainReductionDb() const { return currentGainReductionDb.load(); }

private:
    bool bypassed { false };
    double sampleRate { 44100.0 };

    // Paramètres utilisateur
    float thresholdDb { -40.0f };
    float ratio { 4.0f }; // 1.0 = neutre, >4.0 = Gate/Expander fort
    float attackTimeMs { 2.0f };
    float releaseTimeMs { 100.0f };
    float holdTimeMs { 50.0f };

    // Coefficients d'enveloppe
    float attackAlpha { 0.0f };
    float releaseAlpha { 0.0f };

    // État de la machine de détection
    GateState currentState { GateState::Closed };
    float envelope { 0.0f };
    int holdCounter { 0 };
    int holdSamples { 0 };

    // Lissage du gain de sortie
    float currentGain { 0.0f };
    
    // Télémesure UI
    std::atomic<float> currentGainReductionDb { 0.0f };

    void updateCoefficients();
};