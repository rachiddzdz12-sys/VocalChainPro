#include "ReverbDelayModule.h"

ReverbDelayModule::ReverbDelayModule()
{
    rParams.roomSize = 0.3f;
    rParams.damping = 0.5f;
    rParams.wetLevel = 0.15f;
    rParams.dryLevel = 1.0f;
    reverb.setParameters(rParams);
}

void ReverbDelayModule::prepare(const juce::dsp::ProcessSpec& spec)
{
    sampleRate = spec.sampleRate;
    reverb.prepare(spec);

    delayBuffer.setSize(static_cast<int>(spec.numChannels), static_cast<int>(sampleRate * 2.0)); // 2 sec max
    delayBuffer.clear();
    setDelayTimeMs(120.0f); // Slap-back par défaut
}

void ReverbDelayModule::reset()
{
    reverb.reset();
    delayBuffer.clear();
    delayWritePosition = 0;
}

void ReverbDelayModule::setDelayTimeMs(float ms)
{
    delaySampleLength = static_cast<int>((ms * 0.001f) * sampleRate);
}

void ReverbDelayModule::process(juce::dsp::AudioBlock<float>& block)
{
    if (bypassed) return;

    // 1. Process Delay
    const int numChannels = static_cast<int>(block.getNumChannels());
    const int numSamples = static_cast<int>(block.getNumSamples());
    const int delayBufLen = delayBuffer.getNumSamples();

    for (int i = 0; i < numSamples; ++i)
    {
        int readPos = delayWritePosition - delaySampleLength;
        if (readPos < 0) readPos += delayBufLen;

        for (int ch = 0; ch < numChannels; ++ch)
        {
            const float in = block.getChannelPointer(ch)[i];
            const float delayedSample = delayBuffer.getSample(ch, readPos);

            delayBuffer.setSample(ch, delayWritePosition, in + delayedSample * 0.25f); // 25% feedback
            block.getChannelPointer(ch)[i] = in + (delayedSample * delayMix);
        }

        if (++delayWritePosition >= delayBufLen) delayWritePosition = 0;
    }

    // 2. Process Reverb
    juce::dsp::ProcessContextReplacing<float> ctx(block);
    reverb.process(ctx);
}