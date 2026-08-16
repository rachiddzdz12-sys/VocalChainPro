#pragma once

#include <JuceHeader.h>
#include "Modules/GainStageModule.h"
#include "Modules/PreampModule.h"
#include "Modules/HPFModule.h"
#include "Modules/GateModule.h"
#include "Modules/CorrectiveEQModule.h"
#include "Modules/CompOptoFETModule.h"
#include "Modules/DeEsserModule.h"
#include "Modules/ColorEQModule.h"
#include "Modules/GlueCompModule.h"
#include "Modules/ExciterModule.h"
#include "Modules/ReverbDelayModule.h"
#include "Modules/LimiterModule.h"
#include "Modules/OutputStageModule.h"

class VocalChainAudioProcessor  : public juce::AudioProcessor
{
public:
    VocalChainAudioProcessor();
    ~VocalChainAudioProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState apvts;

    // Getters publics pour accéder aux modules depuis l'éditeur / VU-mètres
    GainStageModule* getGainStage() const { return gainStage.get(); }
    GateModule* getGate() const { return gate.get(); }
    CompOptoFETModule* getCompOptoFET() const { return compOptoFET.get(); }
    DeEsserModule* getDeEsser() const { return deEsser.get(); }
    GlueCompModule* getGlueComp() const { return glueComp.get(); }
    OutputStageModule* getOutputStage() const { return outputStage.get(); }

private:
    juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
    void updateParameters();

    std::unique_ptr<GainStageModule> gainStage;
    std::unique_ptr<PreampModule> preamp;
    std::unique_ptr<HPFModule> hpf;
    std::unique_ptr<GateModule> gate;
    std::unique_ptr<CorrectiveEQModule> correctiveEQ;
    std::unique_ptr<CompOptoFETModule> compOptoFET;
    std::unique_ptr<DeEsserModule> deEsser;
    std::unique_ptr<ColorEQModule> colorEQ;
    std::unique_ptr<GlueCompModule> glueComp;
    std::unique_ptr<ExciterModule> exciter;
    std::unique_ptr<ReverbDelayModule> reverbDelay;
    std::unique_ptr<LimiterModule> limiter;
    std::unique_ptr<OutputStageModule> outputStage;

    // Corrigé : IProcessModule est le vrai nom de l'interface (BaseModule n'existe pas)
    std::vector<IProcessModule*> dspChain;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (VocalChainAudioProcessor)
};
