cmake_minimum_required(VERSION 3.22)

project(VocalChainPro VERSION 1.0.0 LANGUAGES C CXX)

# Standard C++17 obligatoire pour JUCE 7/8
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ------------------------------------------------------------------------------
# 1. Intégration de JUCE (via FetchContent ou chemin local)
# ------------------------------------------------------------------------------
# Décommente et adapte si JUCE est installé localement sur ton système :
# add_subdirectory("C:/JUCE" JUCE)

# Sinon, téléchargement automatique de JUCE 7 via FetchContent :
include(FetchContent)
FetchContent_Declare(
    JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG        7.0.12 # Recommandé : version très stable
)
FetchContent_MakeAvailable(JUCE)

# ------------------------------------------------------------------------------
# 2. Déclaration du plugin JUCE
# ------------------------------------------------------------------------------
juce_add_plugin(VocalChainPro
    COMPANY_NAME "MyAudioStudio"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD TRUE
    PLUGIN_MANUFACTURER_CODE VcpM
    PLUGIN_CODE Vcp1
    FORMATS VST3 AU # AU sera automatiquement ignoré sous Windows
    PRODUCT_NAME "Vocal Chain Pro"
)

# Génère automatiquement JuceHeader.h, requis par tous les modules qui l'incluent
juce_generate_juce_header(VocalChainPro)

# ------------------------------------------------------------------------------
# 3. Liste des fichiers sources
# ------------------------------------------------------------------------------
target_sources(VocalChainPro PRIVATE
    # Fichiers principaux JUCE
    Source/PluginProcessor.h
    Source/PluginProcessor.cpp
    Source/PluginEditor.h
    Source/PluginEditor.cpp

    # Interfaces & Base
    Source/Common/IProcessModule.h

    # Modules DSP (Headers + Implémentations)
    Source/Modules/GainStageModule.h
    Source/Modules/GainStageModule.cpp
    Source/Modules/PreampModule.h
    Source/Modules/PreampModule.cpp
    Source/Modules/HPFModule.h
    Source/Modules/HPFModule.cpp
    Source/Modules/GateModule.h
    Source/Modules/GateModule.cpp
    Source/Modules/CorrectiveEQModule.h
    Source/Modules/CorrectiveEQModule.cpp
    Source/Modules/CompOptoFETModule.h
    Source/Modules/CompOptoFETModule.cpp
    Source/Modules/DeEsserModule.h
    Source/Modules/DeEsserModule.cpp
    Source/Modules/ColorEQModule.h
    Source/Modules/ColorEQModule.cpp
    Source/Modules/GlueCompModule.h
    Source/Modules/GlueCompModule.cpp
    Source/Modules/ExciterModule.h
    Source/Modules/ExciterModule.cpp
    Source/Modules/ReverbDelayModule.h
    Source/Modules/ReverbDelayModule.cpp
    Source/Modules/LimiterModule.h
    # NOTE : LimiterModule est header-only (tout inline dans le .h), pas de .cpp associé
    Source/Modules/OutputStageModule.h
    Source/Modules/OutputStageModule.cpp
)

# ------------------------------------------------------------------------------
# 4. Inclusion des dossiers d'en-tête (Header Search Paths)
# ------------------------------------------------------------------------------
target_include_directories(VocalChainPro PRIVATE
    Source
    Source/Common
    Source/Modules
)

# ------------------------------------------------------------------------------
# 5. Modules JUCE requis & Flags de compilation
# ------------------------------------------------------------------------------
target_link_libraries(VocalChainPro PRIVATE
    juce::juce_audio_basics
    juce::juce_audio_devices
    juce::juce_audio_formats
    juce::juce_audio_plugin_client
    juce::juce_audio_processors
    juce::juce_audio_utils
    juce::juce_core
    juce::juce_data_structures
    juce::juce_dsp
    juce::juce_events
    juce::juce_graphics
    juce::juce_gui_basics
    juce::juce_gui_extra
    juce::juce_recommended_config_flags
    juce::juce_recommended_lto_flags
    juce::juce_recommended_warning_flags
)

# Macro JUCE pour désactiver les warnings non pertinents
target_compile_definitions(VocalChainPro PRIVATE
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
)

# ------------------------------------------------------------------------------
# 6. Harnais de diagnostic (programme console qui teste chaque module isolément
#    et catch les crashs individuellement pour identifier le module fautif)
# ------------------------------------------------------------------------------
juce_add_console_app(CrashHarness
    PRODUCT_NAME "CrashHarness")

juce_generate_juce_header(CrashHarness)

target_sources(CrashHarness PRIVATE
    Source/Diagnostics/CrashHarness.cpp
    Source/Modules/GainStageModule.cpp
    Source/Modules/PreampModule.cpp
    Source/Modules/HPFModule.cpp
    Source/Modules/GateModule.cpp
    Source/Modules/CorrectiveEQModule.cpp
    Source/Modules/CompOptoFETModule.cpp
    Source/Modules/DeEsserModule.cpp
    Source/Modules/ColorEQModule.cpp
    Source/Modules/GlueCompModule.cpp
    Source/Modules/ExciterModule.cpp
    Source/Modules/ReverbDelayModule.cpp
    Source/Modules/OutputStageModule.cpp
)

target_include_directories(CrashHarness PRIVATE
    Source
    Source/Common
    Source/Modules
)

target_link_libraries(CrashHarness PRIVATE
    juce::juce_audio_basics
    juce::juce_core
    juce::juce_dsp
    juce::juce_events
    juce::juce_recommended_config_flags
    juce::juce_recommended_warning_flags
)

# /EHa nécessaire pour mélanger la gestion d'exceptions structurées Windows (__try/__except)
# avec des objets C++ ayant des destructeurs (comme std::unique_ptr) dans la même fonction
target_compile_options(CrashHarness PRIVATE /EHa)

