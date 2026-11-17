#pragma once

#include <vector>
#include <algorithm>
#include <cmath>

namespace GrainveilDsp
{
    class CircularBuffer
    {
    public:
        static constexpr double maxCaptureSeconds = 16.0;

        void prepare(double newSampleRate)
        {
            sampleRate = std::max(1.0, newSampleRate);
            const int requiredSamples = static_cast<int>(std::ceil(sampleRate * maxCaptureSeconds)) + 8;

            bufferL.assign(static_cast<size_t>(requiredSamples), 0.0f);
            bufferR.assign(static_cast<size_t>(requiredSamples), 0.0f);
            bufferSize = requiredSamples;
            writePos = 0;
        }

        void clear()
        {
            std::fill(bufferL.begin(), bufferL.end(), 0.0f);
            std::fill(bufferR.begin(), bufferR.end(), 0.0f);
            writePos = 0;
        }

        void writeSample(float left, float right, bool frozen) noexcept
        {
            if (frozen || bufferSize <= 0)
                return;

            bufferL[static_cast<size_t>(writePos)] = left;
            bufferR[static_cast<size_t>(writePos)] = right;

            ++writePos;
            if (writePos >= bufferSize)
                writePos = 0;
        }

        // 4-point, 3rd-order Hermite interpolation at an absolute fractional buffer index
        float readHermiteAt(int channel, double pos) const noexcept
        {
            if (bufferSize <= 4)
                return 0.0f;

            const auto& buf = (channel == 0) ? bufferL : bufferR;
            const double sizeD = static_cast<double>(bufferSize);

            double wrapped = std::fmod(pos, sizeD);
            if (wrapped < 0.0)
                wrapped += sizeD;

            const int i1 = static_cast<int>(wrapped);
            const float frac = static_cast<float>(wrapped - static_cast<double>(i1));

            const int i0 = (i1 - 1 + bufferSize) % bufferSize;
            const int i2 = (i1 + 1) % bufferSize;
            const int i3 = (i1 + 2) % bufferSize;

            const float y0 = buf[static_cast<size_t>(i0)];
            const float y1 = buf[static_cast<size_t>(i1)];
            const float y2 = buf[static_cast<size_t>(i2)];
            const float y3 = buf[static_cast<size_t>(i3)];

            const float c0 = y1;
            const float c1 = 0.5f * (y2 - y0);
            const float c2 = y0 - 2.5f * y1 + 2.0f * y2 - 0.5f * y3;
            const float c3 = 0.5f * (y3 - y0) + 1.5f * (y1 - y2);

            return ((c3 * frac + c2) * frac + c1) * frac + c0;
        }

        // Read at a delay offset (in samples) behind the current write head
        float readDelayed(int channel, double delaySamples) const noexcept
        {
            const double clampedDelay = std::clamp(delaySamples, 1.0, static_cast<double>(bufferSize - 4));
            return readHermiteAt(channel, static_cast<double>(writePos) - clampedDelay);
        }

        int getWritePos() const noexcept { return writePos; }
        int getBufferSize() const noexcept { return bufferSize; }
        double getSampleRate() const noexcept { return sampleRate; }

    private:
        std::vector<float> bufferL;
        std::vector<float> bufferR;
        int bufferSize = 0;
        int writePos = 0;
        double sampleRate = 44100.0;
    };
}
