#include "PluginProcessor.h"
#include "dsp/GrainEngine.h"
#include <cmath>
#include <iostream>

namespace
{
int failures = 0;
void check(bool ok, const char* message)
{
    if (! ok) { std::cerr << "FAIL: " << message << '\n'; ++failures; }
}
}

int main()
{
    constexpr int blockSize = 128;
    constexpr int sampleRate = 48000;
    {
        juce::ScopedJuceInitialiser_GUI init;
        const auto root = juce::File::getSpecialLocation(juce::File::tempDirectory)
                              .getNonexistentChildFile("grainveil-sound-test-", {}, false);
        GrainveilAudioProcessor processor(root);
        auto* size = processor.getAPVTS().getParameter(GrainveilParams::grainSize);
        auto* mix = processor.getAPVTS().getParameter(GrainveilParams::mix);
        check(size != nullptr && size->convertFrom0to1(0) == 5 && size->convertFrom0to1(1) == 1200, "grain size range");
        check(mix != nullptr && std::abs(mix->convertFrom0to1(mix->getDefaultValue()) - .6f) < .001f, "60 percent init mix");
    }
    {
        GrainveilDsp::GrainEngine engine;
        engine.prepare(sampleRate);
        GrainveilDsp::EngineParams p;
        p.mix = 1.0f;
        p.freeze = true; // freshly cleared buffer: any later output would be dry leakage
        juce::AudioBuffer<float> audio(2, blockSize);
        float peak = 0;
        for (int block = 0; block < 40; ++block)
        {
            for (int i = 0; i < blockSize; ++i)
            {
                audio.setSample(0, i, .8f);
                audio.setSample(1, i, -.7f);
            }
            engine.process(audio, p);
            if (block >= 20)
                for (int ch = 0; ch < 2; ++ch)
                    for (int i = 0; i < blockSize; ++i)
                        peak = std::max(peak, std::abs(audio.getSample(ch, i)));
        }
        check(peak == 0.0f, "fully wet must contain no dry signal after smoothing");
    }
    {
        GrainveilDsp::GrainEngine dark, bright;
        dark.prepare(sampleRate); bright.prepare(sampleRate);
        GrainveilDsp::EngineParams p;
        p.mix = 1; p.grainSizeMs = 5; p.densityHz = 40;
        p.captureTimeMs = 50; p.feedback = 0; p.scatter = .5f;
        juce::AudioBuffer<float> a(2, blockSize), b(2, blockSize);
        double darkEnergy = 0, brightEnergy = 0, difference = 0;
        float peak = 0;
        int samples = 0;
        for (int block = 0; block < 375; ++block)
        {
            for (int i = 0; i < blockSize; ++i)
            {
                const float x = .05f * std::sin((block * blockSize + i) * (2.0f * 3.14159265358979323846f * 7000.0f / sampleRate));
                a.setSample(0, i, x); a.setSample(1, i, x);
                b.setSample(0, i, x); b.setSample(1, i, x);
            }
            p.tone = 0; dark.process(a, p);
            p.tone = 1; bright.process(b, p);
            if (block >= 190)
                for (int i = 0; i < blockSize; ++i)
                {
                    const float d = a.getSample(0, i), w = b.getSample(0, i);
                    const float x = .05f * std::sin((block * blockSize + i) * (2.0f * 3.14159265358979323846f * 7000.0f / sampleRate));
                    check(std::isfinite(d) && std::isfinite(w), "wet signal stays finite");
                    darkEnergy += d * d; brightEnergy += w * w; difference += (w - x) * (w - x);
                    peak = std::max(peak, std::abs(w)); ++samples;
                }
        }
        const double darkRms = std::sqrt(darkEnergy / samples), brightRms = std::sqrt(brightEnergy / samples);
        std::cout << "Short-grain wet peak " << peak << ", bright RMS " << brightRms
                  << ", dark RMS " << darkRms << ", wet/input difference RMS " << std::sqrt(difference / samples) << '\n';
        check(peak > .01f, "wet short-grain signal is not silent");
        check(brightRms > darkRms * 1.5, "bright tone retains substantially more high frequency energy");
        check(std::sqrt(difference / samples) > .005, "wet-only output differs from input");
    }
    return failures == 0 ? 0 : 1;
}
