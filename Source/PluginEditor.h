#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

// ==============================================================================
// Composant réutilisable pour afficher un VU-Mètre (Level / Gain Reduction)
// ==============================================================================
class MeterComponent : public juce::Component
{
public:
    enum class Type { PeakLevel, GainReduction };

    MeterComponent(Type meterType) : type(meterType) {}

    void setLevel(float value)
    {
        currentValue = value;
        repaint();
    }

    void paint(juce::Graphics& g) override
    {
        auto bounds = getLocalBounds().toFloat();
        g.setColour(juce::Colours::black.withAlpha(0.6f));
        g.fillRect(bounds);

        if (type == Type::PeakLevel)
        {
            // Niveau en dB (-60 à 0 dB) converti en proportion [0, 1]
            float norm = juce::jmap(juce::jlimit(-60.0f, 0.0f, currentValue), -60.0f, 0.0f, 0.0f, 1.0f);
            auto fillWidth = bounds.getWidth() * norm;
            
            g.setColour(norm > 0.9f ? juce::Colours::red : juce::Colours::limegreen);
            g.fillRect(bounds.removeFromLeft(fillWidth));
        }
        else // Gain Reduction
        {
            // GR en dB (0 à -24 dB) converti en proportion [0, 1]
            float norm = juce::jmap(juce::jlimit(-24.0f, 0.0f, currentValue), 0.0f, -24.0f, 0.0f, 1.0f);
            auto fillWidth = bounds.getWidth() * norm;

            g.setColour(juce::Colours::orange);
            // La barre s'affiche de droite à gauche pour la réduction de gain
            g.fillRect(bounds.removeFromRight(fillWidth));
        }

        g.setColour(juce::Colours::darkgrey);
        g.drawRect(getLocalBounds());
    }

private:
    Type type;
    float currentValue = 0.0f;
};

// ==============================================================================
// Structure d'IHM pour un module unique de la chaîne
// ==============================================================================
struct ModuleUI
{
    juce::GroupComponent panel;
    juce::ToggleButton bypassButton { "Bypass" };
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> bypassAttachment;

    std::vector<std::unique_ptr<juce::Slider>> sliders;
    std::vector<std::unique_ptr<juce::Label>> labels;
    std::vector<std::unique_ptr<juce::ComboBox>> comboBoxes;
    
    std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>> sliderAttachments;
    std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment>> comboAttachments;

    std::unique_ptr<MeterComponent> peakMeter;
    std::unique_ptr<MeterComponent> grMeter;
};

// ==============================================================================
// PluginEditor Principal
// ==============================================================================
class VocalChainAudioProcessorEditor  : public juce::AudioProcessorEditor,
                                        private juce::Timer
{
public:
    VocalChainAudioProcessorEditor (VocalChainAudioProcessor&);
    ~VocalChainAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override; // Mise à jour des VU-mètres (~30 fps)
    
    void setupPresetsSection();
    void savePreset();
    void loadPreset();

    VocalChainAudioProcessor& audioProcessor;

    // Barre supérieure : Presets
    juce::GroupComponent presetGroup { "presets", "Presets" };
    juce::ComboBox presetSelector;
    juce::TextButton savePresetButton { "Save" };
    juce::TextButton loadPresetButton { "Load" };

    // Vue défilante horizontale pour aligner les 13 modules
    juce::Viewport viewport;
    juce::Component containerComponent;

    // Liste des modules GUI (dans l'ordre du signal flow)
    std::vector<std::unique_ptr<ModuleUI>> moduleUIs;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (VocalChainAudioProcessorEditor)
};
