#include "OutputStageModule.h"

OutputStageModule::OutputStageModule() {}

void OutputStageModule::prepare(const juce::dsp::ProcessSpec& spec)
{
    sampleRate = spec.sampleRate;
    volume.reset(sampleRate, 0.02);

    alphaRms = std::exp(-1.0f / static_cast<float>(sampleRate * 0.3f));

    // Coefficients K-Weighting Standard ITU-R BS.1770
    *kWeightHighShelf.coefficients = *juce::dsp::IIR::Coefficients<float>::makeHighShelf(sampleRate, 1500.0f, 0.7071f, juce::Decibels::decibelsToGain(4.0f));
    *kWeightHighPass.coefficients = *juce::dsp::IIR::Coefficients<float>::makeHighPass(sampleRate, 38.0f);

    reset();
}

void OutputStageModule::reset()
{
    volume.snapToTargetValue();
    peakLevels.fill(0.0f);
    rmsLevels.fill(0.0f);
    kWeightHighShelf.reset();
    kWeightHighPass.reset();
}

void OutputStageModule::process(juce::dsp::AudioBlock<float>& block)
{
    const size_t numChannels = block.getNumChannels();
    const size_t numSamples = block.getNumSamples();

    peakLevels.fill(0.0f);
    float kWeightedSum = 0.0f;

    for (size_t i = 0; i < numSamples; ++i)
    {
        const float g = volume.getNextValue();

        for (size_t ch = 0; ch < numChannels && ch < 2; ++ch)
        {
            if (!bypassed) block.getChannelPointer(ch)[i] *= g;

            const float sample = block.getChannelPointer(ch)[i];
            const float absS = std::abs(sample);

            if (absS > peakLevels[ch]) peakLevels[ch] = absS;
            rmsLevels[ch] = alphaRms * rmsLevels[ch] + (1.0f - alphaRms) * (sample * sample);

            // Filtrage K-Weighting approximatif pour LUFS
            float kSample = kWeightHighPass.processSample(kWeightHighShelf.processSample(sample));
            kWeightedSum += kSample * kSample;
        }
    }

    const float meanSquare = kWeightedSum / static_cast<float>(numSamples * numChannels);
    const float lufs = -0.691f + 10.0f * std::log10(meanSquare + 1e-10f);
    lufsMomentary.store(lufs);
}