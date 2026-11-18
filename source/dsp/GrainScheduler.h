#pragma once

#include "GrainVoice.h"
#include "CircularBuffer.h"
#include "ModeMapping.h"
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
                v = GrainVoice {};

            samplesUntilNextGrain = 0.0;
            ageCounter = 0;
            crawlPhase = 0.0;
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
                           float& wetR,
                           bool frozen = false) noexcept
        {
            // Endpoint-preserving multiplicative mode bias keeps sparse/full
            // density endpoints while making the centre distinct in each mode.
            const auto& mapping = modeMapping(modeIndex);
            const double density = std::clamp((densityHz - 1.0) / 39.0, 0.0, 1.0);
            const float effectiveScatter = static_cast<float>(bias01(scatter, mapping.scatter));
            const double timingJitterScale = mapping.timingBase + effectiveScatter * mapping.timingScatter;
            const double shaped = bias01(density, mapping.density);
            const double requestedOverlap = 3.0 + 61.0 * shaped;
            const double nominalSamples = std::clamp(modeGrainMs(modeIndex, grainSizeMs) * 0.001 * sampleRate,
                                                      16.0, sampleRate * 2.5);
            crawlPhase += 0.09 / sampleRate;
            if (crawlPhase >= 1.0) crawlPhase -= 1.0;
            const double effDensity = std::min(requestedOverlap * sampleRate / nominalSamples, sampleRate * 4.0);
            const double baseInterval = sampleRate / effDensity;

            // At short grain sizes more than one start per sample may be needed
            // to fill the pool. The fixed bound prevents unbounded audio work.
            int starts = 0;
            while (samplesUntilNextGrain <= 0.0 && starts < 4)
            {
                spawnGrain(buffer, modeIndex, captureTimeMs, grainSizeMs, effDensity,
                           effectiveScatter, reverseAmt, pitchSt, pitchRand, spread, frozen);
                const double timingJitter = 1.0 + (nextRandomBipolar() * timingJitterScale);
                samplesUntilNextGrain += std::max(0.25, baseInterval * timingJitter);
                ++starts;
            }
            if (starts == 4 && samplesUntilNextGrain <= 0.0)
                samplesUntilNextGrain = 0.0; // discard backlog, never accumulate unbounded work
            samplesUntilNextGrain -= 1.0;

            wetL = 0.0f;
            wetR = 0.0f;

            const double captureSamples = std::clamp(captureTimeMs * 0.001 * sampleRate, 32.0,
                                                      static_cast<double>(buffer.getBufferSize() - 8));
            for (auto& v : voices)
            {
                if (v.isRendering())
                    v.processSample(buffer, wetL, wetR, frozen, captureSamples);
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
                        float spread,
                        bool frozen) noexcept
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

            const auto& mapping = modeMapping(modeIndex);
            const float effScatter = scatter;
            const float biasedReverse = static_cast<float>(bias01(reverseAmt, mapping.reverse));
            const float effReverse = modeIndex == 2 ? std::max(biasedReverse, 0.70f) : biasedReverse;
            const float effPitchRand = static_cast<float>(bias01(pitchRand, mapping.pitchRandom));
            const bool isLofi = modeIndex == 7;
            const double sizeJitter = 1.0 + (nextRandomBipolar() * effScatter * 0.35);
            double actualMs = modeGrainMs(modeIndex, grainSizeMs) * sizeJitter;
            if (modeIndex == 4) actualMs = std::clamp(actualMs, 5.0, 20.0);
            const double grainSamples = std::clamp(actualMs * 0.001 * sampleRate, 16.0, sampleRate * 2.5);

            const double maxCapSamples  = static_cast<double>(buffer.getBufferSize() - 8);
            const double captureSamples = std::clamp(captureTimeMs * 0.001 * sampleRate, 32.0, maxCapSamples);

            // Zero scatter locks the start to the mode's anchor; at maximum it
            // spans the entire capture window, including in EchoGrain mode.
            const double anchor = mapping.anchor;
            const double scatterCurve = std::pow(std::clamp(static_cast<double>(effScatter), 0.0, 1.0), 0.65);
            double offsetFactor = anchor + scatterCurve * (nextRandomUnipolar() - anchor);
            // IceHold moves slowly even with a frozen write head, without
            // touching the captured samples or adding any processing buffers.
            const double crawl = modeIndex == 3 && frozen ? std::sin(crawlPhase * 6.283185307179586) : 0.0;
            offsetFactor = std::clamp(offsetFactor + crawl * 0.03, 0.0, 1.0);
            const double startPos = static_cast<double>(buffer.getWritePos()) - (offsetFactor * captureSamples);

            // Pitch calculation
            double totalSemitones = static_cast<double>(pitchSt);
            if (modeIndex == 5) // Weighted fifths and octaves with random detune.
            {
                static constexpr float intervals[8] = { 7.0f, 12.0f, 7.0f, 12.0f, -12.0f, 19.0f, 24.0f, 0.0f };
                if (nextRandomUnipolar() < std::max(effPitchRand, 0.75f))
                {
                    const int idx = static_cast<int>(nextRandomUnipolar() * 8.0f);
                    totalSemitones += static_cast<double>(intervals[idx]);
                }
                totalSemitones += static_cast<double>(nextRandomBipolar() * 12.0f * effPitchRand * effPitchRand);
            }
            else if (modeIndex == 3)
            {
                totalSemitones += static_cast<double>(nextRandomBipolar() * (0.08f + effPitchRand * 12.0f)) + crawl * 0.25;
            }
            else
            {
                totalSemitones += static_cast<double>(nextRandomBipolar() * effPitchRand * 12.0f);
            }

            const double rate = std::clamp(std::pow(2.0, totalSemitones / 12.0), 0.25, 4.0);
            const bool isReverse = (nextRandomUnipolar() < effReverse);
            // At full spread every grain is hard-panned; at zero all are centred.
            const float pan = (nextRandomUnipolar() < 0.5f ? -1.0f : 1.0f) * std::clamp(spread, 0.0f, 1.0f);

            const double overlap = std::clamp(effDensity * (grainSamples / sampleRate), 1.0, static_cast<double>(poolSize));
            const bool shortEdges = isReverse || modeIndex == 1 || modeIndex == 4 || modeIndex == 7;
            // Hann averages less power than a short-edge window. Makeup is
            // bounded and followed by wet/feedback saturation in the engine.
            const float amp = (shortEdges ? 1.5f : 2.2f) / std::sqrt(static_cast<float>(overlap));
            const auto window = shortEdges ? GrainVoice::Window::edge : GrainVoice::Window::hann;
            const double edgeSamples = sampleRate * (isReverse ? 0.003 : 0.001);
            target->trigger(startPos, grainSamples, rate, isReverse, isLofi, pan, amp, ++ageCounter,
                            window, edgeSamples, modeIndex == 2 && isReverse);
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
        double crawlPhase = 0.0;
        uint64_t ageCounter = 0;
        uint32_t rngState = 0x12345678u;
    };
}
