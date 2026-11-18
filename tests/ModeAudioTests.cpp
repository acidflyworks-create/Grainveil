#include "dsp/GrainEngine.h"
#include "FactoryPresets.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <iostream>
#include <vector>

namespace
{
constexpr int rate = 48000, blockSize = 128, total = 96000;
constexpr double pi = 3.14159265358979323846;
int failures = 0;
void check(bool ok, const char* message, const char* name = "")
{
    if (! ok) { std::cerr << "FAIL: " << message << " " << name << '\n'; ++failures; }
}
// Deterministic synthetic plucked harmonic signal. It is NOT a guitar DI recording.
float pluck(int t)
{
    static constexpr float notes[] = { 82.41f, 110.0f, 146.83f, 196.0f, 123.47f, 164.81f, 98.0f, 130.81f };
    const int length = 12000;
    const int note = (t / length) % 8, elapsed = t % length;
    const float envelope = std::exp(-static_cast<float>(elapsed) / (rate * .13f));
    const float phase = static_cast<float>(2.0 * pi * notes[note] * elapsed / rate);
    return .20f * envelope * (std::sin(phase) + .45f * std::sin(phase * 2) + .2f * std::sin(phase * 3));
}

GrainveilDsp::EngineParams paramsFor(const GrainveilFactory::Preset& preset)
{
    const auto& v = preset.values;
    GrainveilDsp::EngineParams p;
    p.mix=v[0]; p.inGainDb=v[1]; p.outGainDb=v[2]; p.freeze=v[3] >= .5f;
    p.captureTimeMs=v[4]; p.feedback=v[5]; p.fbTone=v[6]; p.grainSizeMs=v[7];
    p.densityHz=v[8]; p.scatter=v[9]; p.pitchSt=v[10]; p.pitchRand=v[11];
    p.reverseAmt=v[12]; p.spread=v[13]; p.tone=v[14]; p.mode=static_cast<int>(v[15]);
    return p;
}
}

int main()
{
    // Physical values corresponding approximately to noon for each skewed knob.
    // Two seconds measures deterministic output differences, not subjective identity.
    std::array<std::vector<float>, 8> modeOutput;
    for (int mode = 0; mode < 8; ++mode)
    {
        auto& result = modeOutput[static_cast<size_t>(mode)];
        result.reserve(total);
        GrainveilDsp::GrainEngine engine;
        engine.prepare(rate);
        GrainveilDsp::EngineParams p;
        p.mode=mode; p.mix=.7f; p.grainSizeMs=304; p.densityHz=10.8f; p.captureTimeMs=1038;
        p.scatter=.5f; p.pitchSt=0; p.pitchRand=.5f; p.reverseAmt=.5f; p.feedback=.475f;
        p.fbTone=.5f; p.spread=.5f; p.tone=.5f;
        juce::AudioBuffer<float> audio(2, blockSize);
        float peak = 0; double wetDifference = 0;
        for (int start=0; start<total; start+=blockSize)
        {
            for (int i=0; i<blockSize; ++i)
                for (int ch=0; ch<2; ++ch) audio.setSample(ch, i, pluck(start+i));
            engine.process(audio, p);
            for (int i=0; i<blockSize; ++i)
            {
                const float y = audio.getSample(0, i), x = pluck(start+i);
                check(std::isfinite(y), "non-finite mode output");
                peak = std::max(peak, std::abs(y));
                if (start+i > rate/4) wetDifference += (y-x)*(y-x);
                result.push_back(y);
            }
        }
        std::cout << "Mode " << mode << " peak " << peak << " input-difference RMS " << std::sqrt(wetDifference / (total-rate/4)) << '\n';
        check(peak <= 1.00001f, "mode peak above unity with bounded input and unity output");
        check(std::sqrt(wetDifference / (total-rate/4)) > .01, "mode changes synthetic input");
    }
    for (int i=0; i<8; ++i) for (int j=i+1; j<8; ++j)
    {
        double error=0;
        for (int n=rate/4; n<total; ++n)
        {
            const float d=modeOutput[static_cast<size_t>(i)][static_cast<size_t>(n)] - modeOutput[static_cast<size_t>(j)][static_cast<size_t>(n)];
            error += d*d;
        }
        const double rms = std::sqrt(error / (total-rate/4));
        if (rms <= .004) std::cerr << "Mode pair " << i << "/" << j << " difference " << rms << '\n';
        check(rms > .004, "mode outputs differ on synthetic signal");
    }
    // Init must be bounded without relying on output-gain reduction.
    {
        GrainveilDsp::GrainEngine engine; engine.prepare(rate);
        GrainveilDsp::EngineParams p;
        juce::AudioBuffer<float> audio(2, blockSize);
        float peak=0; double difference=0;
        for (int start=0; start<total; start+=blockSize)
        {
            for (int i=0; i<blockSize; ++i) for (int ch=0; ch<2; ++ch) audio.setSample(ch,i,pluck(start+i));
            engine.process(audio,p);
            for (int i=0; i<blockSize; ++i)
            {
                const float y=audio.getSample(0,i), x=pluck(start+i);
                peak=std::max(peak,std::abs(y));
                if (start+i>rate/4) difference+=(y-x)*(y-x);
            }
        }
        std::cout << "Init peak " << peak << " difference RMS " << std::sqrt(difference/(total-rate/4)) << '\n';
        check(peak < 1.0f, "Init stays below unity on synthetic input");
        check(std::sqrt(difference/(total-rate/4)) > .01, "Init changes synthetic input");
    }
    // Test all factory programs with capture enabled; use gain-matched dry
    // subtraction so output attenuation alone cannot masquerade as wet audio.
    for (const auto& preset : GrainveilFactory::presets)
    {
        GrainveilDsp::GrainEngine engine; engine.prepare(rate);
        const auto p=paramsFor(preset);
        juce::AudioBuffer<float> audio(2, blockSize);
        double wetEnergy=0; int count=0; float peak=0; bool finite=true;
        const float dryGain=(1.0f-p.mix)*std::pow(10.0f,p.outGainDb/20.0f);
        for (int start=0; start<total; start+=blockSize)
        {
            for (int i=0; i<blockSize; ++i) for (int ch=0; ch<2; ++ch) audio.setSample(ch,i,pluck(start+i));
            engine.process(audio,p);
            for (int i=0; i<blockSize; ++i)
            {
                const float y=audio.getSample(0,i);
                finite=finite && std::isfinite(y);
                peak=std::max(peak,std::abs(y));
                if (start+i>rate/4) { const float wet=y-dryGain*pluck(start+i); wetEnergy+=wet*wet; ++count; }
            }
        }
        const double wetRms=std::sqrt(wetEnergy/count);
        std::cout << "Preset " << preset.name << " peak " << peak << " wet RMS " << wetRms << '\n';
        check(!p.freeze, "factory preset starts with capture enabled",preset.name);
        check(finite && peak < 1.0f, "factory output is finite and below unity on synthetic input",preset.name);
        check(wetRms > .001, "factory program produces wet signal within 2 seconds",preset.name);
    }
    return failures == 0 ? 0 : 1;
}
