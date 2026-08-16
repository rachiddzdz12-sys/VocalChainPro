// CrashHarness.cpp
// Petit programme de diagnostic autonome : teste chacun des 13 modules DSP
// individuellement avec des tailles de buffer variées, et isole les crashs
// grâce à la gestion d'exceptions structurées de Windows (SEH), pour
// identifier précisément quel module plante sans stopper les autres tests.

#include <JuceHeader.h>
#include <iostream>
#include <windows.h>

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

// Tailles de buffer variées, comme celles que pluginval teste (y compris
// des tailles inhabituelles : 1 sample, tailles impaires, gros blocs)
static const int testBlockSizes[] = { 1, 7, 32, 64, 128, 256, 441, 512, 1024, 2048, 4096, 8192 };

static int testsPassed = 0;
static int testsFailed = 0;

// Exécute un test protégé par SEH : si le code à l'intérieur plante,
// on récupère la main au lieu de crasher tout le programme.
template <typename Func>
bool runProtected(const char* testName, Func&& func)
{
    __try
    {
        func();
        std::cout << "[OK]   " << testName << std::endl;
        testsPassed++;
        return true;
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        DWORD code = GetExceptionCode();
        std::cout << "[FAIL] " << testName << "  -- Exception code: 0x" << std::hex << code << std::dec << std::endl;
        testsFailed++;
        return false;
    }
}

template <typename ModuleType>
void testModule(const char* moduleName)
{
    std::cout << "\n--- Test du module : " << moduleName << " ---" << std::endl;

    runProtected((std::string(moduleName) + " : construction").c_str(), [&]()
    {
        auto module = std::make_unique<ModuleType>();
    });

    for (int blockSize : testBlockSizes)
    {
        std::string label = std::string(moduleName) + " : prepare+process (blockSize=" + std::to_string(blockSize) + ")";

        runProtected(label.c_str(), [&]()
        {
            auto module = std::make_unique<ModuleType>();

            juce::dsp::ProcessSpec spec;
            spec.sampleRate = 44100.0;
            spec.maximumBlockSize = 8192; // toujours préparé pour le plus gros cas
            spec.numChannels = 2;

            module->prepare(spec);

            juce::AudioBuffer<float> buffer(2, blockSize);
            buffer.clear();

            // Remplit avec un peu de bruit pour simuler un vrai signal
            juce::Random rng(42);
            for (int ch = 0; ch < 2; ++ch)
                for (int i = 0; i < blockSize; ++i)
                    buffer.setSample(ch, i, rng.nextFloat() * 0.5f - 0.25f);

            juce::dsp::AudioBlock<float> block(buffer);

            // Traite plusieurs fois de suite, comme le ferait un vrai host
            for (int iter = 0; iter < 5; ++iter)
                module->process(block);

            module->reset();
            module->process(block);
        });
    }
}

int main()
{
    std::cout << "=====================================================" << std::endl;
    std::cout << " VocalChainPro - Harnais de diagnostic des modules DSP" << std::endl;
    std::cout << "=====================================================" << std::endl;

    testModule<GainStageModule>("GainStageModule");
    testModule<PreampModule>("PreampModule");
    testModule<HPFModule>("HPFModule");
    testModule<GateModule>("GateModule");
    testModule<CorrectiveEQModule>("CorrectiveEQModule");
    testModule<CompOptoFETModule>("CompOptoFETModule");
    testModule<DeEsserModule>("DeEsserModule");
    testModule<ColorEQModule>("ColorEQModule");
    testModule<GlueCompModule>("GlueCompModule");
    testModule<ExciterModule>("ExciterModule");
    testModule<ReverbDelayModule>("ReverbDelayModule");
    testModule<LimiterModule>("LimiterModule");
    testModule<OutputStageModule>("OutputStageModule");

    std::cout << "\n=====================================================" << std::endl;
    std::cout << " RESULTAT : " << testsPassed << " tests OK, " << testsFailed << " tests FAIL" << std::endl;
    std::cout << "=====================================================" << std::endl;

    return testsFailed > 0 ? 1 : 0;
}
