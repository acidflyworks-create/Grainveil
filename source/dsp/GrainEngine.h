#pragma once

#include "CircularBuffer.h"
#include "GrainScheduler.h"
#include "TelemetryExchange.h"
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_dsp/juce_dsp.h>
#include <algorithm>
#include <array>
#include <atomic>
#include <cmath>
#include <cstdint>

namespace GrainveilDsp
{
    struct EngineParams
    {
        float mix           = 0.6f;
        float inGainDb      = 0.0f;
        float outGainDb     = 0.0f;
        bool  freeze        = false;
        float captureTimeMs = 1000.0f;
        float feedback      = 0.25f;
        float fbTone        = 0.5f;
        float grainSizeMs   = 120.0f;
        float densityHz     = 10.0f;
        float scatter       = 0.2f;
        float reverseAmt    = 0.0f;
        float pitchSt       = 0.0f;
        float pitchRand     = 0.0f;
        float spread        = 0.5f;
        float tone          = 0.7f;
        int   mode          = 0;
    };

    static_assert(GrainScheduler::poolSize == TelemetrySnapshot::maxGrains, "Telemetry grain capacity must match the scheduler");

    class GrainEngine
    {
    public:
        void prepare(double newSampleRate)
        {
            sampleRate = std::max(1.0, newSampleRate);
            circularBuffer.prepare(sampleRate);
            scheduler.prepare(sampleRate);

            juce::dsp::ProcessSpec spec;
            spec.sampleRate       = sampleRate;
            spec.maximumBlockSize = 1;
            spec.numChannels      = 2;

            fbFilter.prepare(spec);
            fbFilter.setType(juce::dsp::StateVariableTPTFilterType::lowpass);
            fbFilter.setResonance(0.7071f);

            wetToneFilter.prepare(spec);
            wetToneFilter.setType(juce::dsp::StateVariableTPTFilterType::lowpass);
            wetToneFilter.setResonance(0.7071f);
            feedbackDc.prepare(sampleRate);
            wetDc.prepare(sampleRate);

            smoothedMix.reset(sampleRate, 0.02);
            smoothedInGain.reset(sampleRate, 0.02);
            smoothedOutGain.reset(sampleRate, 0.02);
            smoothedCaptureMs.reset(sampleRate, 0.05);
            smoothedFeedback.reset(sampleRate, 0.02);
            smoothedFbTone.reset(sampleRate, 0.03);
            smoothedGrainSizeMs.reset(sampleRate, 0.03);
            smoothedDensityHz.reset(sampleRate, 0.03);
            smoothedPitchSt.reset(sampleRate, 0.02);
            smoothedScatter.reset(sampleRate, 0.03);
            smoothedPitchRand.reset(sampleRate, 0.03);
            smoothedReverse.reset(sampleRate, 0.03);
            smoothedSpread.reset(sampleRate, 0.03);
            smoothedTone.reset(sampleRate, 0.03);

            reset();
        }

        void reset()
        {
            circularBuffer.clear();
            scheduler.reset();
            fbFilter.reset();
            wetToneFilter.reset();
            feedbackDc.reset();
            wetDc.reset();
            prevWetL = 0.0f;
            prevWetR = 0.0f;
            samplesUntilTelemetry = 0;
        }

        void process(juce::AudioBuffer<float>& buffer, const EngineParams& params) noexcept
        {
            juce::ScopedNoDenormals noDenormals;
            const int numSamples = buffer.getNumSamples();
            if (numSamples <= 0 || buffer.getNumChannels() < 2)
                return;

            auto safe = [](float v, float fallback, float lo, float hi) noexcept
            { return std::clamp(std::isfinite(v) ? v : fallback, lo, hi); };
            smoothedMix.setTargetValue(safe(params.mix, .6f, 0.0f, 1.0f));
            smoothedInGain.setTargetValue(juce::Decibels::decibelsToGain(safe(params.inGainDb, 0, -24, 12)));
            smoothedOutGain.setTargetValue(juce::Decibels::decibelsToGain(safe(params.outGainDb, 0, -24, 12)));
            smoothedCaptureMs.setTargetValue(safe(params.captureTimeMs, 1000, 50, 4000));
            smoothedFeedback.setTargetValue(safe(params.feedback, 0, 0, .95f));
            smoothedFbTone.setTargetValue(safe(params.fbTone, .5f, 0, 1));
            smoothedGrainSizeMs.setTargetValue(safe(params.grainSizeMs, 120, 5, 1200));
            smoothedDensityHz.setTargetValue(safe(params.densityHz, 10, 1, 40));
            smoothedPitchSt.setTargetValue(safe(params.pitchSt, 0, -24, 24));
            smoothedScatter.setTargetValue(safe(params.scatter, 0, 0, 1));
            smoothedPitchRand.setTargetValue(safe(params.pitchRand, 0, 0, 1));
            smoothedReverse.setTargetValue(safe(params.reverseAmt, 0, 0, 1));
            smoothedSpread.setTargetValue(safe(params.spread, .5f, 0, 1));
            smoothedTone.setTargetValue(safe(params.tone, .7f, 0, 1));

            auto* left  = buffer.getWritePointer(0);
            auto* right = buffer.getWritePointer(1);

            const float maxCutoff = static_cast<float>(std::min(18000.0, sampleRate * 0.45));

            for (int i = 0; i < numSamples; ++i)
            {
                const float wetMix     = smoothedMix.getNextValue();
                const float dryMix     = 1.0f - wetMix;
                const float inGain     = smoothedInGain.getNextValue();
                const float outGain    = smoothedOutGain.getNextValue();
                const double capMs     = static_cast<double>(smoothedCaptureMs.getNextValue());
                const float fbAmount   = smoothedFeedback.getNextValue();
                const float fbToneVal  = smoothedFbTone.getNextValue();
                const double sizeMs    = static_cast<double>(smoothedGrainSizeMs.getNextValue());
                const double densityHz = static_cast<double>(smoothedDensityHz.getNextValue());
                const float pitchSt    = smoothedPitchSt.getNextValue();
                const float scatter    = smoothedScatter.getNextValue();
                const float pitchRand  = smoothedPitchRand.getNextValue();
                const float reverse    = smoothedReverse.getNextValue();
                const float spread     = smoothedSpread.getNextValue();
                const float toneVal    = smoothedTone.getNextValue();

                // Bound extreme finite host input as well as NaN/Inf before it
                // can poison the capture buffer or filter state.
                const float dryL = std::clamp(sanitize(left[i]), -32.0f, 32.0f);
                const float dryR = std::clamp(sanitize(right[i]), -32.0f, 32.0f);

                // Feedback vs Freeze interaction:
                // When freeze is active, CircularBuffer::writeSample() halts all writes
                // (both live input and feedback recirculation) so the frozen buffer contents
                // remain completely intact and never overwrite or bloom out of control.
                const float fbCutoff = std::min(maxCutoff, 400.0f * std::pow(7.5f, std::min(1.0f, fbToneVal * 2.0f)));
                fbFilter.setCutoffFrequency(fbCutoff);
                const float fbLowL = fbFilter.processSample(0, sanitize(prevWetL));
                const float fbLowR = fbFilter.processSample(1, sanitize(prevWetR));
                const float fbHighGain = 2.5f * std::max(0.0f, fbToneVal * 2.0f - 1.0f);
                const float colourL = fbLowL + fbHighGain * (sanitize(prevWetL) - fbLowL);
                const float colourR = fbLowR + fbHighGain * (sanitize(prevWetR) - fbLowR);
                // 2x drive makes the upper feedback range capable of blooming;
                // both clip stages bound the loop even when it self-oscillates.
                const float fbL = std::tanh(feedbackDc.process(0, std::tanh(2.0f * sanitize(colourL)))) * fbAmount;
                const float fbR = std::tanh(feedbackDc.process(1, std::tanh(2.0f * sanitize(colourR)))) * fbAmount;

                circularBuffer.writeSample(dryL * inGain + fbL, dryR * inGain + fbR, params.freeze);

                float wetL = 0.0f;
                float wetR = 0.0f;
                scheduler.processSample(circularBuffer,
                                        params.mode,
                                        capMs,
                                        sizeMs,
                                        densityHz,
                                        scatter,
                                        reverse,
                                        pitchSt,
                                        pitchRand,
                                        spread,
                                        wetL,
                                        wetR,
                                        params.freeze);

                // Lower half sweeps a dark low-pass; upper half adds a high
                // shelf, so the bright end can be genuinely aggressive.
                const float wetCutoff = std::min(maxCutoff, 600.0f * std::pow(5.0f, std::min(1.0f, toneVal * 2.0f)));
                wetToneFilter.setCutoffFrequency(wetCutoff);
                const float lowL = wetToneFilter.processSample(0, sanitize(wetL));
                const float lowR = wetToneFilter.processSample(1, sanitize(wetR));
                const float highs = 3.0f * std::max(0.0f, toneVal * 2.0f - 1.0f);
                const float colouredL = sanitize(lowL + highs * (sanitize(wetL) - lowL));
                const float colouredR = sanitize(lowR + highs * (sanitize(wetR) - lowR));
                wetL = std::tanh(wetDc.process(0, colouredL));
                wetR = std::tanh(wetDc.process(1, colouredR));

                prevWetL = wetL;
                prevWetR = wetR;

                left[i]  = (dryL * dryMix + wetL * wetMix) * outGain;
                right[i] = (dryR * dryMix + wetR * wetMix) * outGain;
            }

            // Telemetry alone is decimated to about 30 Hz; audio processing is unchanged.
            samplesUntilTelemetry -= numSamples;
            if (samplesUntilTelemetry <= 0)
            {
                updateTelemetry(params.freeze, params.mode);
                samplesUntilTelemetry = std::max(1, static_cast<int>(sampleRate / 30.0));
            }
        }

        bool copyTelemetry(TelemetrySnapshot& dest) const noexcept
        {
            return telemetryExchange.copy(dest);
        }

    private:
        struct DcBlocker
        {
            void prepare(double rate) noexcept
            {
                coefficient = static_cast<float>(std::exp(-2.0 * 3.14159265358979323846 * 15.0 / rate));
                reset();
            }
            void reset() noexcept { previousInput.fill(0.0f); previousOutput.fill(0.0f); }
            float process(size_t channel, float input) noexcept
            {
                input = std::isfinite(input) ? input : 0.0f;
                float output = input - previousInput[channel] + coefficient * previousOutput[channel];
                if (! std::isfinite(output) || std::abs(output) < 1.0e-20f) output = 0.0f;
                previousInput[channel] = input;
                previousOutput[channel] = output;
                return output;
            }
            std::array<float, 2> previousInput {}, previousOutput {};
            float coefficient = 0.0f;
        };

        void updateTelemetry(bool frozen, int mode) noexcept
        {
            TelemetrySnapshot telemetry;
            const int bufSize = circularBuffer.getBufferSize();
            const int writePos = circularBuffer.getWritePos();
            // Render the active capture window, not the whole 16-second capacity.
            // The write cursor points to the next slot, hence the newest sample is -1.
            const int windowSamples = std::clamp(static_cast<int>(sampleRate * 0.001 * smoothedCaptureMs.getCurrentValue()),
                                                 32, std::max(32, bufSize - 8));
            if (bufSize > 0)
            {
                for (int i = 0; i < TelemetrySnapshot::waveformPoints; ++i)
                {
                    const double offset = static_cast<double>(i) * (windowSamples - 1) / (TelemetrySnapshot::waveformPoints - 1);
                    const double pos = static_cast<double>(writePos - 1) - offset;
                    const float sL = circularBuffer.readHermiteAt(0, pos);
                    const float sR = circularBuffer.readHermiteAt(1, pos);
                    telemetry.waveform[static_cast<size_t>(TelemetrySnapshot::waveformPoints - 1 - i)] = 0.5f * (sL + sR);
                }
            }

            int count = 0;
            const double bufSizeD = static_cast<double>(std::max(1, bufSize));
            for (const auto& v : scheduler.getVoices())
            {
                if (v.active && count < static_cast<int>(GrainScheduler::poolSize))
                {
                    double rel = std::fmod(static_cast<double>(writePos) - v.readPos, bufSizeD);
                    if (rel < 0.0)
                        rel += bufSizeD;
                    if (rel <= static_cast<double>(windowSamples))
                        telemetry.grainPositions[static_cast<size_t>(count++)] =
                            static_cast<float>(std::clamp(1.0 - (rel / static_cast<double>(windowSamples)), 0.0, 1.0));
                }
            }
            telemetry.activeGrainCount = count;
            telemetry.frozen = frozen;
            telemetry.mode = mode;

            telemetryExchange.publish(telemetry);
        }

        static float sanitize(float x) noexcept
        {
            return std::isfinite(x) ? x : 0.0f;
        }

        CircularBuffer circularBuffer;
        GrainScheduler scheduler;
        juce::dsp::StateVariableTPTFilter<float> fbFilter;
        juce::dsp::StateVariableTPTFilter<float> wetToneFilter;
        DcBlocker feedbackDc, wetDc;

        double sampleRate = 44100.0;
        float prevWetL = 0.0f;
        float prevWetR = 0.0f;

        TelemetryExchange telemetryExchange;
        int samplesUntilTelemetry = 0;

        juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> smoothedMix;
        juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> smoothedInGain;
        juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> smoothedOutGain;
        juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> smoothedCaptureMs;
        juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> smoothedFeedback;
        juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> smoothedFbTone;
        juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> smoothedGrainSizeMs;
        juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> smoothedDensityHz;
        juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> smoothedPitchSt;
        juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> smoothedScatter;
        juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> smoothedPitchRand;
        juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> smoothedReverse;
        juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> smoothedSpread;
        juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> smoothedTone;
    };
}
