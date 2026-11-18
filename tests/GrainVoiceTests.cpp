#include "dsp/GrainVoice.h"
#include <algorithm>
#include <cmath>
#include <iostream>

namespace
{
int failures = 0;
void check(bool ok, const char* message)
{
    if (! ok) { std::cerr << "FAIL: " << message << '\n'; ++failures; }
}
}

int main()
{
    constexpr double sr = 48000.0;
    GrainveilDsp::CircularBuffer constant, ramp, sine;
    constant.prepare(sr); ramp.prepare(sr); sine.prepare(sr);
    for (int i = 0; i < 48000; ++i)
    {
        constant.writeSample(.5f, .5f, false);
        const float value = static_cast<float>(i) / 48000.0f;
        ramp.writeSample(value, value, false);
        const float tone = std::sin(static_cast<float>(i) * (2.0f * 3.14159265358979323846f * 440.0f / 48000.0f));
        sine.writeSample(tone, tone, false);
    }

    for (auto window : { GrainveilDsp::GrainVoice::Window::hann, GrainveilDsp::GrainVoice::Window::edge })
    {
        GrainveilDsp::GrainVoice v;
        v.trigger(1000, 2048, 1, false, false, 0, 1, 1, window, 48);
        float first = 0, quarter = 0, middle = 0, last = 0;
        for (int i = 0; i < 2048; ++i)
        {
            float l = 0, r = 0; v.processSample(constant, l, r);
            if (i == 0) first = l;
            if (i == 512) quarter = l;
            if (i == 1024) middle = l;
            if (i == 2047) last = l;
        }
        check(std::abs(first) < 1e-6f && std::abs(last) < 1e-5f, "grain edges fade to zero");
        if (window == GrainveilDsp::GrainVoice::Window::hann)
            check(std::abs(quarter / middle - .5f) < .01f, "Hann window has full-grain taper");
        else
            check(std::abs(quarter / middle - 1.0f) < .01f, "Tukey window has flat middle");
    }

    {
        GrainveilDsp::GrainVoice v;
        v.trigger(10000, 2048, 1, true, false, 0, 1, 1, GrainveilDsp::GrainVoice::Window::edge, 144, true);
        float first = 0, early = 0, late = 0, last = 0;
        for (int i = 0; i < 2048; ++i)
        {
            float l = 0, r = 0; v.processSample(constant, l, r);
            if (i == 0) first = l;
            if (i == 512) early = l;
            if (i == 1536) late = l;
            if (i == 2047) last = l;
        }
        check(std::abs(first) < 1e-6f && std::abs(last) < 1e-5f && late > early * 1.5f,
              "reverse swell rises while retaining short zero-valued edges");
    }
    {
        GrainveilDsp::GrainVoice v;
        v.trigger(30000, 2048, 1, true, false, 0, 1, 1, GrainveilDsp::GrainVoice::Window::edge, 144);
        float a = 0, b = 0;
        for (int i = 0; i < 600; ++i)
        {
            float l = 0, r = 0; v.processSample(ramp, l, r);
            if (i == 300) a = l;
            if (i == 500) b = l;
        }
        check(b < a && std::abs((a - b) - 200.0f / 48000.0f * .70710678f) < 1e-4f,
              "reverse grain reads a ramp backwards at the expected rate");
    }
    // Frozen playback loops only within the capture window. A ramp has a hard
    // discontinuity at the loop seam, so test the short seam crossfade both ways.
    for (bool reverse : { false, true })
    {
        GrainveilDsp::GrainVoice v;
        const double end = static_cast<double>(ramp.getWritePos());
        v.trigger(end - (reverse ? 80 : 4720), 20000, 1, reverse, false, 0, 1, 1,
                  GrainveilDsp::GrainVoice::Window::edge, 48);
        float previous = 0, maxStep = 0;
        for (int i = 0; i < 12000; ++i)
        {
            float l = 0, r = 0; v.processSample(ramp, l, r, true, 4800);
            if (i > 100) maxStep = std::max(maxStep, std::abs(l - previous));
            previous = l;
        }
        check(maxStep < .01f, "frozen capture loop crossfades its seam");
    }
    {
        GrainveilDsp::GrainVoice v;
        v.trigger(10000, 2048, 1, false, false, 0, 1, 1, GrainveilDsp::GrainVoice::Window::edge, 48);
        float before = 0;
        for (int i = 0; i < 300; ++i) { float l = 0, r = 0; v.processSample(ramp, l, r); before = l; }
        float after = 0, right = 0;
        v.processSample(ramp, after, right, true, 4800);
        check(std::abs(before - after) < 1e-6f, "freeze transition starts without discontinuity");
    }
    int crossings[2] {};
    for (int which = 0; which < 2; ++which)
    {
        GrainveilDsp::GrainVoice v;
        v.trigger(5000, 4000, which == 0 ? .5 : 2.0, false, false, 0, 1, 1,
                  GrainveilDsp::GrainVoice::Window::edge, 48);
        float previous = 0;
        for (int i = 0; i < 3500; ++i)
        {
            float l = 0, r = 0; v.processSample(sine, l, r);
            if (i > 100 && previous <= 0 && l > 0) ++crossings[which];
            previous = l;
        }
    }
    check(crossings[0] > 0 && crossings[1] >= crossings[0] * 3.7 && crossings[1] <= crossings[0] * 4.3,
          "-12 and +12 st produce approximately 0.5x and 2x frequency");

    {
        GrainveilDsp::GrainVoice v;
        v.trigger(1000, 2048, 1, false, false, 0, 1, 1, GrainveilDsp::GrainVoice::Window::edge, 48);
        float before = 0;
        for (int i = 0; i < 300; ++i) { float l = 0, r = 0; v.processSample(constant, l, r); before = l; }
        v.trigger(1000, 2048, 1, false, false, 0, 1, 2, GrainveilDsp::GrainVoice::Window::edge, 48);
        float after = 0, right = 0; v.processSample(constant, after, right);
        check(std::abs(before - after) < 1e-6f, "voice steal preserves sample continuity");
        for (int i = 0; i < 64; ++i) { float l = 0, r = 0; v.processSample(constant, l, r); }
        check(v.stealFade == 0, "steal fade is bounded to 64 samples");
    }
    return failures == 0 ? 0 : 1;
}
