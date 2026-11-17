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
            double effDensity = densityHz;
            double timingJitterScale = 0.05 + scatter * 0.45;

            switch (modeIndex)
            {
                case 0: effDensity *= 1.40; break; // Cloud: dense wash
                case 1: effDensity *= 0.45; timingJitterScale = 0.02 + scatter * 0.10; break; // EchoGrain: sparser, delay-like
                case 2: effDensity *= 1.15; break; // ReverseSwarm
                case 3: effDensity *= 0.75; break; // IceHold: slow evolution
                case 4: effDensity *= 1.80; timingJitterScale = 0.15 + scatter * 0.65; break; // Fracture: stutter/jumps
                case 5: effDensity *= 1.10; break; // PitchBloom
                case 6: effDensity *= 1.20; timingJitterScale = 0.02 + scatter * 0.15; break; // Smear: stretch
                case 7: effDensity *= 0.90; break; // Ash: coarser lo-fi
                default: break;
            }

            if (samplesUntilNextGrain <= 0.0)
            {
                spawnGrain(buffer, modeIndex, captureTimeMs, grainSizeMs, effDensity,
                           scatter, reverseAmt, pitchSt, pitchRand, spread);

                const double clampedDensity = std::clamp(effDensity, 0.5, 100.0);
                const double baseInterval   = sampleRate / clampedDensity;
                const double timingJitter   = 1.0 + (nextRandomBipolar() * timingJitterScale);
                samplesUntilNextGrain       = std::max(1.0, baseInterval * timingJitter);
            }
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

            double sizeMult = 1.0;
            float effScatter = scatter;
            float effReverse = reverseAmt;
            bool isLofi = false;

            switch (modeIndex)
            {
                case 0: sizeMult = 1.35; effScatter *= 0.7f; break; // Cloud
                case 1: sizeMult = 0.90; effScatter *= 0.25f; break; // EchoGrain
                case 2: sizeMult = 1.25; effReverse = std::max(reverseAmt, 0.75f); break; // ReverseSwarm
                case 3: sizeMult = 1.80; effScatter *= 0.5f; break; // IceHold
                case 4: sizeMult = 0.28; effScatter = std::max(scatter, 0.6f); break; // Fracture
                case 5: sizeMult = 1.20; break; // PitchBloom
                case 6: sizeMult = 2.20; effScatter *= 0.3f; break; // Smear
                case 7: sizeMult = 0.85; effScatter = std::max(scatter, 0.35f); isLofi = true; break; // Ash
                default: break;
            }

            const double sizeJitter   = 1.0 + (nextRandomBipolar() * effScatter * 0.35);
            const double grainSamples = std::clamp(grainSizeMs * sizeMult * 0.001 * sampleRate * sizeJitter,
                                                   16.0, sampleRate * 2.5);

            const double maxCapSamples  = static_cast<double>(buffer.getBufferSize() - 8);
            const double captureSamples = std::clamp(captureTimeMs * 0.001 * sampleRate, 32.0, maxCapSamples);

            double offsetFactor = 0.05 + (nextRandomUnipolar() * (0.10 + effScatter * 0.85));
            if (modeIndex == 1) // EchoGrain: anchor near captureTime
                offsetFactor = 0.85 + (nextRandomBipolar() * effScatter * 0.15);

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
            const float pan = nextRandomBipolar() * std::clamp(spread, 0.0f, 1.0f);

            const double overlap = std::max(1.0, effDensity * (grainSamples / sampleRate));
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
