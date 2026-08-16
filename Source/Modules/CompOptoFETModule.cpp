#include "CompOptoFETModule.h"

CompOptoFETModule::CompOptoFETModule()
{
}

void CompOptoFETModule::prepare(const juce::dsp::ProcessSpec& spec)
{
    sampleRate = spec.sampleRate;
    updateBallistics();
    reset();
}

void CompOptoFETModule::reset()
{
    detectorEnvelope = 0.0f;
    grEnvelopeDb = 0.0f;
    currentGainReductionDb.store(0.0f);
}

void CompOptoFETModule::setThresholdDb(float newThresholdDb)
{
    thresholdDb = juce::jlimit(-60.0f, 0.0f, newThresholdDb);
}

void CompOptoFETModule::setRatio(float newRatio)
{
    ratio = juce::jlimit(1.0f, 20.0f, newRatio);
}

void CompOptoFETModule::setAttackMs(float newAttackMs)
{
    attackMs = juce::jlimit(0.02f, 200.0f, newAttackMs);
    updateBallistics();
}

void CompOptoFETModule::setReleaseMs(float newReleaseMs)
{
    releaseMs = juce::jlimit(10.0f, 1200.0f, newReleaseMs);
    updateBallistics();
}

void CompOptoFETModule::setKneeDb(float newKneeDb)
{
    kneeDb = juce::jlimit(0.0f, 24.0f, newKneeDb);
}

void CompOptoFETModule::setMakeupGainDb(float makeupDb)
{
    manualMakeupDb = juce::jlimit(-12.0f, 24.0f, makeupDb);
}

void CompOptoFETModule::setAutoMakeupEnabled(bool enableAutoMakeup)
{
    autoMakeup = enableAutoMakeup;
}

void CompOptoFETModule::setMode(CompMode newMode)
{
    mode = newMode;
    updateBallistics();
}

void CompOptoFETModule::updateBallistics()
{
    if (sampleRate <= 0.0)
        return;

    float effectiveAttackMs = attackMs;
    float effectiveReleaseMs = releaseMs;

    // Ajustement des constantes de temps selon la topologie analogique
    if (mode == CompMode::Opto)
    {
        effectiveAttackMs = 15.0f; // Attaque photo-cellule naturelle
        effectiveReleaseMs = 60.0f; // Étage rapide initial
        const float optoSlowReleaseMs = 1200.0f; // Étage lent (mémoire opto)
        optoReleaseSlowAlpha = std::exp(-1.0f / static_cast<float>(sampleRate * (optoSlowReleaseMs * 0.001f)));
    }
    else if (mode == CompMode::FET)
    {
        // Réponse ultrarapide type 1176 (échelle réajustée)
        effectiveAttackMs = juce::jmap(attackMs, 0.02f, 200.0f, 0.05f, 2.0f);
    }

    attackAlpha = std::exp(-1.0f / static_cast<float>(sampleRate * (effectiveAttackMs * 0.001f)));
    releaseAlpha = std::exp(-1.0f / static_cast<float>(sampleRate * (effectiveReleaseMs * 0.001f)));
}

float CompOptoFETModule::computeGainReductionDb(float detectorDb)
{
    if (detectorDb <= (thresholdDb - kneeDb * 0.5f))
    {
        return 0.0f;
    }
    else if (detectorDb > (thresholdDb + kneeDb * 0.5f))
    {
        return (detectorDb - thresholdDb) * (1.0f - (1.0f / ratio));
    }
    else
    {
        // Zone Soft Knee (Interpolation quadratique)
        const float delta = detectorDb - thresholdDb + (kneeDb * 0.5f);
        return (delta * delta) / (2.0f * kneeDb) * (1.0f - (1.0f / ratio));
    }
}

void CompOptoFETModule::process(juce::dsp::AudioBlock<float>& block)
{
    if (bypassed)
    {
        currentGainReductionDb.store(0.0f);
        return;
    }

    const size_t numChannels = block.getNumChannels();
    const size_t numSamples = block.getNumSamples();

    // Calcul du Makeup Gain
    float totalMakeupDb = manualMakeupDb;
    if (autoMakeup)
    {
        totalMakeupDb += ((0.0f - thresholdDb) * 0.5f) * (1.0f - (1.0f / ratio));
    }
    const float makeupGainLin = juce::Decibels::decibelsToGain(totalMakeupDb);

    float maxGRThisBlock = 0.0f;

    for (size_t i = 0; i < numSamples; ++i)
    {
        // 1. Peak Detection Max sur tous les canaux
        float inputPeak = 0.0f;
        for (size_t ch = 0; ch < numChannels; ++ch)
        {
            inputPeak = std::max(inputPeak, std::abs(block.getChannelPointer(ch)[i]));
        }

        // Conversion en dB
        const float inputDb = juce::Decibels::gainToDecibels(inputPeak, -100.0f);

        // 2. Calcul de la réduction de gain cible (dB)
        const float targetGRDb = computeGainReductionDb(inputDb);

        // 3. Application des balistiques de réduction de gain
        if (targetGRDb > grEnvelopeDb)
        {
            // Attaque
            grEnvelopeDb = attackAlpha * grEnvelopeDb + (1.0f - attackAlpha) * targetGRDb;
        }
        else
        {
            // Release (Gestion spécifique Opto vs FET/VCA)
            if (mode == CompMode::Opto)
            {
                // Double-stage release : 50% rapide, 50% très lent
                if (grEnvelopeDb > (targetGRDb + 3.0f))
                    grEnvelopeDb = releaseAlpha * grEnvelopeDb + (1.0f - releaseAlpha) * targetGRDb;
                else
                    grEnvelopeDb = optoReleaseSlowAlpha * grEnvelopeDb + (1.0f - optoReleaseSlowAlpha) * targetGRDb;
            }
            else
            {
                grEnvelopeDb = releaseAlpha * grEnvelopeDb + (1.0f - releaseAlpha) * targetGRDb;
            }
        }

        // 4. Application du gain linéaire
        const float currentGainLin = juce::Decibels::decibelsToGain(-grEnvelopeDb) * makeupGainLin;

        for (size_t ch = 0; ch < numChannels; ++ch)
        {
            block.getChannelPointer(ch)[i] *= currentGainLin;
        }

        if (grEnvelopeDb > maxGRThisBlock)
            maxGRThisBlock = grEnvelopeDb;
    }

    currentGainReductionDb.store(maxGRThisBlock);
}