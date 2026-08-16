#include "GateModule.h"

GateModule::GateModule()
{
}

void GateModule::prepare(const juce::dsp::ProcessSpec& spec)
{
    sampleRate = spec.sampleRate;
    updateCoefficients();
    reset();
}

void GateModule::reset()
{
    envelope = 0.0f;
    currentGain = 0.0f;
    currentState = GateState::Closed;
    holdCounter = 0;
    currentGainReductionDb.store(0.0f);
}

void GateModule::setThresholdDb(float newThresholdDb)
{
    thresholdDb = juce::jlimit(-80.0f, 0.0f, newThresholdDb);
}

void GateModule::setRatio(float newRatio)
{
    ratio = juce::jlimit(1.0f, 20.0f, newRatio);
}

void GateModule::setAttackMs(float newAttackMs)
{
    attackTimeMs = juce::jlimit(0.1f, 100.0f, newAttackMs);
    updateCoefficients();
}

void GateModule::setReleaseMs(float newReleaseMs)
{
    releaseTimeMs = juce::jlimit(10.0f, 2000.0f, newReleaseMs);
    updateCoefficients();
}

void GateModule::setHoldMs(float newHoldMs)
{
    holdTimeMs = juce::jlimit(0.0f, 1000.0f, newHoldMs);
    holdSamples = static_cast<int>((holdTimeMs * 0.001f) * sampleRate);
}

void GateModule::updateCoefficients()
{
    if (sampleRate <= 0.0)
        return;

    attackAlpha = std::exp(-1.0f / static_cast<float>(sampleRate * (attackTimeMs * 0.001f)));
    releaseAlpha = std::exp(-1.0f / static_cast<float>(sampleRate * (releaseTimeMs * 0.001f)));
    holdSamples = static_cast<int>((holdTimeMs * 0.001f) * sampleRate);
}

void GateModule::process(juce::dsp::AudioBlock<float>& block)
{
    const size_t numChannels = block.getNumChannels();
    const size_t numSamples = block.getNumSamples();

    if (bypassed)
    {
        currentGainReductionDb.store(0.0f);
        return;
    }

    float maxGRThisBlock = 0.0f;

    for (size_t i = 0; i < numSamples; ++i)
    {
        // 1. Calcul de l'amplitude absolue max du signal entrant
        float inputPeak = 0.0f;
        for (size_t ch = 0; ch < numChannels; ++ch)
        {
            inputPeak = std::max(inputPeak, std::abs(block.getChannelPointer(ch)[i]));
        }

        // 2. Détection d'enveloppe avec balistique Attack / Release
        if (inputPeak > envelope)
            envelope = attackAlpha * envelope + (1.0f - attackAlpha) * inputPeak;
        else
            envelope = releaseAlpha * envelope + (1.0f - releaseAlpha) * inputPeak;

        // Conversion en dB avec floor de sécurité à -100dB
        const float envDb = juce::Decibels::gainToDecibels(envelope, -100.0f);

        // 3. Machine à états (State Machine)
        if (envDb >= thresholdDb)
        {
            currentState = GateState::Open;
            holdCounter = holdSamples;
        }
        else
        {
            if (currentState == GateState::Open || currentState == GateState::Hold)
            {
                if (holdCounter > 0)
                {
                    currentState = GateState::Hold;
                    --holdCounter;
                }
                else
                {
                    currentState = GateState::Release;
                }
            }
        }

        // 4. Calcul de la courbe de gain (Expander / Gate)
        float targetGainDb = 0.0f;

        if (envDb < thresholdDb && currentState != GateState::Hold)
        {
            // Expansion vers le bas sous le threshold
            const float overThresholdDb = envDb - thresholdDb; // Négatif
            targetGainDb = overThresholdDb * (1.0f - (1.0f / ratio));
        }

        const float targetGainLin = juce::Decibels::decibelsToGain(targetGainDb);

        // Lissage du gain appliqué
        currentGain = 0.95f * currentGain + 0.05f * targetGainLin;

        // 5. Application du gain sur tous les canaux
        for (size_t ch = 0; ch < numChannels; ++ch)
        {
            block.getChannelPointer(ch)[i] *= currentGain;
        }

        // Mesure de la Gain Reduction (GR)
        const float grDb = -juce::Decibels::gainToDecibels(currentGain);
        if (grDb > maxGRThisBlock)
            maxGRThisBlock = grDb;
    }

    currentGainReductionDb.store(maxGRThisBlock);
}