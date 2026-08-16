#pragma once
#include <JuceHeader.h>

class IProcessModule
{
public:
    virtual ~IProcessModule() = default;

    /** Initialise le module avec les paramètres audio de la DAW */
    virtual void prepare(const juce::dsp::ProcessSpec& spec) = 0;

    /** Reinitialise l'état interne (buffers, filtres, méteurs) */
    virtual void reset() = 0;

    /** Traitement audio temps réel */
    virtual void process(juce::dsp::AudioBlock<float>& block) = 0;

    /** Active/Désactive le bypass du module */
    virtual void setBypass(bool shouldBypass) = 0;
    
    /** Indique si le module est en bypass */
    virtual bool isBypassed() const = 0;

    /** Identifiant unique du module */
    virtual juce::String getName() const = 0;
};