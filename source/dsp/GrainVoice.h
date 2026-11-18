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
                     uint64_t voiceAge) noexcept
        {
            active       = true;
            startPos     = initialReadPos;
            readPos      = initialReadPos;
            size         = std::max(16.0, grainSizeSamples);
            rate         = playbackRate;
            reverse      = isReverse;
            lofi         = isLofi;
            envPhase     = 0.0;
            envIncrement = 1.0 / size;
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

        void processSample(const CircularBuffer& buffer, float& outL, float& outR) noexcept
        {
            if (!active)
                return;

            // Hann window: 0.5 * (1 - cos(2*pi*phase)), guaranteed 0 at phase=0 and phase=1
            constexpr double twoPi = 6.28318530717958647692;
            const float window = 0.5f * (1.0f - static_cast<float>(std::cos(twoPi * envPhase)));

            float sampleL = 0.0f;
            float sampleR = 0.0f;

            if (lofi)
            {
                if (holdCounter <= 0)
                {
                    heldL = buffer.readHermiteAt(0, readPos);
                    heldR = buffer.readHermiteAt(1, readPos);
                    holdCounter = 4; // 4x downsample-in-grain for lo-fi grit
                }
                --holdCounter;
                sampleL = heldL;
                sampleR = heldR;
            }
            else
            {
                sampleL = buffer.readHermiteAt(0, readPos);
                sampleR = buffer.readHermiteAt(1, readPos);
            }

            const float gain = window * amp;
            outL += (sampleL * panL + sampleR * crossL) * gain;
            outR += (sampleR * panR + sampleL * crossR) * gain;

            readPos += reverse ? -rate : rate;
            envPhase += envIncrement;

            if (envPhase >= 1.0)
                active = false;
        }
    };
}
