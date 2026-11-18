#pragma once

#include "CircularBuffer.h"
#include <cmath>
#include <cstdint>

namespace GrainveilDsp
{
    struct GrainVoice
    {
        bool     active       = false;
        double   startPos     = 0.0;
        double   readPos      = 0.0;
        double   size         = 4410.0; // in samples
        double   rate         = 1.0;
        bool     reverse      = false;
        bool     lofi         = false;
        float    panL         = 0.70710678f;
        float    panR         = 0.70710678f;
        float    crossL       = 0.0f;
        float    crossR       = 0.0f;
        double   envPhase     = 0.0;
        double   envIncrement = 0.0;
        float    amp          = 1.0f;
        uint64_t age          = 0;
        enum class Window { hann, edge };
        Window   windowType   = Window::hann;
        double   edgeSamples  = 1.0;
        bool     swell        = false;
        float    lastL        = 0.0f;
        float    lastR        = 0.0f;
        float    stealL       = 0.0f;
        float    stealR       = 0.0f;
        int      stealFade    = 0;
        bool     freezeKnown  = false;
        bool     wasFrozen    = false;
        int      freezeFade   = 0;
        float    freezeFromL  = 0.0f;
        float    freezeFromR  = 0.0f;
        static constexpr int stealFadeSamples = 64;

        bool isRendering() const noexcept { return active || stealFade > 0 || freezeFade > 0; }

        // Lo-fi sample-and-hold state (used by Ash mode)
        int      holdCounter  = 0;
        float    heldL        = 0.0f;
        float    heldR        = 0.0f;

        void trigger(double initialReadPos,
                     double grainSizeSamples,
                     double playbackRate,
                     bool isReverse,
                     bool isLofi,
                     float panNorm, // -1.0 (L) to +1.0 (R)
                     float amplitude,
                     uint64_t voiceAge,
                     Window newWindow = Window::hann,
                     double requestedEdgeSamples = 64.0,
                     bool useSwell = false) noexcept
        {
            // Preserve the last output as a bounded 64-sample fade when a
            // busy slot is stolen. No extra voice or allocation is required.
            const bool wasRendering = isRendering();
            stealL = wasRendering ? lastL : 0.0f;
            stealR = wasRendering ? lastR : 0.0f;
            stealFade = wasRendering ? stealFadeSamples : 0;
            freezeKnown = false;
            freezeFade = 0;
            active       = true;
            startPos     = initialReadPos;
            readPos      = initialReadPos;
            size         = std::max(16.0, grainSizeSamples);
            rate         = playbackRate;
            reverse      = isReverse;
            lofi         = isLofi;
            envPhase     = 0.0;
            envIncrement = 1.0 / size;
            windowType   = newWindow;
            swell        = useSwell;
            edgeSamples  = std::clamp(requestedEdgeSamples, 1.0, std::max(1.0, (size - 1.0) * 0.5));
            amp          = amplitude;
            age          = voiceAge;
            holdCounter  = 0;
            heldL        = 0.0f;
            heldR        = 0.0f;

            const float angle = (panNorm * 0.5f + 0.5f) * 1.57079632679f;
            const float left = panNorm >= 1.0f ? 0.0f : std::cos(angle);
            const float right = panNorm <= -1.0f ? 0.0f : std::sin(angle);
            // Stereo balance with crossfeed: a hard-panned grain includes both
            // source channels rather than silently discarding the opposite side.
            const float scale = 0.70710678f / std::max(left, right);
            panL = left * scale;
            panR = right * scale;
            crossL = std::max(0.0f, left - right) * scale;
            crossR = std::max(0.0f, right - left) * scale;
        }

        void processSample(const CircularBuffer& buffer, float& outL, float& outR,
                           bool frozen = false, double captureSamples = 0.0) noexcept
        {
            if (! isRendering()) return;
            if (! freezeKnown) { wasFrozen = frozen; freezeKnown = true; }
            else if (wasFrozen != frozen)
            {
                freezeFromL = lastL; freezeFromR = lastR;
                freezeFade = stealFadeSamples;
                wasFrozen = frozen;
            }
            float currentL = 0.0f, currentR = 0.0f;
            if (active)
            {
                constexpr double pi = 3.14159265358979323846;
                float window;
                if (windowType == Window::hann)
                    window = 0.5f * (1.0f - static_cast<float>(std::cos(2.0 * pi * envPhase)));
                else
                {
                    // Flat-topped Tukey: short raised-cosine edges only.
                    const double elapsed = envPhase * size;
                    const double distance = std::min(elapsed, size - 1.0 - elapsed);
                    const double edge = std::clamp(distance / edgeSamples, 0.0, 1.0);
                    window = 0.5f * (1.0f - static_cast<float>(std::cos(pi * edge)));
                }

                // Keep the short click-protection edges, while shaping a
                // separate rising body for ReverseSwarm's backwards swell.
                if (swell)
                    window *= static_cast<float>(0.25 + 0.75 * std::min(1.0, envPhase / 0.8));

                double loopStart = 0.0, loopEnd = 0.0, loopFade = 0.0;
                if (frozen && captureSamples >= 32.0)
                {
                    loopEnd = static_cast<double>(buffer.getWritePos());
                    loopStart = loopEnd - captureSamples;
                    loopFade = std::min(128.0, captureSamples * 0.25);
                    if (readPos < loopStart || readPos >= loopEnd)
                    {
                        double relative = std::fmod(readPos - loopStart, captureSamples);
                        if (relative < 0.0) relative += captureSamples;
                        readPos = loopStart + relative;
                    }
                }
                auto readSample = [&](int channel) noexcept
                {
                    const float main = buffer.readHermiteAt(channel, readPos);
                    if (! frozen || loopFade <= 0.0) return main;
                    if (! reverse && readPos >= loopEnd - loopFade)
                    {
                        const double progress = (readPos - (loopEnd - loopFade)) / loopFade;
                        const float other = buffer.readHermiteAt(channel, loopStart + progress * loopFade);
                        return main + static_cast<float>(progress) * (other - main);
                    }
                    if (reverse && readPos <= loopStart + loopFade)
                    {
                        const double progress = ((loopStart + loopFade) - readPos) / loopFade;
                        const float other = buffer.readHermiteAt(channel, loopEnd - progress * loopFade);
                        return main + static_cast<float>(progress) * (other - main);
                    }
                    return main;
                };
                float sampleL = 0.0f, sampleR = 0.0f;
                if (lofi)
                {
                    if (holdCounter <= 0)
                    {
                        heldL = readSample(0);
                        heldR = readSample(1);
                        holdCounter = 8; // Ash: coarser in-grain sample-and-hold
                    }
                    --holdCounter;
                    sampleL = heldL; sampleR = heldR;
                }
                else
                {
                    sampleL = readSample(0);
                    sampleR = readSample(1);
                }
                const float gain = window * amp;
                currentL = (sampleL * panL + sampleR * crossL) * gain;
                currentR = (sampleR * panR + sampleL * crossR) * gain;
                readPos += reverse ? -rate : rate;
                if (frozen && loopFade > 0.0)
                {
                    if (readPos >= loopEnd) readPos -= captureSamples - loopFade;
                    else if (readPos < loopStart) readPos += captureSamples - loopFade;
                }
                envPhase += envIncrement;
                if (envPhase >= 1.0) active = false;
            }
            if (stealFade > 0)
            {
                const float fade = static_cast<float>(stealFade) / static_cast<float>(stealFadeSamples);
                currentL += stealL * fade;
                currentR += stealR * fade;
                --stealFade;
            }
            if (freezeFade > 0)
            {
                const float fade = static_cast<float>(freezeFade) / static_cast<float>(stealFadeSamples);
                currentL = freezeFromL * fade + currentL * (1.0f - fade);
                currentR = freezeFromR * fade + currentR * (1.0f - fade);
                --freezeFade;
            }
            lastL = currentL;
            lastR = currentR;
            outL += currentL;
            outR += currentR;
        }
    };
}
