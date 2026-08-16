#include "PluginProcessor.h"
#include "PluginEditor.h"

VocalChainAudioProcessorEditor::VocalChainAudioProcessorEditor (VocalChainAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    // Taille globale de l'éditeur
    setSize (1200, 480);

    // 1. Configuration de la section Presets
    setupPresetsSection();

    // 2. Construction dynamique des UI pour chaque module
    auto addSlider = [&](ModuleUI& ui, const juce::String& paramID, const juce::String& labelText) {
        auto slider = std::make_unique<juce::Slider>(juce::Slider::RotaryHorizontalVerticalDrag, juce::Slider::TextBoxBelow);
        auto label = std::make_unique<juce::Label>("", labelText);
        label->setJustificationType(juce::Justification::centred);

        ui.panel.addAndMakeVisible(*slider);
        ui.panel.addAndMakeVisible(*label);

        ui.sliderAttachments.push_back(
            std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.apvts, paramID, *slider));

        ui.sliders.push_back(std::move(slider));
        ui.labels.push_back(std::move(label));
    };

    auto addChoice = [&](ModuleUI& ui, const juce::String& paramID, const juce::StringArray& choices) {
        auto combo = std::make_unique<juce::ComboBox>();
        combo->addItemList(choices, 1);
        ui.panel.addAndMakeVisible(*combo);

        ui.comboAttachments.push_back(
            std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(audioProcessor.apvts, paramID, *combo));

        ui.comboBoxes.push_back(std::move(combo));
    };

    auto createModule = [&](const juce::String& name, const juce::String& bypassID) -> ModuleUI* {
        auto ui = std::make_unique<ModuleUI>();
        ui->panel.setText(name);
        ui->panel.addAndMakeVisible(ui->bypassButton);
        ui->bypassAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
            audioProcessor.apvts, bypassID, ui->bypassButton);
        
        containerComponent.addAndMakeVisible(ui->panel);
        auto* rawPtr = ui.get();
        moduleUIs.push_back(std::move(ui));
        return rawPtr;
    };

    // --- Instanciation des modules GUI ---
    
    // 1. Gain Stage
    auto* mGain = createModule("1. Gain Stage", "gain_bypass");
    addSlider(*mGain, "gain_input", "Input Gain");
    mGain->peakMeter = std::make_unique<MeterComponent>(MeterComponent::Type::PeakLevel);
    mGain->panel.addAndMakeVisible(mGain->peakMeter.get());

    // 2. Preamp
    auto* mPreamp = createModule("2. Preamp", "preamp_bypass");
    addSlider(*mPreamp, "preamp_drive", "Drive");
    addChoice(*mPreamp, "preamp_mode", { "Clean", "Tube", "Transistor" });

    // 3. HPF
    auto* mHpf = createModule("3. HPF", "filter_bypass");
    addSlider(*mHpf, "filter_hpf_freq", "Cutoff");
    addChoice(*mHpf, "filter_hpf_slope", { "12 dB/oct", "24 dB/oct" });

    // 4. Gate / Expander
    auto* mGate = createModule("4. Gate", "gate_bypass");
    addSlider(*mGate, "gate_thresh", "Thresh");
    addSlider(*mGate, "gate_ratio", "Ratio");
    addSlider(*mGate, "gate_attack", "Attack");
    addSlider(*mGate, "gate_release", "Release");
    mGate->grMeter = std::make_unique<MeterComponent>(MeterComponent::Type::GainReduction);
    mGate->panel.addAndMakeVisible(mGate->grMeter.get());

    // 5. Corrective EQ (5 bandes)
    auto* mEqCorr = createModule("5. Corrective EQ", "eq_corr_bypass");
    for (int i = 1; i <= 5; ++i)
    {
        addSlider(*mEqCorr, "eq_corr_b" + juce::String(i) + "_freq", "B" + juce::String(i) + " Freq");
        addSlider(*mEqCorr, "eq_corr_b" + juce::String(i) + "_gain", "B" + juce::String(i) + " Gain");
    }

    // 6. Compressor Opto/FET
    auto* mComp = createModule("6. Comp Opto/FET", "comp_bypass");
    addChoice(*mComp, "comp_type", { "Opto", "FET" });
    addSlider(*mComp, "comp_thresh", "Thresh");
    addSlider(*mComp, "comp_ratio", "Ratio");
    addSlider(*mComp, "comp_makeup", "Makeup");
    mComp->grMeter = std::make_unique<MeterComponent>(MeterComponent::Type::GainReduction);
    mComp->panel.addAndMakeVisible(mComp->grMeter.get());

    // 7. De-Esser
    auto* mDeEsser = createModule("7. De-Esser", "deesser_bypass");
    addSlider(*mDeEsser, "deesser_freq", "Freq");
    addSlider(*mDeEsser, "deesser_thresh", "Thresh");
    addSlider(*mDeEsser, "deesser_amount", "Amount");
    mDeEsser->grMeter = std::make_unique<MeterComponent>(MeterComponent::Type::GainReduction);
    mDeEsser->panel.addAndMakeVisible(mDeEsser->grMeter.get());

    // 8. Color EQ
    auto* mColorEq = createModule("8. Color EQ", "eq_color_bypass");
    for (int i = 1; i <= 6; ++i)
    {
        addSlider(*mColorEq, "eq_color_b" + juce::String(i) + "_freq", "B" + juce::String(i) + " Freq");
        addSlider(*mColorEq, "eq_color_b" + juce::String(i) + "_gain", "B" + juce::String(i) + " Gain");
    }

    // 9. Glue Comp
    auto* mGlue = createModule("9. Glue Comp", "glue_bypass");
    addSlider(*mGlue, "glue_thresh", "Thresh");
    addSlider(*mGlue, "glue_ratio", "Ratio");
    mGlue->grMeter = std::make_unique<MeterComponent>(MeterComponent::Type::GainReduction);
    mGlue->panel.addAndMakeVisible(mGlue->grMeter.get());

    // 10. Exciter
    auto* mExciter = createModule("10. Exciter", "exciter_bypass");
    addSlider(*mExciter, "exciter_even", "Warmth");
    addSlider(*mExciter, "exciter_odd", "Air");
    addSlider(*mExciter, "exciter_mix", "Mix");

    // 11. Reverb / Delay
    auto* mFx = createModule("11. Reverb / Delay", "fx_bypass");
    addSlider(*mFx, "fx_rev_wet", "Reverb");
    addSlider(*mFx, "fx_delay_mix", "Delay");

    // 12. Limiter
    auto* mLimiter = createModule("12. Limiter", "limiter_bypass");
    addSlider(*mLimiter, "limiter_ceiling", "Ceiling");

    // 13. Output Stage
    auto* mOut = createModule("13. Output Stage", "out_bypass");
    addSlider(*mOut, "out_gain", "Out Gain");
    mOut->peakMeter = std::make_unique<MeterComponent>(MeterComponent::Type::PeakLevel);
    mOut->panel.addAndMakeVisible(mOut->peakMeter.get());

    // Configuration du Viewport (défilement horizontal)
    viewport.setViewedComponent(&containerComponent, false);
    viewport.setScrollBarsShown(true, true, false, true);
    addAndMakeVisible(viewport);

    // Lancement du timer à 30 Hz pour les VU-mètres
    startTimerHz(30);
}

VocalChainAudioProcessorEditor::~VocalChainAudioProcessorEditor()
{
    stopTimer();
}

void VocalChainAudioProcessorEditor::setupPresetsSection()
{
    addAndMakeVisible(presetGroup);
    presetGroup.addAndMakeVisible(presetSelector);
    presetGroup.addAndMakeVisible(savePresetButton);
    presetGroup.addAndMakeVisible(loadPresetButton);

    presetSelector.addItem("Default", 1);
    presetSelector.addItem("Vocal Lead Pop", 2);
    presetSelector.addItem("Radio / Broadcast", 3);
    presetSelector.setSelectedId(1);

    savePresetButton.onClick = [this] { savePreset(); };
    loadPresetButton.onClick = [this] { loadPreset(); };
}

void VocalChainAudioProcessorEditor::savePreset()
{
    activeFileChooser = std::make_unique<juce::FileChooser>(
        "Enregistrer le Preset",
        juce::File::getSpecialLocation(juce::File::userDocumentsDirectory),
        "*.xml");

    auto flags = juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::warnAboutOverwriting;

    activeFileChooser->launchAsync(flags, [this](const juce::FileChooser& chooser)
    {
        auto file = chooser.getResult();
        if (file != juce::File{})
        {
            auto state = audioProcessor.apvts.copyState();
            std::unique_ptr<juce::XmlElement> xml(state.createXml());
            xml->writeTo(file);
        }
    });
}

void VocalChainAudioProcessorEditor::loadPreset()
{
    activeFileChooser = std::make_unique<juce::FileChooser>(
        "Charger un Preset",
        juce::File::getSpecialLocation(juce::File::userDocumentsDirectory),
        "*.xml");

    auto flags = juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles;

    activeFileChooser->launchAsync(flags, [this](const juce::FileChooser& chooser)
    {
        auto file = chooser.getResult();
        if (file != juce::File{})
        {
            auto xml = juce::XmlDocument::parse(file);
            if (xml != nullptr && xml->hasTagName(audioProcessor.apvts.state.getType()))
            {
                audioProcessor.apvts.replaceState(juce::ValueTree::fromXml(*xml));
            }
        }
    });
}

void VocalChainAudioProcessorEditor::timerCallback()
{
    // Actualisation des VU-mètres via les getters publics de l'audioProcessor
    if (auto* mGain = moduleUIs[0].get(); mGain && mGain->peakMeter)
        if (auto* mod = audioProcessor.getGainStage())
            mGain->peakMeter->setLevel(mod->getPeakLevel(0));

    if (auto* mGate = moduleUIs[3].get(); mGate && mGate->grMeter)
        if (auto* mod = audioProcessor.getGate())
            mGate->grMeter->setLevel(mod->getGainReductionDb());

    if (auto* mComp = moduleUIs[5].get(); mComp && mComp->grMeter)
        if (auto* mod = audioProcessor.getCompOptoFET())
            mComp->grMeter->setLevel(mod->getGainReductionDb());

    if (auto* mDeEsser = moduleUIs[6].get(); mDeEsser && mDeEsser->grMeter)
        if (auto* mod = audioProcessor.getDeEsser())
            mDeEsser->grMeter->setLevel(mod->getGainReductionDb());

    if (auto* mGlue = moduleUIs[8].get(); mGlue && mGlue->grMeter)
        if (auto* mod = audioProcessor.getGlueComp())
            mGlue->grMeter->setLevel(mod->getGainReductionDb());

    if (auto* mOut = moduleUIs[12].get(); mOut && mOut->peakMeter)
        if (auto* mod = audioProcessor.getOutputStage())
            mOut->peakMeter->setLevel(mod->getPeakLevel(0));
}

void VocalChainAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colour (0xff22252a));
}

void VocalChainAudioProcessorEditor::resized()
{
    auto area = getLocalBounds().reduced(8);

    // Zone supérieure : Presets
    auto presetArea = area.removeFromTop(50);
    presetGroup.setBounds(presetArea);
    auto pInner = presetArea.reduced(10, 15);
    presetSelector.setBounds(pInner.removeFromLeft(200));
    pInner.removeFromLeft(10);
    savePresetButton.setBounds(pInner.removeFromLeft(80));
    pInner.removeFromLeft(5);
    loadPresetButton.setBounds(pInner.removeFromLeft(80));

    area.removeFromTop(8);

    // Zone principale : Viewport
    viewport.setBounds(area);

    // Positionnement dynamique horizontal des modules
    int currentX = 0;
    const int moduleHeight = area.getHeight() - 20;

    for (auto& ui : moduleUIs)
    {
        int numElements = static_cast<int>(ui->sliders.size() + ui->comboBoxes.size());
        int moduleWidth = juce::jmax(140, numElements * 75 + 20);

        ui->panel.setBounds(currentX, 0, moduleWidth, moduleHeight);
        
        auto panelBounds = ui->panel.getLocalBounds().reduced(8, 20);

        // Bouton Bypass
        ui->bypassButton.setBounds(panelBounds.removeFromTop(24));

        // VU-mètres s'ils existent
        if (ui->peakMeter)
            ui->peakMeter->setBounds(panelBounds.removeFromTop(12).reduced(4, 1));
        if (ui->grMeter)
            ui->grMeter->setBounds(panelBounds.removeFromTop(12).reduced(4, 1));

        // Positionnement des ComboBoxes
        for (auto& combo : ui->comboBoxes)
            combo->setBounds(panelBounds.removeFromTop(26).reduced(4, 2));

        // Positionnement des Sliders (Knobs) en grille/colonne
        auto sliderArea = panelBounds;
        int knobWidth = 65;
        int knobHeight = 85;

        int xOffset = sliderArea.getX();
        int yOffset = sliderArea.getY();

        for (size_t i = 0; i < ui->sliders.size(); ++i)
        {
            ui->labels[i]->setBounds(xOffset, yOffset, knobWidth, 16);
            ui->sliders[i]->setBounds(xOffset, yOffset + 16, knobWidth, knobHeight - 16);

            xOffset += knobWidth + 5;
            if (xOffset + knobWidth > ui->panel.getWidth() - 10)
            {
                xOffset = sliderArea.getX();
                yOffset += knobHeight + 5;
            }
        }

        currentX += moduleWidth + 8;
    }

    containerComponent.setSize(currentX, moduleHeight);
}
