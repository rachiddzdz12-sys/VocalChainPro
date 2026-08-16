#include "CorrectiveEQModule.h"

CorrectiveEQModule::CorrectiveEQModule()
{
    // Configuration par défaut de l'EQ de nettoyage pré-compression
    // Bande 1 : Low Shelf à 120 Hz
    bandConfigs[0] = { FilterType::LowShelf, 120.0f, 0.0f, 0.7071f, true };
    // Bande 2 : Bell bas-médium (pour résonances boitier / room)
    bandConfigs[1] = { FilterType::Bell, 400.0f, 0.0f, 1.5f, true };
    // Bande 3 : Bell haut-médium (pour résonances agressives / nasillardes)
    bandConfigs[2] = { FilterType::Bell, 2500.0f, 0.0f, 2.0f, true };
    // Bande 4 : High Shelf à 8000 Hz
    bandConfigs[3] = { FilterType::HighShelf, 8000.0f, 0.0f, 0.7071f, true };
}

void CorrectiveEQModule::prepare(const juce::dsp::ProcessSpec& spec)
{
    sampleRate = spec.sampleRate;

    for (size_t i = 0; i < numBands; ++i)
    {
        filters[i].prepare(spec);
        updateBandCoefficients(i);
    }

    reset();
}

void CorrectiveEQModule::reset()
{
    for (size_t i = 0; i < numBands; ++i)
    {
        filters[i].reset();
    }
}

void CorrectiveEQModule::setBandParameters(size_t bandIndex, float freqHz, float gainDb, float Q)
{
    jassert(bandIndex < numBands);

    bandConfigs[bandIndex].frequencyHz = juce::jlimit(20.0f, 20000.0f, freqHz);
    bandConfigs[bandIndex].gainDb = juce::jlimit(-24.0f, 24.0f, gainDb);
    bandConfigs[bandIndex].Q = juce::jlimit(0.1f, 10.0f, Q);

    updateBandCoefficients(bandIndex);
}

void CorrectiveEQModule::setBandEnabled(size_t bandIndex, bool enabled)
{
    jassert(bandIndex < numBands);
    bandConfigs[bandIndex].enabled = enabled;
}

void CorrectiveEQModule::updateBandCoefficients(size_t bandIndex)
{
    if (sampleRate <= 0.0)
        return;

    const auto& config = bandConfigs[bandIndex];
    juce::dsp::IIR::Coefficients<float>::Ptr newCoeffs;

    switch (config.type)
    {
        case FilterType::LowShelf:
            newCoeffs = juce::dsp::IIR::Coefficients<float>::makeLowShelf(
                sampleRate, config.frequencyHz, config.Q, juce::Decibels::decibelsToGain(config.gainDb));
            break;

        case FilterType::Bell:
            newCoeffs = juce::dsp::IIR::Coefficients<float>::makePeakFilter(
                sampleRate, config.frequencyHz, config.Q, juce::Decibels::decibelsToGain(config.gainDb));
            break;

        case FilterType::HighShelf:
            newCoeffs = juce::dsp::IIR::Coefficients<float>::makeHighShelf(
                sampleRate, config.frequencyHz, config.Q, juce::Decibels::decibelsToGain(config.gainDb));
            break;
    }

    if (newCoeffs)
    {
        *filters[bandIndex].coefficients = *newCoeffs;
    }
}

void CorrectiveEQModule::process(juce::dsp::AudioBlock<float>& block)
{
    if (bypassed)
        return;

    juce::dsp::ProcessContextReplacing<float> context(block);

    // Traitement en cascade à travers les 4 bandes d'EQ
    for (size_t i = 0; i < numBands; ++i)
    {
        if (bandConfigs[i].enabled)
        {
            filters[i].process(context);
        }
    }
}