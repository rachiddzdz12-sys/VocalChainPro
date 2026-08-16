#include "HPFModule.h"

HPFModule::HPFModule()
{
}

void HPFModule::prepare(const juce::dsp::ProcessSpec& spec)
{
    sampleRate = spec.sampleRate;

    filterCascade1.prepare(spec);
    filterCascade2.prepare(spec);

    updateFilters();
    reset();
}

void HPFModule::reset()
{
    filterCascade1.reset();
    filterCascade2.reset();
}

void HPFModule::setCutoffFrequency(float newFrequencyHz)
{
    const float clampedFreq = juce::jlimit(20.0f, 300.0f, newFrequencyHz);
    
    if (std::abs(cutoffHz - clampedFreq) > 0.01f)
    {
        cutoffHz = clampedFreq;
        updateFilters();
    }
}

void HPFModule::setSlope(HPFSlope newSlope)
{
    if (currentSlope != newSlope)
    {
        currentSlope = newSlope;
        updateFilters();
    }
}

void HPFModule::updateFilters()
{
    if (sampleRate <= 0.0)
        return;

    if (currentSlope == HPFSlope::Slope12dB)
    {
        // Butterworth standard Q = 1/sqrt(2) = 0.7071
        auto coefficients = juce::dsp::IIR::Coefficients<float>::makeHighPass(sampleRate, cutoffHz, 0.70710678f);
        *filterCascade1.coefficients = *coefficients;
    }
    else // Slope24dB
    {
        // Filtre 4ème ordre composé de 2 Biquads en cascade avec facteurs Q de Butterworth :
        // Q1 = 0.5411961, Q2 = 1.3065630
        auto coeffs1 = juce::dsp::IIR::Coefficients<float>::makeHighPass(sampleRate, cutoffHz, 0.5411961f);
        auto coeffs2 = juce::dsp::IIR::Coefficients<float>::makeHighPass(sampleRate, cutoffHz, 1.3065630f);

        *filterCascade1.coefficients = *coeffs1;
        *filterCascade2.coefficients = *coeffs2;
    }
}

void HPFModule::process(juce::dsp::AudioBlock<float>& block)
{
    if (bypassed)
        return;

    juce::dsp::ProcessContextReplacing<float> context(block);

    // Première section (12 dB/oct)
    filterCascade1.process(context);

    // Deuxième section appliquée en cascade si 24 dB/oct
    if (currentSlope == HPFSlope::Slope24dB)
    {
        filterCascade2.process(context);
    }
}