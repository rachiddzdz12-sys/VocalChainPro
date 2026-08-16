#include "GainStageModule.h"

GainStageModule::GainStageModule()
{
    smoothedGain.setCurrentAndTargetValue(1.0f);
}

void GainStageModule::prepare(const juce::dsp::ProcessSpec& spec)
{
    sampleRate = spec.sampleRate;
    
    // Temps de lissage de 20ms pour éviter tout zipper noise sur le gain
    smoothedGain.reset(sampleRate, 0.02);

    // Calcul du coefficient alpha pour la ballistique RMS (IIR lowpass)
    alphaRms = std::exp(-1.0f / static_cast<float>(sampleRate * rmsIntegrationTimeSec));

    reset();
}

void GainStageModule::reset()
{
    smoothedGain.setCurrentAndTargetValue(smoothedGain.getTargetValue());
    peakLevels.fill(0.0f);
    rmsLevels.fill(0.0f);
}

void GainStageModule::setGainDb(float gainInDb)
{
    // Conversion dB -> Linéaire
    const float gainLinear = juce::Decibels::decibelsToGain(gainInDb, -100.0f);
    smoothedGain.setTargetValue(gainLinear);
}

void GainStageModule::setPhaseInvert(bool shouldInvert)
{
    phaseInverted = shouldInvert;
}

float GainStageModule::getPeakLevel(int channel) const
{
    jassert(channel >= 0 && channel < maxChannels);
    return peakLevels[static_cast<size_t>(channel)];
}

float GainStageModule::getRmsLevel(int channel) const
{
    jassert(channel >= 0 && channel < maxChannels);
    return std::sqrt(rmsLevels[static_cast<size_t>(channel)]);
}

void GainStageModule::process(juce::dsp::AudioBlock<float>& block)
{
    const size_t numChannels = block.getNumChannels();
    const size_t numSamples = block.getNumSamples();

    // Réinitialisation légère des valeurs crêtes pour la capture du bloc courant
    for (size_t ch = 0; ch < numChannels && ch < maxChannels; ++ch)
    {
        peakLevels[ch] = 0.0f;
    }

    if (bypassed)
    {
        // En cas de bypass, on calcule quand même le métering du signal traversant
        for (size_t ch = 0; ch < numChannels && ch < maxChannels; ++ch)
        {
            auto* channelData = block.getChannelPointer(ch);
            for (size_t i = 0; i < numSamples; ++i)
            {
                const float sample = channelData[i];
                const float absSample = std::abs(sample);
                const float sampleSq = sample * sample;

                // Capture Peak
                if (absSample > peakLevels[ch])
                    peakLevels[ch] = absSample;

                // Lissage RMS
                rmsLevels[ch] = alphaRms * rmsLevels[ch] + (1.0f - alphaRms) * sampleSq;
            }
        }
        return;
    }

    // Traitement DSP actif
    const float phaseFactor = phaseInverted ? -1.0f : 1.0f;

    for (size_t i = 0; i < numSamples; ++i)
    {
        // Gain lissé par échantillon pour éviter les craquements
        const float currentGain = smoothedGain.getNextValue() * phaseFactor;

        for (size_t ch = 0; ch < numChannels && ch < maxChannels; ++ch)
        {
            float* channelData = block.getChannelPointer(ch);
            
            // Application du Gain + Inversion de phase
            channelData[i] *= currentGain;

            const float sample = channelData[i];
            const float absSample = std::abs(sample);
            const float sampleSq = sample * sample;

            // Détection Crête
            if (absSample > peakLevels[ch])
                peakLevels[ch] = absSample;

            // Intégration RMS (Ballistique)
            rmsLevels[ch] = alphaRms * rmsLevels[ch] + (1.0f - alphaRms) * sampleSq;
        }
    }
}