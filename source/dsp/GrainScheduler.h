#pragma once

#include "GrainVoice.h"
#include "CircularBuffer.h"
#include <array>
#include <algorithm>
#include <cmath>
#include <cstdint>

namespace GrainveilDsp
{
    class GrainScheduler
    {
    public:
        static constexpr size_t poolSize = 32;

        void prepare(double newSampleRate) noexcept
        {
            sampleRate = std::max(1.0, newSampleRate);
            reset();
        }

        void reset() noexcept
        {
            for (auto& v : voices)
                v.active = false;

            samplesUntilNextGrain = 0.0;
            ageCounter = 0;
        }

        void processSample(const CircularBuffer& buffer,
                           int modeIndex,
                           double captureTimeMs,
                           double grainSizeMs,
                           double densityHz,
                           float scatter,
                           float reverseAmt,
                           float pitchSt,
                           float pitchRand,
                           float spread,
                           float& wetL,
                           float& wetR) noexcept
        {
            // Keep the existing 1..40 parameter/automation range, but map it to
            // overlap rather than an absolute Hz rate. The old mode density
            // multipliers shape the response between common sparse/full endpoints.
            const double density = std::clamp((densityHz - 1.0) / 39.0, 0.0, 1.0);
            double modeDensity = 1.0;
            double timingJitterScale = 0.05 + scatter * 0.45;
            switch (modeIndex)
            {
                case 0: modeDensity = 1.40; break;
                case 1: modeDensity = 0.45; timingJitterScale = 0.02 + scatter * 0.10; break;
                case 2: modeDensity = 1.15; break;
                case 3: modeDensity = 0.75; break;
                case 4: modeDensity = 1.80; timingJitterScale = 0.15 + scatter * 0.65; break;
                case 5: modeDensity = 1.10; break;
                case 6: modeDensity = 1.20; timingJitterScale = 0.02 + scatter * 0.15; break;
                case 7: modeDensity = 0.90; break;
                default: break;
            }
            const double shaped = density * modeDensity / (1.0 + (modeDensity - 1.0) * density);
            const double requestedOverlap = 3.0 + 61.0 * shaped;
            const double nominalSamples = std::clamp(grainSizeMs * modeSize(modeIndex) * 0.001 * sampleRate,
                                                      16.0, sampleRate * 2.5);
            const double effDensity = std::min(requestedOverlap * sampleRate / nominalSamples, sampleRate * 4.0);
            const double baseInterval = sampleRate / effDensity;

            // At short grain sizes more than one start per sample may be needed
            // to fill the pool. The fixed bound prevents unbounded audio work.
            int starts = 0;
            while (samplesUntilNextGrain <= 0.0 && starts < 4)
            {
                spawnGrain(buffer, modeIndex, captureTimeMs, grainSizeMs, effDensity,
                           scatter, reverseAmt, pitchSt, pitchRand, spread);
                const double timingJitter = 1.0 + (nextRandomBipolar() * timingJitterScale);
                samplesUntilNextGrain += std::max(0.25, baseInterval * timingJitter);
                ++starts;
            }
            if (starts == 4 && samplesUntilNextGrain <= 0.0)
                samplesUntilNextGrain = 0.0; // discard backlog, never accumulate unbounded work
            samplesUntilNextGrain -= 1.0;

            wetL = 0.0f;
            wetR = 0.0f;

            for (auto& v : voices)
            {
                if (v.active)
                    v.processSample(buffer, wetL, wetR);
            }
        }

        const std::array<GrainVoice, poolSize>& getVoices() const noexcept { return voices; }

    private:
        static double modeSize(int mode) noexcept
        {
            switch (mode)
            {
                case 0: return 1.35;
                case 1: return 0.90;
                case 2: return 1.25;
                case 3: return 1.80;
                case 4: return 0.28;
                case 5: return 1.20;
                case 6: return 2.20;
                case 7: return 0.85;
                default: return 1.0;
            }
        }

        void spawnGrain(const CircularBuffer& buffer,
                        int modeIndex,
                        double captureTimeMs,
                        double grainSizeMs,
                        double effDensity,
                        float scatter,
                        float reverseAmt,
                        float pitchSt,
                        float pitchRand,
                        float spread) noexcept
        {
            GrainVoice* target = nullptr;
            uint64_t oldestAge = UINT64_MAX;

            for (auto& v : voices)
            {
                if (!v.active)
                {
                    target = &v;
                    break;
                }
                if (v.age < oldestAge)
                {
                    oldestAge = v.age;
                    target = &v;
                }
            }

            if (target == nullptr)
                return;

            const double sizeMult = modeSize(modeIndex);
            float effScatter = scatter;
            float effReverse = reverseAmt;
            bool isLofi = false;

            // Preserve mode-specific grain size, jitter, reverse and lo-fi rules.
            switch (modeIndex)
            {
                case 0: effScatter *= 0.7f; break;
                case 1: effScatter *= 0.25f; break;
                case 2: effReverse = std::max(reverseAmt, 0.75f); break;
                case 3: effScatter *= 0.5f; break;
                case 4: effScatter = std::max(scatter, 0.6f); break;
                case 6: effScatter *= 0.3f; break;
                case 7: effScatter = std::max(scatter, 0.35f); isLofi = true; break;
                default: break;
            }

            const double sizeJitter   = 1.0 + (nextRandomBipolar() * effScatter * 0.35);
            const double grainSamples = std::clamp(grainSizeMs * sizeMult * 0.001 * sampleRate * sizeJitter,
                                                   16.0, sampleRate * 2.5);

            const double maxCapSamples  = static_cast<double>(buffer.getBufferSize() - 8);
            const double captureSamples = std::clamp(captureTimeMs * 0.001 * sampleRate, 32.0, maxCapSamples);

            // Zero scatter locks the start to the mode's anchor; at maximum it
            // spans the entire capture window, including in EchoGrain mode.
            const double anchor = modeIndex == 1 ? 0.85 : 0.05;
            const double scatterCurve = std::pow(std::clamp(static_cast<double>(scatter), 0.0, 1.0), 0.65);
            const double offsetFactor = anchor + scatterCurve * (nextRandomUnipolar() - anchor);

            const double startPos = static_cast<double>(buffer.getWritePos()) - (offsetFactor * captureSamples);

            // Pitch calculation
            double totalSemitones = static_cast<double>(pitchSt);
            if (modeIndex == 5) // PitchBloom: musical intervals
            {
                static constexpr float intervals[6] = { 0.0f, 7.0f, 12.0f, -12.0f, 19.0f, 24.0f };
                const float prob = std::max(pitchRand, 0.35f);
                if (nextRandomUnipolar() < prob)
                {
                    const int idx = static_cast<int>(nextRandomUnipolar() * 5.99f);
                    totalSemitones += static_cast<double>(intervals[idx]);
                }
            }
            else if (modeIndex == 3) // IceHold: subtle detune
            {
                totalSemitones += static_cast<double>(nextRandomBipolar() * (0.08f + pitchRand * 12.0f));
            }
            else
            {
                totalSemitones += static_cast<double>(nextRandomBipolar() * pitchRand * 12.0f);
            }

            const double rate = std::clamp(std::pow(2.0, totalSemitones / 12.0), 0.25, 4.0);
            const bool isReverse = (nextRandomUnipolar() < effReverse);
            // At full spread every grain is hard-panned; at zero all are centred.
            const float pan = (nextRandomUnipolar() < 0.5f ? -1.0f : 1.0f) * std::clamp(spread, 0.0f, 1.0f);

            const double overlap = std::clamp(effDensity * (grainSamples / sampleRate), 1.0, static_cast<double>(poolSize));
            const float amp      = 1.0f / std::sqrt(static_cast<float>(overlap));

            target->trigger(startPos, grainSamples, rate, isReverse, isLofi, pan, amp, ++ageCounter);
        }

        float nextRandomUnipolar() noexcept
        {
            rngState ^= rngState << 13;
            rngState ^= rngState >> 17;
            rngState ^= rngState << 5;
            return static_cast<float>(rngState & 0x00FFFFFF) / static_cast<float>(0x01000000);
        }

        float nextRandomBipolar() noexcept
        {
            return nextRandomUnipolar() * 2.0f - 1.0f;
        }

        std::array<GrainVoice, poolSize> voices;
        double sampleRate = 44100.0;
        double samplesUntilNextGrain = 0.0;
        uint64_t ageCounter = 0;
        uint32_t rngState = 0x12345678u;
    };
}
