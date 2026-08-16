#include "ColorEQModule.h"

ColorEQModule::ColorEQModule()
{
    bands[0] = { 80.0f, 0.0f, 0.7071f, true };   // Low Shelf
    bands[1] = { 250.0f, 0.0f, 1.0f, true };     // Warmth
    bands[2] = { 1500.0f, 0.0f, 1.0f, true };    // Body
    bands[3] = { 3500.0f, 0.0f, 1.0f, true };    // Presence
    bands[4] = { 8000.0f, 0.0f, 1.0f, true };    // Brilliance
    bands[5] = { 12000.0f, 0.0f, 0.7071f, true };// High Shelf (Air)
}

void ColorEQModule::prepare(const juce::dsp::ProcessSpec& spec)
{
    sampleRate = spec.sampleRate;
    for (size_t i = 0; i < numBands; ++i) { filters[i].prepare(spec); updateBand(i); }
    reset();
}

void ColorEQModule::reset() { for (auto& f : filters) f.reset(); }

void ColorEQModule::updateBand(size_t i)
{
    if (sampleRate <= 0.0) return;
    const auto g = juce::Decibels::decibelsToGain(bands[i].gainDb);
    juce::dsp::IIR::Coefficients<float>::Ptr c;

    if (i == 0) c = juce::dsp::IIR::Coefficients<float>::makeLowShelf(sampleRate, bands[i].freq, bands[i].Q, g);
    else if (i == 5) c = juce::dsp::IIR::Coefficients<float>::makeHighShelf(sampleRate, bands[i].freq, bands[i].Q, g);
    else c = juce::dsp::IIR::Coefficients<float>::makePeakFilter(sampleRate, bands[i].freq, bands[i].Q, g);

    if (c) *filters[i].coefficients = *c;
}

void ColorEQModule::process(juce::dsp::AudioBlock<float>& block)
{
    if (bypassed) return;
    juce::dsp::ProcessContextReplacing<float> ctx(block);
    for (size_t i = 0; i < numBands; ++i)
        if (bands[i].enabled) filters[i].process(ctx);
}