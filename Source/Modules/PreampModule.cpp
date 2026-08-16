#include "PreampModule.h"

PreampModule::PreampModule()
{
    drive.setCurrentAndTargetValue(1.0f);
}

void PreampModule::prepare(const juce::dsp::ProcessSpec& spec)
{
    sampleRate = spec.sampleRate;
    drive.reset(sampleRate, 0.02); // 20ms lissage

    // Initialisation du suréchantillonnage 2x (filtrage polyphasé IIR)
    oversampling = std::make_unique<juce::dsp::Oversampling<float>>(
        spec.numChannels,
        1, // Factor 2x (2^1)
        juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR,
        true
    );

    oversampling->initProcessing(spec.maximumBlockSize);
    reset();
}

void PreampModule::reset()
{
    drive.setCurrentAndTargetValue(drive.getTargetValue());
    x1.fill(0.0f);
    y1.fill(0.0f);

    if (oversampling)
        oversampling->reset();
}

void PreampModule::setDrive(float newDrive)
{
    // Limites de sécurité du Drive : 1.0 à 10.0
    const float clampedDrive = juce::jlimit(1.0f, 10.0f, newDrive);
    drive.setTargetValue(clampedDrive);
}

void PreampModule::setPreampType(PreampType newType)
{
    currentType = newType;
}

void PreampModule::setOversamplingEnabled(bool enable)
{
    oversamplingEnabled = enable;
}

float PreampModule::processSample(float sample)
{
    const float currentDrive = drive.getNextValue();
    const float autoGainMakeup = 1.0f / std::sqrt(currentDrive);

    switch (currentType)
    {
        case PreampType::Clean:
        {
            // Saturation ultra-douce (presque linéaire)
            const float drivenSignal = sample * currentDrive;
            return std::tanh(drivenSignal) * autoGainMakeup;
        }

        case PreampType::Tube:
        {
            // Asymétrie (DC Bias) pour générer des harmoniques paires
            const float dcBias = 0.15f * (currentDrive - 1.0f) / 9.0f;
            const float drivenSignal = (sample * currentDrive) + dcBias;

            // Fonction de transfert asymétrique
            float saturated = drivenSignal / (1.0f + std::abs(drivenSignal));

            return saturated * autoGainMakeup;
        }

        case PreampType::Transistor:
        {
            // Symmetrical Soft Clipping (Harmoniques impaires tranchantes)
            const float drivenSignal = sample * currentDrive;
            const float saturated = std::tanh(drivenSignal);

            return saturated * autoGainMakeup;
        }
    }

    return sample;
}

void PreampModule::process(juce::dsp::AudioBlock<float>& block)
{
    if (bypassed)
        return;

    // Traitement avec ou sans Oversampling
    if (oversamplingEnabled && oversampling)
    {
        // Up-sampling (2x)
        juce::dsp::AudioBlock<float> oversampledBlock = oversampling->processSamplesUp(block);

        const size_t numChannels = oversampledBlock.getNumChannels();
        const size_t numSamples = oversampledBlock.getNumSamples();

        for (size_t ch = 0; ch < numChannels; ++ch)
        {
            float* channelData = oversampledBlock.getChannelPointer(ch);
            const size_t safeCh = std::min(ch, size_t(1));

            for (size_t i = 0; i < numSamples; ++i)
            {
                float processed = processSample(channelData[i]);

                // DC Blocker pour éliminer l'offset continu (nécessaire en mode Tube)
                if (currentType == PreampType::Tube)
                {
                    float temp = processed;
                    processed = processed - x1[safeCh] + R_dc * y1[safeCh];
                    x1[safeCh] = temp;
                    y1[safeCh] = processed;
                }

                channelData[i] = processed;
            }
        }

        // Down-sampling vers le Sample Rate original avec filtre anti-aliasing
        oversampling->processSamplesDown(block);
    }
    else
    {
        // Traitement direct au sample rate hôte
        const size_t numChannels = block.getNumChannels();
        const size_t numSamples = block.getNumSamples();

        for (size_t ch = 0; ch < numChannels; ++ch)
        {
            float* channelData = block.getChannelPointer(ch);
            const size_t safeCh = std::min(ch, size_t(1));

            for (size_t i = 0; i < numSamples; ++i)
            {
                float processed = processSample(channelData[i]);

                if (currentType == PreampType::Tube)
                {
                    float temp = processed;
                    processed = processed - x1[safeCh] + R_dc * y1[safeCh];
                    x1[safeCh] = temp;
                    y1[safeCh] = processed;
                }

                channelData[i] = processed;
            }
        }
    }
}