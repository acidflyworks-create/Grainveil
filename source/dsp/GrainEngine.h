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
        float mix           = 0.5f;
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

            smoothedMix.reset(sampleRate, 0.02);
            smoothedInGain.reset(sampleRate, 0.02);
            smoothedOutGain.reset(sampleRate, 0.02);
            smoothedCaptureMs.reset(sampleRate, 0.05);
            smoothedFeedback.reset(sampleRate, 0.02);
            smoothedFbTone.reset(sampleRate, 0.03);
            smoothedGrainSizeMs.reset(sampleRate, 0.03);
            smoothedDensityHz.reset(sampleRate, 0.03);
            smoothedPitchSt.reset(sampleRate, 0.02);
            smoothedTone.reset(sampleRate, 0.03);

            reset();
        }

        void reset()
        {
            circularBuffer.clear();
            scheduler.reset();
            fbFilter.reset();
            wetToneFilter.reset();
            prevWetL = 0.0f;
            prevWetR = 0.0f;
            samplesUntilTelemetry = 0;
        }

        void process(juce::AudioBuffer<float>& buffer, const EngineParams& params) noexcept
        {
            const int numSamples = buffer.getNumSamples();
            if (numSamples <= 0 || buffer.getNumChannels() < 2)
                return;

            smoothedMix.setTargetValue(params.mix);
            smoothedInGain.setTargetValue(juce::Decibels::decibelsToGain(params.inGainDb));
            smoothedOutGain.setTargetValue(juce::Decibels::decibelsToGain(params.outGainDb));
            smoothedCaptureMs.setTargetValue(params.captureTimeMs);
            smoothedFeedback.setTargetValue(std::clamp(params.feedback, 0.0f, 0.95f));
            smoothedFbTone.setTargetValue(std::clamp(params.fbTone, 0.0f, 1.0f));
            smoothedGrainSizeMs.setTargetValue(params.grainSizeMs);
            smoothedDensityHz.setTargetValue(params.densityHz);
            smoothedPitchSt.setTargetValue(params.pitchSt);
            smoothedTone.setTargetValue(std::clamp(params.tone, 0.0f, 1.0f));

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
                const float toneVal    = smoothedTone.getNextValue();

                const float dryL = left[i];
                const float dryR = right[i];

                // Feedback vs Freeze interaction:
                // When freeze is active, CircularBuffer::writeSample() halts all writes
                // (both live input and feedback recirculation) so the frozen buffer contents
                // remain completely intact and never overwrite or bloom out of control.
                const float fbCutoff = std::min(maxCutoff, 400.0f * std::pow(40.0f, fbToneVal));
                fbFilter.setCutoffFrequency(fbCutoff);

                const float fbL = std::tanh(fbFilter.processSample(0, sanitize(prevWetL)) * 0.85f) * fbAmount;
                const float fbR = std::tanh(fbFilter.processSample(1, sanitize(prevWetR)) * 0.85f) * fbAmount;

                circularBuffer.writeSample(dryL * inGain + fbL, dryR * inGain + fbR, params.freeze);

                float wetL = 0.0f;
                float wetR = 0.0f;
                scheduler.processSample(circularBuffer,
                                        params.mode,
                                        capMs,
                                        sizeMs,
                                        densityHz,
                                        params.scatter,
                                        params.reverseAmt,
                                        pitchSt,
                                        params.pitchRand,
                                        params.spread,
                                        wetL,
                                        wetR);

                const float wetCutoff = std::min(maxCutoff, 600.0f * std::pow(30.0f, toneVal));
                wetToneFilter.setCutoffFrequency(wetCutoff);

                wetL = std::tanh(wetToneFilter.processSample(0, sanitize(wetL)));
                wetR = std::tanh(wetToneFilter.processSample(1, sanitize(wetR)));

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
        juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> smoothedTone;
    };
}
