#pragma once
#include <JuceHeader.h>
#include "../Common/IProcessModule.h"

enum class PreampType
{
    Clean = 0, // Transparente, légère compression harmonique douce
    Tube,      // Triode à lampe (harmoniques paires / chaleur)
    Transistor // Solid State (harmoniques impaires / tranchant)
};

class PreampModule : public IProcessModule
{
public:
    PreampModule();
    ~PreampModule() override = default;

    void prepare(const juce::dsp::ProcessSpec& spec) override;
    void reset() override;
    void process(juce::dsp::AudioBlock<float>& block) override;
    void setBypass(bool shouldBypass) override { bypassed = shouldBypass; }
    bool isBypassed() const override { return bypassed; }
    juce::String getName() const override { return "Preamp Simulation"; }

    /** Régle le niveau de Drive (1.0f = neutre/clean, jusqu'à 10.0f = forte saturation) */
    void setDrive(float newDrive);

    /** Régle le type de préampli (Clean, Tube, Transistor) */
    void setPreampType(PreampType newType);

    /** Active/désactive le suréchantillonnage anti-aliasing (2x) */
    void setOversamplingEnabled(bool enable);

private:
    float processSample(float sample);
    
    bool bypassed { false };
    double sampleRate { 44100.0 };

    PreampType currentType { PreampType::Tube };
    juce::SmoothedValue<float> drive { 1.0f };

    // DC Blocker IIR State pour le mode Tube
    std::array<float, 2> x1 { 0.0f, 0.0f };
    std::array<float, 2> y1 { 0.0f, 0.0f };
    static constexpr float R_dc = 0.995f;

    // Suréchantillonnage 2x pour éliminer l'aliasing
    std::unique_ptr<juce::dsp::Oversampling<float>> oversampling;
    bool oversamplingEnabled { true };
};