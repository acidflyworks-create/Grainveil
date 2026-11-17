#pragma once

#include <array>
#include <atomic>
#include <cstdint>
#include <cstddef>

namespace GrainveilDsp
{
    struct TelemetrySnapshot
    {
        static constexpr int waveformPoints = 128;
        static constexpr int maxGrains = 32;
        std::array<float, waveformPoints> waveform {};
        std::array<float, maxGrains> grainPositions {};
        int activeGrainCount = 0;
        bool frozen = false;
        int mode = 0;
    };

    // Single audio-thread writer, any number of readers. Every shared field is
    // atomic, and a sequence counter rejects overlapping publications. This
    // avoids the undefined concurrent struct copy of the previous handoff.
    // seq_cst gives one total order for both the counter and fields. The audio
    // thread never waits; the reader makes at most three attempts.
    class TelemetryExchange
    {
    public:
        static_assert(std::atomic<float>::is_always_lock_free, "Telemetry requires lock-free float atomics");
        static_assert(std::atomic<uint32_t>::is_always_lock_free, "Telemetry requires lock-free integer atomics");

        void publish(const TelemetrySnapshot& frame) noexcept
        {
            sequence.fetch_add(1);
            for (size_t i = 0; i < waveform.size(); ++i) waveform[i].store(frame.waveform[i]);
            for (size_t i = 0; i < grains.size(); ++i) grains[i].store(frame.grainPositions[i]);
            grainCount.store(static_cast<uint32_t>(frame.activeGrainCount));
            frozen.store(frame.frozen ? 1u : 0u);
            mode.store(static_cast<uint32_t>(frame.mode));
            sequence.fetch_add(1);
        }

        bool copy(TelemetrySnapshot& destination) const noexcept
        {
            for (int attempt = 0; attempt < 3; ++attempt)
            {
                const auto before = sequence.load();
                if (before == 0 || (before & 1u) != 0) continue;
                TelemetrySnapshot frame;
                for (size_t i = 0; i < waveform.size(); ++i) frame.waveform[i] = waveform[i].load();
                for (size_t i = 0; i < grains.size(); ++i) frame.grainPositions[i] = grains[i].load();
                frame.activeGrainCount = static_cast<int>(grainCount.load());
                frame.frozen = frozen.load() != 0;
                frame.mode = static_cast<int>(mode.load());
                if (sequence.load() == before)
                {
                    destination = frame;
                    return true;
                }
            }
            return false;
        }

    private:
        std::array<std::atomic<float>, TelemetrySnapshot::waveformPoints> waveform {};
        std::array<std::atomic<float>, TelemetrySnapshot::maxGrains> grains {};
        std::atomic<uint32_t> grainCount { 0 }, frozen { 0 }, mode { 0 }, sequence { 0 };
    };
}
