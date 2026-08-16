#include "PluginProcessor.h"
#include "PluginEditor.h"

VocalChainAudioProcessor::VocalChainAudioProcessor()
    : AudioProcessor(BusesProperties().withInput("Input", juce::AudioChannelSet::stereo(), true)
                                      .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      apvts(*this, nullptr, "Parameters", createParameterLayout())
{
    // Instanciation des modules
    gainStage = std::make_unique<GainStageModule>();
    preamp = std::make_unique<PreampModule>();
    hpf = std::make_unique<HPFModule>();
    gate = std::make_unique<GateModule>();
    correctiveEQ = std::make_unique<CorrectiveEQModule>();
    compOptoFET = std::make_unique<CompOptoFETModule>();
    deEsser = std::make_unique<DeEsserModule>();
    colorEQ = std::make_unique<ColorEQModule>();
    glueComp = std::make_unique<GlueCompModule>();
    exciter = std::make_unique<ExciterModule>();
    reverbDelay = std::make_unique<ReverbDelayModule>();
    limiter = std::make_unique<LimiterModule>();
    outputStage = std::make_unique<OutputStageModule>();

    // Remplissage de la chaîne séquentielle
    dspChain = {
        gainStage.get(), preamp.get(), hpf.get(), gate.get(),
        correctiveEQ.get(), compOptoFET.get(), deEsser.get(),
        colorEQ.get(), glueComp.get(), exciter.get(),
        reverbDelay.get(), limiter.get(), outputStage.get()
    };
}

juce::AudioProcessorEditor* VocalChainAudioProcessor::createEditor()
{
    return new VocalChainAudioProcessorEditor(*this);
}

juce::AudioProcessorValueTreeState::ParameterLayout VocalChainAudioProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    // Helper Lambda pour la déclaration des paramètres
    auto addFloat = [&](const juce::String& id, const juce::String& name, float min, float max, float def, float step = 0.01f) {
        params.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{id, 1}, name, juce::NormalisableRange<float>(min, max, step), def));
    };
    auto addBool = [&](const juce::String& id, const juce::String& name, bool def) {
        params.push_back(std::make_unique<juce::AudioParameterBool>(juce::ParameterID{id, 1}, name, def));
    };
    auto addChoice = [&](const juce::String& id, const juce::String& name, const juce::StringArray& choices, int def) {
        params.push_back(std::make_unique<juce::AudioParameterChoice>(juce::ParameterID{id, 1}, name, choices, def));
    };

    // 1. GAIN STAGE
    addBool("gain_bypass", "Gain Stage Bypass", false);
    addFloat("gain_input", "Input Gain (dB)", -24.0f, 24.0f, 0.0f);
    addBool("gain_phase", "Phase Invert", false);

    // 2. PREAMP
    addBool("preamp_bypass", "Preamp Bypass", false);
    addFloat("preamp_drive", "Preamp Drive", 0.0f, 100.0f, 20.0f);
    addChoice("preamp_mode", "Preamp Mode", { "Clean", "Tube", "Transistor" }, 1);

    // 3. HPF
    addBool("filter_bypass", "HPF Bypass", false);
    addFloat("filter_hpf_freq", "HPF Freq (Hz)", 20.0f, 300.0f, 80.0f);
    addChoice("filter_hpf_slope", "HPF Slope", { "12 dB/oct", "24 dB/oct" }, 1);

    // 4. GATE / EXPANDER
    addBool("gate_bypass", "Gate Bypass", false);
    addFloat("gate_thresh", "Gate Threshold (dB)", -80.0f, 0.0f, -40.0f);
    addFloat("gate_ratio", "Gate Ratio", 1.0f, 8.0f, 2.0f);
    addFloat("gate_attack", "Gate Attack (ms)", 0.1f, 50.0f, 1.0f);
    addFloat("gate_release", "Gate Release (ms)", 10.0f, 1000.0f, 100.0f);

    // 5. CORRECTIVE EQ (5 Bandes)
    addBool("eq_corr_bypass", "Corrective EQ Bypass", false);
    for (int i = 0; i < 5; ++i)
    {
        juce::String prefix = "eq_corr_b" + juce::String(i + 1) + "_";
        addFloat(prefix + "freq", "EQ Corr B" + juce::String(i + 1) + " Freq", 20.0f, 20000.0f, (i == 0 ? 100.0f : i == 1 ? 300.0f : i == 2 ? 1000.0f : i == 3 ? 3000.0f : 8000.0f));
        addFloat(prefix + "gain", "EQ Corr B" + juce::String(i + 1) + " Gain (dB)", -18.0f, 18.0f, 0.0f);
        addFloat(prefix + "q", "EQ Corr B" + juce::String(i + 1) + " Q", 0.1f, 10.0f, 0.707f);
    }

    // 6. COMPRESSOR OPTO / FET
    addBool("comp_bypass", "Compressor Bypass", false);
    addChoice("comp_type", "Comp Type", { "Opto (LA-2A)", "FET (1176)" }, 0);
    addFloat("comp_thresh", "Comp Threshold (dB)", -60.0f, 0.0f, -20.0f);
    addFloat("comp_ratio", "Comp Ratio", 1.0f, 20.0f, 4.0f);
    addFloat("comp_attack", "Comp Attack (ms)", 0.01f, 100.0f, 10.0f);
    addFloat("comp_release", "Comp Release (ms)", 10.0f, 1000.0f, 100.0f);
    addFloat("comp_makeup", "Comp Makeup (dB)", 0.0f, 24.0f, 0.0f);

    // 7. DE-ESSER
    addBool("deesser_bypass", "De-Esser Bypass", false);
    addFloat("deesser_freq", "De-Esser Freq (Hz)", 4000.0f, 10000.0f, 6000.0f);
    addFloat("deesser_thresh", "De-Esser Thresh (dB)", -60.0f, 0.0f, -20.0f);
    addFloat("deesser_amount", "De-Esser Reduction (dB)", 0.0f, 24.0f, 6.0f);
    addChoice("deesser_mode", "De-Esser Mode", { "Wideband", "Split-Band" }, 1);

    // 8. COLOR EQ (6 Bandes)
    addBool("eq_color_bypass", "Color EQ Bypass", false);
    for (int i = 0; i < 6; ++i)
    {
        juce::String prefix = "eq_color_b" + juce::String(i + 1) + "_";
        addFloat(prefix + "freq", "EQ Color B" + juce::String(i + 1) + " Freq", 20.0f, 20000.0f, (i == 0 ? 80.0f : i == 1 ? 250.0f : i == 2 ? 1500.0f : i == 3 ? 3500.0f : i == 4 ? 8000.0f : 12000.0f));
        addFloat(prefix + "gain", "EQ Color B" + juce::String(i + 1) + " Gain (dB)", -12.0f, 12.0f, 0.0f);
        addFloat(prefix + "q", "EQ Color B" + juce::String(i + 1) + " Q", 0.1f, 5.0f, 0.707f);
    }

    // 9. GLUE COMPRESSOR
    addBool("glue_bypass", "Glue Comp Bypass", false);
    addFloat("glue_thresh", "Glue Thresh (dB)", -40.0f, 0.0f, -10.0f);
    addFloat("glue_ratio", "Glue Ratio", 1.5f, 10.0f, 2.0f);
    addFloat("glue_attack", "Glue Attack (ms)", 0.1f, 30.0f, 10.0f);
    addFloat("glue_release", "Glue Release (ms)", 50.0f, 1200.0f, 100.0f);

    // 10. SATURATION / EXCITER
    addBool("exciter_bypass", "Exciter Bypass", false);
    addFloat("exciter_even", "Even Harmonics (Warmth)", 0.0f, 1.0f, 0.2f);
    addFloat("exciter_odd", "Odd Harmonics (Air)", 0.0f, 1.0f, 0.2f);
    addFloat("exciter_mix", "Exciter Mix", 0.0f, 1.0f, 0.3f);

    // 11. REVERB & DELAY
    addBool("fx_bypass", "Reverb/Delay Bypass", false);
    addFloat("fx_rev_wet", "Reverb Wet", 0.0f, 1.0f, 0.15f);
    addFloat("fx_delay_time", "Delay Time (ms)", 10.0f, 1000.0f, 120.0f);
    addFloat("fx_delay_mix", "Delay Mix", 0.0f, 1.0f, 0.15f);

    // 12. OUTPUT LIMITER
    addBool("limiter_bypass", "Limiter Bypass", false);
    addFloat("limiter_ceiling", "Limiter Ceiling (dB)", -12.0f, 0.0f, -0.1f);

    // 13. OUTPUT STAGE
    addBool("out_bypass", "Output Stage Bypass", false);
    addFloat("out_gain", "Output Gain (dB)", -24.0f, 24.0f, 0.0f);

    return { params.begin(), params.end() };
}

void VocalChainAudioProcessor::updateParameters()
{
    // 1. Gain Stage
    gainStage->setBypass(apvts.getRawParameterValue("gain_bypass")->load() > 0.5f);
    gainStage->setGainDb(apvts.getRawParameterValue("gain_input")->load());
    gainStage->setPhaseInvert(apvts.getRawParameterValue("gain_phase")->load() > 0.5f);

    // 2. Preamp
    preamp->setBypass(apvts.getRawParameterValue("preamp_bypass")->load() > 0.5f);
    preamp->setDrive(apvts.getRawParameterValue("preamp_drive")->load());
    preamp->setPreampType(static_cast<PreampType>(static_cast<int>(apvts.getRawParameterValue("preamp_mode")->load())));

    // 3. HPF
    hpf->setBypass(apvts.getRawParameterValue("filter_bypass")->load() > 0.5f);
    hpf->setCutoffFrequency(apvts.getRawParameterValue("filter_hpf_freq")->load());
    hpf->setSlope(static_cast<HPFSlope>(static_cast<int>(apvts.getRawParameterValue("filter_hpf_slope")->load())));

    // 4. Gate
    gate->setBypass(apvts.getRawParameterValue("gate_bypass")->load() > 0.5f);
    gate->setThresholdDb(apvts.getRawParameterValue("gate_thresh")->load());
    gate->setRatio(apvts.getRawParameterValue("gate_ratio")->load());
    gate->setAttackMs(apvts.getRawParameterValue("gate_attack")->load());
    gate->setReleaseMs(apvts.getRawParameterValue("gate_release")->load());

    // 5. Corrective EQ
    correctiveEQ->setBypass(apvts.getRawParameterValue("eq_corr_bypass")->load() > 0.5f);
    for (size_t i = 0; i < 5; ++i)
    {
        juce::String prefix = "eq_corr_b" + juce::String(i + 1) + "_";
        correctiveEQ->setBandParameters(i,
            apvts.getRawParameterValue(prefix + "freq")->load(),
            apvts.getRawParameterValue(prefix + "gain")->load(),
            apvts.getRawParameterValue(prefix + "q")->load());
    }

    // 6. Compressor Opto/FET
    compOptoFET->setBypass(apvts.getRawParameterValue("comp_bypass")->load() > 0.5f);
    compOptoFET->setMode(static_cast<CompMode>(static_cast<int>(apvts.getRawParameterValue("comp_type")->load())));
    compOptoFET->setThresholdDb(apvts.getRawParameterValue("comp_thresh")->load());
    compOptoFET->setRatio(apvts.getRawParameterValue("comp_ratio")->load());
    compOptoFET->setAttackMs(apvts.getRawParameterValue("comp_attack")->load());
    compOptoFET->setReleaseMs(apvts.getRawParameterValue("comp_release")->load());
    compOptoFET->setMakeupGainDb(apvts.getRawParameterValue("comp_makeup")->load());

    // 7. De-Esser
    deEsser->setBypass(apvts.getRawParameterValue("deesser_bypass")->load() > 0.5f);
    deEsser->setFrequency(apvts.getRawParameterValue("deesser_freq")->load());
    deEsser->setThresholdDb(apvts.getRawParameterValue("deesser_thresh")->load());
    deEsser->setAmountDb(apvts.getRawParameterValue("deesser_amount")->load());
    deEsser->setMode(static_cast<DeEsserMode>(static_cast<int>(apvts.getRawParameterValue("deesser_mode")->load())));

    // 8. Color EQ
    colorEQ->setBypass(apvts.getRawParameterValue("eq_color_bypass")->load() > 0.5f);
    for (size_t i = 0; i < 6; ++i)
    {
        juce::String prefix = "eq_color_b" + juce::String(i + 1) + "_";
        colorEQ->setBand(i,
            apvts.getRawParameterValue(prefix + "freq")->load(),
            apvts.getRawParameterValue(prefix + "gain")->load(),
            apvts.getRawParameterValue(prefix + "q")->load());
    }

    // 9. Glue Comp
    glueComp->setBypass(apvts.getRawParameterValue("glue_bypass")->load() > 0.5f);
    glueComp->setThresholdDb(apvts.getRawParameterValue("glue_thresh")->load());
    glueComp->setRatio(apvts.getRawParameterValue("glue_ratio")->load());
    glueComp->setAttackMs(apvts.getRawParameterValue("glue_attack")->load());
    glueComp->setReleaseMs(apvts.getRawParameterValue("glue_release")->load());

    // 10. Exciter
    exciter->setBypass(apvts.getRawParameterValue("exciter_bypass")->load() > 0.5f);
    exciter->setEvenHarmonics(apvts.getRawParameterValue("exciter_even")->load());
    exciter->setOddHarmonics(apvts.getRawParameterValue("exciter_odd")->load());
    exciter->setMix(apvts.getRawParameterValue("exciter_mix")->load());

    // 11. Reverb / Delay
    reverbDelay->setBypass(apvts.getRawParameterValue("fx_bypass")->load() > 0.5f);
    reverbDelay->setReverbWet(apvts.getRawParameterValue("fx_rev_wet")->load());
    reverbDelay->setDelayTimeMs(apvts.getRawParameterValue("fx_delay_time")->load());
    reverbDelay->setDelayMix(apvts.getRawParameterValue("fx_delay_mix")->load());

    // 12. Output Limiter
    limiter->setBypass(apvts.getRawParameterValue("limiter_bypass")->load() > 0.5f);
    limiter->setCeilingDb(apvts.getRawParameterValue("limiter_ceiling")->load());

    // 13. Output Stage
    outputStage->setBypass(apvts.getRawParameterValue("out_bypass")->load() > 0.5f);
    outputStage->setOutputGainDb(apvts.getRawParameterValue("out_gain")->load());
}

void VocalChainAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    juce::dsp::ProcessSpec spec;
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = static_cast<juce::uint32>(samplesPerBlock);
    spec.numChannels = static_cast<juce::uint32>(getTotalNumOutputChannels());

    for (auto* module : dspChain)
        module->prepare(spec);

    updateParameters();
}

void VocalChainAudioProcessor::releaseResources()
{
    for (auto* module : dspChain)
        module->reset();
}

void VocalChainAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    // Synchronisation dynamique des paramètres DSP
    updateParameters();

    juce::dsp::AudioBlock<float> block(buffer);

    // Traitement séquentiel à travers les 13 modules
    for (auto* module : dspChain)
        module->process(block);
}

void VocalChainAudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

void VocalChainAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));
    if (xmlState != nullptr && xmlState->hasTagName(apvts.state.getType()))
        apvts.replaceState(juce::ValueTree::fromXml(*xmlState));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new VocalChainAudioProcessor(); }
