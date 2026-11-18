#include "dsp/GrainScheduler.h"
#include <algorithm>
#include <cmath>
#include <iostream>

namespace
{
int failures = 0;
void check(bool ok, const char* message, int mode = -1)
{
    if (! ok) { std::cerr << "FAIL: " << message << " (mode " << mode << ")\n"; ++failures; }
}
}

int main()
{
    constexpr double sr = 48000.0;
    GrainveilDsp::CircularBuffer buffer;
    buffer.prepare(sr);
    for (int i = 0; i < 48000; ++i)
        buffer.writeSample(std::sin(i * .13f), std::cos(i * .11f), false);

    for (int mode = 0; mode < 8; ++mode)
    {
        for (float density : { 1.0f, 40.0f })
        {
            GrainveilDsp::GrainScheduler scheduler;
            scheduler.prepare(sr);
            double sum = 0.0;
            int maximum = 0, count = 0;
            for (int i = 0; i < 12000; ++i)
            {
                float l = 0, r = 0;
                scheduler.processSample(buffer, mode, 1000, 5, density, 0.0f, 0, 0, 0, 0, l, r);
                if (i >= 3000)
                {
                    int active = 0;
                    for (const auto& voice : scheduler.getVoices()) active += voice.active ? 1 : 0;
                    maximum = std::max(maximum, active);
                    sum += active;
                    ++count;
                }
            }
            const double average = sum / count;
            if (density == 1.0f)
                check(average > 1.9 && average < 4.1, "minimum density is sparse", mode);
            else
            {
                check(maximum == 32, "maximum density reaches full pool", mode);
                check(average > 30.0, "maximum density keeps pool busy", mode);
            }
            std::cout << "mode " << mode << " density " << density << " active mean " << average << " max " << maximum << '\n';
        }

        for (float scatter : { 0.0f, 1.0f })
        {
            GrainveilDsp::GrainScheduler scheduler;
            scheduler.prepare(sr);
            double lo = 1.0, hi = 0.0;
            int observed = 0, left = 0, right = 0, occupancySamples = 0;
            double occupancy = 0;
            uint64_t lastAge = 0;
            for (int i = 0; i < 4000; ++i)
            {
                float l = 0, r = 0;
                scheduler.processSample(buffer, mode, 1000, 5, 40, scatter, 1, 0, 0, 1, l, r);
                for (const auto& voice : scheduler.getVoices())
                    if (voice.active && voice.age > lastAge)
                    {
                        const double offset = (buffer.getWritePos() - voice.startPos) / sr;
                        lo = std::min(lo, offset); hi = std::max(hi, offset);
                        check(voice.reverse, "100% reverse must reverse every new grain", mode);
                        check(voice.windowType == GrainveilDsp::GrainVoice::Window::edge, "reverse grain uses edge-only envelope", mode);
                        const bool hardLeft = voice.panR == 0.0f && voice.crossR == 0.0f;
                        const bool hardRight = voice.panL == 0.0f && voice.crossL == 0.0f;
                        check(hardLeft || hardRight, "maximum spread must hard-pan", mode);
                        left += hardLeft; right += hardRight;
                        ++observed;
                    }
                int active = 0;
                for (const auto& voice : scheduler.getVoices())
                {
                    lastAge = std::max(lastAge, voice.age);
                    active += voice.active ? 1 : 0;
                }
                if (i > 2000) { occupancy += active; ++occupancySamples; }
            }
            check(occupancy / occupancySamples > 29.0, "maximum density stays busy with scatter", mode);
            check(observed > 50, "observe enough grain starts", mode);
            check(left > 0 && right > 0, "maximum spread uses both sides", mode);
            if (scatter == 0.0f)
            {
                const double anchor = mode == 1 ? .85 : .05;
                check(std::abs(lo - anchor) < 1e-6 && std::abs(hi - anchor) < 1e-6, "zero scatter locks position", mode);
            }
            else
                check(lo < .05 && hi > .95, "maximum scatter spans capture window", mode);
        }
    }

    for (int mode : { 0, 1, 3, 4, 5, 6, 7 })
    {
        GrainveilDsp::GrainScheduler scheduler;
        scheduler.prepare(sr);
        float l = 0, r = 0;
        scheduler.processSample(buffer, mode, 1000, 100, 1, 0, 0, 0, 0, 0, l, r);
        for (const auto& grain : scheduler.getVoices()) if (grain.active)
        {
            const bool edge = mode == 1 || mode == 4 || mode == 7;
            check(grain.windowType == (edge ? GrainveilDsp::GrainVoice::Window::edge : GrainveilDsp::GrainVoice::Window::hann),
                  "mode window selection", mode);
        }
    }

    for (float pitch : { -24.0f, 24.0f })
    {
        GrainveilDsp::GrainScheduler scheduler;
        scheduler.prepare(sr);
        float l = 0, r = 0;
        scheduler.processSample(buffer, 0, 1000, 5, 1, 0, 0, pitch, 0, 0, l, r);
        for (const auto& grain : scheduler.getVoices())
            if (grain.active) check(std::abs(grain.rate - (pitch < 0 ? .25 : 4.0)) < 1e-6, "pitch reaches +/-24 st");
    }
    {
        GrainveilDsp::GrainScheduler scheduler;
        scheduler.prepare(sr);
        double minRate = 4, maxRate = 0;
        for (int i = 0; i < 4000; ++i)
        {
            float l = 0, r = 0;
            scheduler.processSample(buffer, 0, 1000, 5, 40, 0, 0, 0, 1, 0, l, r);
            for (const auto& grain : scheduler.getVoices()) if (grain.active)
            { minRate = std::min(minRate, grain.rate); maxRate = std::max(maxRate, grain.rate); }
        }
        check(minRate < .52 && maxRate > 1.9 && minRate >= .5 && maxRate <= 2.0, "pitch randomness spans +/-12 st");
    }

    // The hard-pan crossfeed must not discard a stereo source's opposite side.
    GrainveilDsp::GrainVoice voice;
    voice.trigger(0, 100, 1, false, false, -1, 1, 1);
    check(voice.panR == 0 && voice.crossR == 0 && voice.crossL > 0, "left pan keeps right source");
    voice.trigger(0, 100, 1, false, false, 1, 1, 1);
    check(voice.panL == 0 && voice.crossL == 0 && voice.crossR > 0, "right pan keeps left source");
    return failures == 0 ? 0 : 1;
}
