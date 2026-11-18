#include "PluginProcessor.h"
#include "dsp/GrainEngine.h"
#include <cmath>
#include <chrono>
#include <iostream>
#include <limits>

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
        check(mix != nullptr && std::abs(mix->convertFrom0to1(mix->getDefaultValue()) - .65f) < .001f, "65 percent init mix");
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
    // Compare a sustained input, and then a silent tail, with feedback off/on.
    // This is a deterministic signal measurement, not a substitute for listening.
    for (int mode : { 0, 1, 3, 4, 7 })
    {
        GrainveilDsp::GrainEngine off, high;
        off.prepare(sampleRate); high.prepare(sampleRate);
        GrainveilDsp::EngineParams p;
        p.mix = 1; p.captureTimeMs = 50; p.grainSizeMs = 120; p.densityHz = 12;
        p.scatter = .25f; p.tone = .7f; p.fbTone = .7f; p.mode = mode;
        juce::AudioBuffer<float> a(2, blockSize), b(2, blockSize);
        double dryEnergy = 0, wetEnergy = 0, offTail = 0, highTail = 0;
        int sustainCount = 0, tailCount = 0;
        float peak = 0;
        for (int block = 0; block < 1125; ++block)
        {
            for (int i = 0; i < blockSize; ++i)
            {
                const int t = block * blockSize + i;
                const float x = t < sampleRate * .45 ? .1f * std::sin(static_cast<float>(t) * (2.0f * 3.14159265358979323846f * 440.0f / sampleRate)) : 0.0f;
                for (int ch = 0; ch < 2; ++ch) { a.setSample(ch, i, x); b.setSample(ch, i, x); }
                if (t > sampleRate * .2 && t < sampleRate * .4) { dryEnergy += x * x; ++sustainCount; }
            }
            p.feedback = 0; off.process(a, p);
            p.feedback = .95f; high.process(b, p);
            for (int i = 0; i < blockSize; ++i)
            {
                const int t = block * blockSize + i;
                const float dryWet = a.getSample(0, i), fbWet = b.getSample(0, i);
                check(std::isfinite(dryWet) && std::isfinite(fbWet), "feedback path stays finite");
                peak = std::max(peak, std::abs(fbWet));
                if (t > sampleRate * .2 && t < sampleRate * .4) wetEnergy += dryWet * dryWet;
                if (t > sampleRate * 2) { offTail += dryWet * dryWet; highTail += fbWet * fbWet; ++tailCount; }
            }
        }
        const double dryRms = std::sqrt(dryEnergy / sustainCount), wetRms = std::sqrt(wetEnergy / sustainCount);
        const double quietTail = std::sqrt(offTail / tailCount), bloomTail = std::sqrt(highTail / tailCount);
        std::cout << "Mode " << mode << " wet/dry RMS " << wetRms / dryRms << " feedback tails " << quietTail << "/" << bloomTail << '\n';
        check(wetRms / dryRms > .2512, "representative wet RMS is within 12 dB of dry");
        check(bloomTail > .02 && bloomTail > quietTail * 4, "high feedback sustains a bloom");
        check(peak <= 1.00001f, "fully wet output stays tanh bounded at unity output gain");
    }
    {
        GrainveilDsp::GrainEngine engine;
        engine.prepare(sampleRate);
        GrainveilDsp::EngineParams p;
        p.mix = .5f; p.captureTimeMs = 200; p.grainSizeMs = 120; p.densityHz = 12;
        p.scatter = .65f; p.feedback = 0; p.tone = .7f;
        juce::AudioBuffer<float> audio(2, blockSize);
        double diff = 0, dry = 0; int n = 0;
        for (int block = 0; block < 375; ++block)
        {
            for (int i = 0; i < blockSize; ++i)
            {
                const int t = block * blockSize + i;
                const float x = .15f * std::sin(static_cast<float>(t) * (2.0f * 3.14159265358979323846f * 220.0f / sampleRate));
                audio.setSample(0, i, x); audio.setSample(1, i, x);
            }
            engine.process(audio, p);
            if (block > 190)
                for (int i = 0; i < blockSize; ++i)
                {
                    const int t = block * blockSize + i;
                    const float x = .15f * std::sin(static_cast<float>(t) * (2.0f * 3.14159265358979323846f * 220.0f / sampleRate));
                    const float delta = audio.getSample(0, i) - x;
                    diff += delta * delta; dry += x * x; ++n;
                }
        }
        const double relative = std::sqrt(diff / dry);
        std::cout << "50% mix difference/input RMS " << relative << " over " << n << " samples\n";
        check(relative > .15, "50% mix measurably changes representative input");
    }
    {
        GrainveilDsp::GrainEngine a, b;
        a.prepare(sampleRate); b.prepare(sampleRate);
        GrainveilDsp::EngineParams p;
        p.mix = 1; p.captureTimeMs = 800; p.grainSizeMs = 1200; p.densityHz = 30;
        p.scatter = .4f; p.feedback = 0; p.mode = 3; p.tone = .7f;
        juce::AudioBuffer<float> first(2, blockSize), second(2, blockSize);
        double energy = 0, difference = 0; int n = 0;
        for (int block = 0; block < 1125; ++block)
        {
            const bool freeze = block * blockSize >= sampleRate;
            for (int i = 0; i < blockSize; ++i)
            {
                const int t = block * blockSize + i;
                const float x = !freeze ? .1f * std::sin(static_cast<float>(t) * (2.0f * 3.14159265358979323846f * 220.0f / sampleRate)) : 0.0f;
                for (int ch = 0; ch < 2; ++ch)
                {
                    first.setSample(ch, i, x);
                    second.setSample(ch, i, freeze ? .9f : x);
                }
            }
            p.freeze = freeze;
            p.feedback = 0;
            a.process(first, p);
            p.feedback = freeze ? .95f : 0.0f;
            b.process(second, p);
            if (block * blockSize > sampleRate * 2)
                for (int i = 0; i < blockSize; ++i)
                {
                    const float x = first.getSample(0, i), y = second.getSample(0, i);
                    energy += x * x; difference += (x-y) * (x-y); ++n;
                }
        }
        const double rms = std::sqrt(energy / n), diff = std::sqrt(difference / n);
        std::cout << "Frozen long-grain RMS " << rms << ", input isolation difference " << diff << '\n';
        check(rms > .02, "frozen long grains sustain a texture after capture");
        check(diff < 1e-7, "frozen buffer ignores new input and feedback writes");
    }
    {
        GrainveilDsp::GrainEngine engine;
        engine.prepare(sampleRate);
        GrainveilDsp::EngineParams p;
        p.mix = 1; p.feedback = .95f; p.grainSizeMs = 5; p.densityHz = 40;
        juce::AudioBuffer<float> audio(2, blockSize);
        bool finite = true;
        for (int block = 0; block < 120; ++block)
        {
            for (int i = 0; i < blockSize; ++i)
            {
                const float v = block == 10 ? (i % 2 == 0 ? std::numeric_limits<float>::infinity() : std::numeric_limits<float>::quiet_NaN()) : .1f;
                audio.setSample(0, i, v); audio.setSample(1, i, -v);
            }
            if (block == 20) p.scatter = std::numeric_limits<float>::quiet_NaN();
            if (block == 30) p.scatter = .8f;
            engine.process(audio, p);
            for (int ch = 0; ch < 2; ++ch)
                for (int i = 0; i < blockSize; ++i)
                    finite = finite && std::isfinite(audio.getSample(ch, i));
        }
        check(finite, "NaN and infinity cannot poison audio or capture state");
    }
    {
        GrainveilDsp::GrainEngine engine;
        engine.prepare(sampleRate);
        GrainveilDsp::EngineParams p;
        p.mix = 1; p.mode = 4; p.grainSizeMs = 5; p.densityHz = 40;
        p.captureTimeMs = 50; p.scatter = 1; p.feedback = .8f;
        juce::AudioBuffer<float> audio(2, blockSize);
        const auto start = std::chrono::steady_clock::now();
        constexpr int blocks = 375; // one second, 48 kHz / 128 samples
        for (int block = 0; block < blocks; ++block)
        {
            for (int i = 0; i < blockSize; ++i)
            {
                const float x = .1f * std::sin(static_cast<float>(block * blockSize + i) * .03f);
                audio.setSample(0, i, x); audio.setSample(1, i, x);
            }
            engine.process(audio, p);
        }
        const auto elapsed = std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count();
        std::cout << "32-voice 48k/128 stress: " << elapsed << " s per 1 s audio (includes test input generation; no real-time guarantee)\n";
    }
    return failures == 0 ? 0 : 1;
}
