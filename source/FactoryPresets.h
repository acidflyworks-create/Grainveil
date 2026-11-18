#pragma once

#include "Parameters.h"
#include <array>

// Generated from presets/factory-v3.csv by tools/generate_factory_presets.py.
// Values are in the APVTS parameters' native units; all 19 are explicit.
namespace GrainveilFactory
{
constexpr int dataVersion = 3;
inline constexpr std::array<const char*, 19> parameterIds {{
    GrainveilParams::mix, GrainveilParams::inGain, GrainveilParams::outGain, GrainveilParams::freeze, GrainveilParams::captureTime, GrainveilParams::feedback, GrainveilParams::fbTone,
    GrainveilParams::grainSize, GrainveilParams::density, GrainveilParams::scatter, GrainveilParams::pitch, GrainveilParams::pitchRand, GrainveilParams::reverseAmt, GrainveilParams::spread,
    GrainveilParams::tone, GrainveilParams::mode, GrainveilParams::lfoRate, GrainveilParams::lfoDepth, GrainveilParams::tempoSync
}};
struct Preset
{
    const char* category;
    const char* name;
    std::array<float, 19> values;
};
inline constexpr std::array<Preset, 45> presets {{
    { "Ambient Wash", "Ambient Wash", {{ 0.55f, 0.0f, -3.0f, 0.0f, 1000.0f, 0.45f, 0.5f, 220.0f, 18.0f, 0.35f, 0.0f, 0.0f, 0.15f, 0.8f, 0.7f, 0.0f, 0.5f, 0.0f, 0.0f }} },
    { "Frozen Drones", "Frozen Drone", {{ 0.75f, 0.0f, -3.0f, 0.0f, 1000.0f, 0.3f, 0.5f, 360.0f, 12.0f, 0.25f, 0.0f, 0.0f, 0.2f, 0.7f, 0.55f, 3.0f, 0.5f, 0.0f, 0.0f }} },
    { "Reverse Swells", "Reverse Swell", {{ 0.65f, 0.0f, -3.0f, 0.0f, 1000.0f, 0.4f, 0.5f, 260.0f, 14.0f, 0.3f, 0.0f, 0.0f, 0.9f, 0.65f, 0.65f, 2.0f, 0.5f, 0.0f, 0.0f }} },
    { "Glitch & Stutter", "Glitch Rhythm", {{ 0.82f, 0.0f, -6.0f, 0.0f, 1000.0f, 0.3f, 0.5f, 140.0f, 32.0f, 0.86f, 0.0f, 0.0f, 0.55f, 0.95f, 0.8f, 4.0f, 0.5f, 0.0f, 0.0f }} },
    { "Pitch Clouds", "Pitch Cloud", {{ 0.8f, 0.0f, -6.0f, 0.0f, 1000.0f, 0.3f, 0.5f, 200.0f, 24.0f, 0.62f, 12.0f, 0.6f, 0.0f, 0.9f, 0.65f, 5.0f, 0.5f, 0.0f, 0.0f }} },
    { "Echo Grains", "Tight Echo-Grain", {{ 0.45f, 0.0f, -3.0f, 0.0f, 420.0f, 0.45f, 0.5f, 110.0f, 8.0f, 0.1f, 0.0f, 0.0f, 0.0f, 0.4f, 0.75f, 1.0f, 0.5f, 0.0f, 0.0f }} },
    { "Lo-Fi / Degraded", "Lo-Fi Loop", {{ 0.65f, 0.0f, -3.0f, 0.0f, 1200.0f, 0.5f, 0.5f, 160.0f, 11.0f, 0.45f, 0.0f, 0.0f, 0.0f, 0.5f, 0.4f, 7.0f, 0.5f, 0.0f, 0.0f }} },
    { "Guitar Textures", "Guitar Swell", {{ 0.6f, 0.0f, -3.0f, 0.0f, 1000.0f, 0.4f, 0.5f, 320.0f, 16.0f, 0.2f, 0.0f, 0.0f, 0.25f, 0.75f, 0.72f, 6.0f, 0.5f, 0.0f, 0.0f }} },
    { "Ambient Wash", "Veil of Dawn", {{ 0.48f, 0.0f, -3.0f, 0.0f, 1800.0f, 0.36f, 0.58f, 310.0f, 11.0f, 0.18f, 0.0f, 0.02f, 0.08f, 0.72f, 0.72f, 0.0f, 0.5f, 0.0f, 0.0f }} },
    { "Ambient Wash", "Slow Horizon", {{ 0.58f, 0.0f, -3.0f, 0.0f, 2600.0f, 0.42f, 0.38f, 420.0f, 8.0f, 0.12f, -5.0f, 0.04f, 0.12f, 0.84f, 0.48f, 6.0f, 0.5f, 0.0f, 0.0f }} },
    { "Ambient Wash", "Silver Canopy", {{ 0.8f, 0.0f, -6.0f, 0.0f, 1400.0f, 0.32f, 0.68f, 145.0f, 30.0f, 0.78f, 12.0f, 0.35f, 0.22f, 0.95f, 0.82f, 0.0f, 0.5f, 0.0f, 0.0f }} },
    { "Ambient Wash", "Night Air", {{ 0.8f, 0.0f, -6.0f, 0.0f, 2200.0f, 0.5f, 0.32f, 380.0f, 20.0f, 0.72f, -12.0f, 0.08f, 0.3f, 0.88f, 0.38f, 6.0f, 0.5f, 0.0f, 0.0f }} },
    { "Frozen Drones", "Amber Ice", {{ 0.68f, 0.0f, -3.0f, 0.0f, 2600.0f, 0.22f, 0.46f, 460.0f, 7.0f, 0.1f, 7.0f, 0.04f, 0.1f, 0.82f, 0.58f, 3.0f, 0.5f, 0.0f, 0.0f }} },
    { "Frozen Drones", "Boreal Hold", {{ 0.64f, 0.0f, -3.0f, 0.0f, 2000.0f, 0.38f, 0.52f, 290.0f, 15.0f, 0.42f, 0.0f, 0.18f, 0.35f, 0.95f, 0.64f, 3.0f, 0.5f, 0.0f, 0.0f }} },
    { "Frozen Drones", "Low Glacier", {{ 0.72f, 0.0f, -3.0f, 0.0f, 3600.0f, 0.28f, 0.28f, 480.0f, 5.0f, 0.08f, -12.0f, 0.03f, 0.45f, 0.62f, 0.32f, 3.0f, 0.5f, 0.0f, 0.0f }} },
    { "Frozen Drones", "Crystal Still", {{ 0.82f, 0.0f, -6.0f, 0.0f, 1400.0f, 0.25f, 0.7f, 235.0f, 27.0f, 0.55f, 12.0f, 0.38f, 0.18f, 0.93f, 0.82f, 3.0f, 0.5f, 0.0f, 0.0f }} },
    { "Reverse Swells", "Backward Bloom", {{ 0.58f, 0.0f, -3.0f, 0.0f, 900.0f, 0.3f, 0.6f, 190.0f, 19.0f, 0.25f, 0.0f, 0.04f, 0.82f, 0.8f, 0.76f, 2.0f, 0.5f, 0.0f, 0.0f }} },
    { "Reverse Swells", "Sundown Return", {{ 0.8f, 0.0f, -6.0f, 0.0f, 2400.0f, 0.42f, 0.35f, 380.0f, 8.0f, 0.55f, -12.0f, 0.08f, 1.0f, 0.72f, 0.45f, 2.0f, 0.5f, 0.0f, 0.0f }} },
    { "Reverse Swells", "Distant Inhale", {{ 0.8f, 0.0f, -6.0f, 0.0f, 650.0f, 0.22f, 0.72f, 125.0f, 10.0f, 0.6f, 12.0f, 0.1f, 1.0f, 0.55f, 0.8f, 2.0f, 0.5f, 0.0f, 0.0f }} },
    { "Reverse Swells", "Glass Tide", {{ 0.86f, 0.0f, -6.0f, 0.0f, 1900.0f, 0.35f, 0.58f, 340.0f, 30.0f, 0.86f, 7.0f, 0.18f, 1.0f, 0.95f, 0.66f, 2.0f, 0.5f, 0.0f, 0.0f }} },
    { "Glitch & Stutter", "Broken Clock", {{ 0.46f, 0.0f, -3.0f, 0.0f, 350.0f, 0.22f, 0.55f, 28.0f, 16.0f, 0.62f, 0.0f, 0.0f, 0.1f, 0.4f, 0.7f, 4.0f, 0.5f, 0.0f, 0.0f }} },
    { "Glitch & Stutter", "Needle Burst", {{ 0.9f, 0.0f, -6.0f, 0.0f, 180.0f, 0.18f, 0.72f, 100.0f, 38.0f, 0.95f, 12.0f, 0.45f, 0.28f, 0.78f, 0.88f, 4.0f, 0.5f, 0.0f, 0.0f }} },
    { "Glitch & Stutter", "Scatterstep", {{ 0.85f, 0.0f, -6.0f, 0.0f, 850.0f, 0.4f, 0.42f, 220.0f, 32.0f, 0.95f, -12.0f, 0.12f, 0.7f, 0.92f, 0.55f, 4.0f, 0.5f, 0.0f, 0.0f }} },
    { "Glitch & Stutter", "Static Shards", {{ 0.86f, 0.0f, -6.0f, 0.0f, 260.0f, 0.28f, 0.3f, 35.0f, 36.0f, 0.95f, -12.0f, 0.55f, 0.3f, 0.82f, 0.36f, 7.0f, 0.5f, 0.0f, 0.0f }} },
    { "Pitch Clouds", "Velvet Fifth", {{ 0.48f, 0.0f, -3.0f, 0.0f, 1600.0f, 0.32f, 0.46f, 250.0f, 12.0f, 0.18f, 7.0f, 0.05f, 0.08f, 0.72f, 0.56f, 5.0f, 0.5f, 0.0f, 0.0f }} },
    { "Pitch Clouds", "Shimmer Arc", {{ 0.82f, 0.0f, -6.0f, 0.0f, 2100.0f, 0.4f, 0.62f, 330.0f, 28.0f, 0.7f, 12.0f, 0.48f, 0.18f, 0.93f, 0.82f, 5.0f, 0.5f, 0.0f, 0.0f }} },
    { "Pitch Clouds", "Octave Drift", {{ 0.82f, 0.0f, -6.0f, 0.0f, 2800.0f, 0.36f, 0.32f, 410.0f, 22.0f, 0.72f, -12.0f, 0.45f, 0.25f, 0.86f, 0.4f, 5.0f, 0.5f, 0.0f, 0.0f }} },
    { "Pitch Clouds", "Prismatic Third", {{ 0.84f, 0.0f, -6.0f, 0.0f, 900.0f, 0.28f, 0.56f, 115.0f, 34.0f, 0.82f, 7.0f, 0.6f, 0.12f, 0.98f, 0.74f, 5.0f, 0.5f, 0.0f, 0.0f }} },
    { "Echo Grains", "Copper Taps", {{ 0.38f, 0.0f, -3.0f, 0.0f, 230.0f, 0.34f, 0.64f, 70.0f, 6.0f, 0.05f, 0.0f, 0.0f, 0.0f, 0.3f, 0.72f, 1.0f, 0.5f, 0.0f, 0.0f }} },
    { "Echo Grains", "Dust Delay", {{ 0.42f, 0.0f, -3.0f, 0.0f, 780.0f, 0.5f, 0.3f, 130.0f, 4.0f, 0.18f, -5.0f, 0.04f, 0.08f, 0.55f, 0.43f, 1.0f, 0.5f, 0.0f, 0.0f }} },
    { "Echo Grains", "Wandering Echo", {{ 0.8f, 0.0f, -6.0f, 0.0f, 1450.0f, 0.46f, 0.52f, 190.0f, 16.0f, 0.75f, 12.0f, 0.35f, 0.25f, 0.95f, 0.66f, 1.0f, 0.5f, 0.0f, 0.0f }} },
    { "Echo Grains", "Dusk Repeater", {{ 0.8f, 0.0f, -6.0f, 0.0f, 2200.0f, 0.5f, 0.25f, 240.0f, 15.0f, 0.08f, -12.0f, 0.02f, 0.4f, 0.65f, 0.36f, 1.0f, 0.5f, 0.0f, 0.0f }} },
    { "Lo-Fi / Degraded", "Oxide Trail", {{ 0.5f, 0.0f, -3.0f, 0.0f, 1700.0f, 0.38f, 0.25f, 220.0f, 8.0f, 0.32f, 0.0f, 0.06f, 0.1f, 0.48f, 0.35f, 7.0f, 0.5f, 0.0f, 0.0f }} },
    { "Lo-Fi / Degraded", "Tape Motes", {{ 0.8f, 0.0f, -6.0f, 0.0f, 720.0f, 0.32f, 0.36f, 50.0f, 32.0f, 0.9f, -12.0f, 0.4f, 0.22f, 0.72f, 0.48f, 7.0f, 0.5f, 0.0f, 0.0f }} },
    { "Lo-Fi / Degraded", "Dim Radio", {{ 0.46f, 0.0f, -3.0f, 0.0f, 350.0f, 0.18f, 0.2f, 40.0f, 24.0f, 0.7f, 7.0f, 0.08f, 0.08f, 0.35f, 0.25f, 7.0f, 0.5f, 0.0f, 0.0f }} },
    { "Lo-Fi / Degraded", "Broken Velvet", {{ 0.82f, 0.0f, -6.0f, 0.0f, 2400.0f, 0.48f, 0.3f, 310.0f, 7.0f, 0.85f, -12.0f, 0.55f, 0.75f, 0.85f, 0.3f, 7.0f, 0.5f, 0.0f, 0.0f }} },
    { "Guitar Textures", "Silk Harmonics", {{ 0.38f, 0.0f, -3.0f, 0.0f, 800.0f, 0.28f, 0.62f, 140.0f, 17.0f, 0.18f, 12.0f, 0.04f, 0.06f, 0.7f, 0.77f, 0.0f, 0.5f, 0.0f, 0.0f }} },
    { "Guitar Textures", "Bowed Strings", {{ 0.52f, 0.0f, -3.0f, 0.0f, 2100.0f, 0.38f, 0.4f, 390.0f, 10.0f, 0.15f, 0.0f, 0.05f, 0.2f, 0.8f, 0.5f, 6.0f, 0.5f, 0.0f, 0.0f }} },
    { "Guitar Textures", "Chime Bloom", {{ 0.8f, 0.0f, -6.0f, 0.0f, 1200.0f, 0.32f, 0.62f, 175.0f, 26.0f, 0.75f, 12.0f, 0.4f, 0.12f, 0.88f, 0.82f, 5.0f, 0.5f, 0.0f, 0.0f }} },
    { "Guitar Textures", "Deep Resonance", {{ 0.4f, 0.0f, -3.0f, 0.0f, 1050.0f, 0.5f, 0.3f, 210.0f, 6.0f, 0.12f, -12.0f, 0.03f, 0.05f, 0.58f, 0.38f, 1.0f, 0.5f, 0.0f, 0.0f }} },
    { "Sound Design / Experimental", "Fault Lines", {{ 0.9f, 0.0f, -6.0f, 0.0f, 540.0f, 0.46f, 0.4f, 200.0f, 38.0f, 1.0f, -12.0f, 0.65f, 0.8f, 0.96f, 0.6f, 4.0f, 0.5f, 0.0f, 0.0f }} },
    { "Sound Design / Experimental", "Metal Vortex", {{ 0.9f, 0.0f, -6.0f, 0.0f, 450.0f, 0.35f, 0.74f, 70.0f, 38.0f, 0.9f, 19.0f, 0.7f, 0.75f, 0.95f, 0.84f, 5.0f, 0.5f, 0.0f, 0.0f }} },
    { "Sound Design / Experimental", "Ghost Circuit", {{ 0.88f, 0.0f, -6.0f, 0.0f, 1200.0f, 0.5f, 0.2f, 120.0f, 30.0f, 0.9f, -19.0f, 0.6f, 0.8f, 0.7f, 0.24f, 7.0f, 0.5f, 0.0f, 0.0f }} },
    { "Sound Design / Experimental", "Orbit Debris", {{ 0.85f, 0.0f, -6.0f, 0.0f, 1100.0f, 0.38f, 0.48f, 80.0f, 35.0f, 0.95f, -12.0f, 0.7f, 0.75f, 1.0f, 0.68f, 0.0f, 0.5f, 0.0f, 0.0f }} },
    { "Sound Design / Experimental", "Feral Memory", {{ 0.9f, 0.0f, -6.0f, 0.0f, 1200.0f, 0.5f, 0.34f, 480.0f, 28.0f, 0.9f, 12.0f, 0.55f, 1.0f, 1.0f, 0.52f, 2.0f, 0.5f, 0.0f, 0.0f }} },
}};
}
