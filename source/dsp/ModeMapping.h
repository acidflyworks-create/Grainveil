#pragma once

#include <algorithm>
#include <array>

namespace GrainveilDsp
{
// Multipliers shape the user's controls, rather than replacing their settings.
// Normalised controls use bias01() to retain both endpoints; feedback is
// multiplied directly and capped by the engine. Size and special bounds are ms.
struct ModeMapping
{
    double size, density;
    float scatter, reverse, pitchRandom, feedback;
    double timingBase, timingScatter, anchor;
};

inline constexpr std::array<ModeMapping, 8> modeMappings {{
    // size  density scatter reverse pitchRandom feedback   jitter       anchor
    { 1.80, 2.00, 1.35f, 0.40f, 1.00f, 0.75f, 0.04, 0.22, 0.08 }, // Cloud
    { 0.80, 0.40, 0.35f, 0.60f, 0.65f, 1.40f, 0.01, 0.05, 0.85 }, // EchoGrain
    { 1.60, 1.35, 1.10f, 1.40f, 1.00f, 1.00f, 0.04, 0.24, 0.10 }, // ReverseSwarm
    { 2.40, 1.10, 0.65f, 0.60f, 0.35f, 0.75f, 0.02, 0.08, 0.08 }, // IceHold
    { 0.055,3.20, 1.80f, 1.10f, 1.20f, 0.85f, 0.20, 0.55, 0.12 }, // Fracture
    { 1.30, 1.70, 1.15f, 0.75f, 1.40f, 0.85f, 0.04, 0.25, 0.08 }, // PitchBloom
    { 3.00, 1.30, 0.60f, 0.60f, 0.75f, 0.70f, 0.008,0.04, 0.06 }, // Smear
    { 0.65, 1.70, 1.50f, 1.00f, 1.50f, 0.85f, 0.10, 0.40, 0.10 }  // Ash
}};

inline const ModeMapping& modeMapping(int mode) noexcept
{
    return modeMappings[static_cast<std::size_t>(std::clamp(mode, 0, 7))];
}

inline double bias01(double value, double factor) noexcept
{
    const double x = std::clamp(value, 0.0, 1.0);
    return x * factor / (1.0 + (factor - 1.0) * x);
}

inline double modeGrainMs(int mode, double userMs) noexcept
{
    const double scaled = userMs * modeMapping(mode).size;
    return mode == 4 ? std::clamp(scaled, 5.0, 20.0) : scaled;
}
}
