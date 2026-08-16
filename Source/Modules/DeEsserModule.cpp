#include "DeEsserModule.h"

DeEsserModule::DeEsserModule() {}

void DeEsserModule::prepare(const juce::dsp::ProcessSpec& spec)
{
    sampleRate = spec.sampleRate;
    bandpassFilter.prepare(spec);
    highpassCrossover.prepare(spec);
    lowpassCrossover.prepare(spec);

    attackAlpha = std::exp(-1.0f / static_cast<float>(sampleRate * 0.001f)); // 1ms
    releaseAlpha = std::exp(-1.0f / static_cast<float>(sampleRate * 0.050f)); // 50ms

    updateFilters();
    reset();
}

void DeEsserModule::reset()
{
    bandpassFilter.reset();
    highpassCrossover.reset();
    lowpassCrossover.reset();
    envelope = 0.0f;
    currentGR.store(0.0f);
}

void DeEsserModule::updateFilters()
{
    if (sampleRate <= 0.0) return;
    *bandpassFilter.coefficients = *juce::dsp::IIR::Coefficients<float>::makeBandPass(sampleRate, frequencyHz, 2.0f);
    *highpassCrossover.coefficients = *juce::dsp::IIR::Coefficients<float>::makeHighPass(sampleRate, frequencyHz);
    *lowpassCrossover.coefficients = *juce::dsp::IIR::Coefficients<float>::makeLowPass(sampleRate, frequencyHz);
}

void DeEsserModule::process(juce::dsp::AudioBlock<float>& block)
{
    if (bypassed) { currentGR.store(0.0f); return; }

    const size_t numChannels = block.getNumChannels();
    const size_t numSamples = block.getNumSamples();

    // Buffer temporaire de détection
    juce::AudioBuffer<float> sidechainBuffer(static_cast<int>(numChannels), static_cast<int>(numSamples));
    juce::dsp::AudioBlock<float> sidechainBlock(sidechainBuffer);
    sidechainBlock.copyFrom(block);

    juce::dsp::ProcessContextReplacing<float> scContext(sidechainBlock);
    bandpassFilter.process(scContext);

    float maxGRThisBlock = 0.0f;

    for (size_t i = 0; i < numSamples; ++i)
    {
        float scPeak = 0.0f;
        for (size_t ch = 0; ch < numChannels; ++ch)
            scPeak = std::max(scPeak, std::abs(sidechainBlock.getChannelPointer(ch)[i]));

        if (scPeak > envelope) envelope = attackAlpha * envelope + (1.0f - attackAlpha) * scPeak;
        else envelope = releaseAlpha * envelope + (1.0f - releaseAlpha) * scPeak;

        const float envDb = juce::Decibels::gainToDecibels(envelope, -100.0f);
        float grDb = 0.0f;

        if (envDb > thresholdDb)
        {
            grDb = (envDb - thresholdDb) * 0.75f;
            grDb = std::min(grDb, maxReductionDb);
        }

        const float gainLin = juce::Decibels::decibelsToGain(-grDb);

        for (size_t ch = 0; ch < numChannels; ++ch)
        {
            if (mode == DeEsserMode::Wideband)
            {
                block.getChannelPointer(ch)[i] *= gainLin;
            }
            else // Split-Band
            {
                const float in = block.getChannelPointer(ch)[i];
                const float highPass = highpassCrossover.processSample(in);
                const float lowPass = in - highPass;
                block.getChannelPointer(ch)[i] = lowPass + (highPass * gainLin);
            }
        }
        maxGRThisBlock = std::max(maxGRThisBlock, grDb);
    }
    currentGR.store(maxGRThisBlock);
}