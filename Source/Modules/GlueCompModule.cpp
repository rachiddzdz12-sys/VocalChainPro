#include "GlueCompModule.h"

GlueCompModule::GlueCompModule() {}

void GlueCompModule::prepare(const juce::dsp::ProcessSpec& spec)
{
    sampleRate = spec.sampleRate;
    updateCoeffs();
    reset();
}

void GlueCompModule::reset() { envDb = 0.0f; currentGR.store(0.0f); }

void GlueCompModule::updateCoeffs()
{
    if (sampleRate <= 0.0) return;
    attAlpha = std::exp(-1.0f / static_cast<float>(sampleRate * (attackMs * 0.001f)));
    relAlpha = std::exp(-1.0f / static_cast<float>(sampleRate * (releaseMs * 0.001f)));
}

void GlueCompModule::process(juce::dsp::AudioBlock<float>& block)
{
    if (bypassed) { currentGR.store(0.0f); return; }

    const size_t numChannels = block.getNumChannels();
    const size_t numSamples = block.getNumSamples();
    float maxGR = 0.0f;

    for (size_t i = 0; i < numSamples; ++i)
    {
        float peak = 0.0f;
        for (size_t ch = 0; ch < numChannels; ++ch)
            peak = std::max(peak, std::abs(block.getChannelPointer(ch)[i]));

        const float inDb = juce::Decibels::gainToDecibels(peak, -100.0f);
        const float targetGR = (inDb > thresholdDb) ? (inDb - thresholdDb) * (1.0f - (1.0f / ratio)) : 0.0f;

        if (targetGR > envDb) envDb = attAlpha * envDb + (1.0f - attAlpha) * targetGR;
        else envDb = relAlpha * envDb + (1.0f - relAlpha) * targetGR;

        const float gLin = juce::Decibels::decibelsToGain(-envDb);
        for (size_t ch = 0; ch < numChannels; ++ch) block.getChannelPointer(ch)[i] *= gLin;

        maxGR = std::max(maxGR, envDb);
    }
    currentGR.store(maxGR);
}