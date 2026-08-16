#include "ExciterModule.h"

void ExciterModule::process(juce::dsp::AudioBlock<float>& block)
{
    if (bypassed) return;

    const size_t numChannels = block.getNumChannels();
    const size_t numSamples = block.getNumSamples();

    for (size_t ch = 0; ch < numChannels; ++ch)
    {
        float* data = block.getChannelPointer(ch);
        for (size_t i = 0; i < numSamples; ++i)
        {
            const float dry = data[i];
            const float even = dry * dry * std::copysign(1.0f, dry) * evenAmount;
            const float odd = std::tanh(dry * 2.0f) * oddAmount;
            const float wet = dry + even + odd;

            data[i] = (1.0f - mix) * dry + mix * wet;
        }
    }
}