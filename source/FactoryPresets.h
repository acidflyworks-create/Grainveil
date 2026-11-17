#pragma once

#include "Parameters.h"
#include <array>

// Versioned built-in program data. Stage C intentionally retains the original
// eight factory programs. Stage D will replace this table with the agreed 45.
namespace GrainveilFactory
{
constexpr int dataVersion = 1;
struct Override { const char* id; float value; };
struct Preset
{
    const char* category;
    const char* name;
    const Override* values;
    size_t count;
};
using namespace GrainveilParams;
inline constexpr Override ambient[] = { {mode,0}, {mix,.55f}, {grainSize,220}, {density,18}, {scatter,.35f}, {feedback,.45f}, {pitch,0}, {reverseAmt,.15f}, {spread,.8f}, {tone,.7f}, {freeze,0} };
inline constexpr Override frozen[] = { {mode,3}, {mix,.75f}, {grainSize,360}, {density,12}, {scatter,.25f}, {feedback,.3f}, {pitch,0}, {reverseAmt,.2f}, {spread,.7f}, {tone,.55f}, {freeze,1} };
inline constexpr Override reverse[] = { {mode,2}, {mix,.65f}, {grainSize,260}, {density,14}, {scatter,.3f}, {feedback,.4f}, {pitch,0}, {reverseAmt,.9f}, {spread,.65f}, {tone,.65f}, {freeze,0} };
inline constexpr Override glitch[] = { {mode,4}, {mix,.6f}, {grainSize,45}, {density,26}, {scatter,.75f}, {feedback,.35f}, {pitch,0}, {reverseAmt,.35f}, {spread,.85f}, {tone,.8f}, {freeze,0} };
inline constexpr Override cloud[] = { {mode,5}, {mix,.55f}, {grainSize,190}, {density,16}, {scatter,.4f}, {feedback,.5f}, {pitch,12}, {pitchRand,.6f}, {spread,.75f}, {tone,.65f}, {freeze,0} };
inline constexpr Override echo[] = { {mode,1}, {mix,.45f}, {captureTime,420}, {grainSize,110}, {density,8}, {scatter,.1f}, {feedback,.45f}, {pitch,0}, {spread,.4f}, {tone,.75f}, {freeze,0} };
inline constexpr Override lofi[] = { {mode,7}, {mix,.65f}, {captureTime,1200}, {grainSize,160}, {density,11}, {scatter,.45f}, {feedback,.55f}, {tone,.4f}, {spread,.5f}, {freeze,0} };
inline constexpr Override guitar[] = { {mode,6}, {mix,.6f}, {grainSize,320}, {density,16}, {scatter,.2f}, {feedback,.4f}, {pitch,0}, {reverseAmt,.25f}, {spread,.75f}, {tone,.72f}, {freeze,0} };
inline constexpr std::array<Preset, 8> presets {{
    { "Ambient Wash", "Ambient Wash", ambient, std::size(ambient) },
    { "Frozen Drones", "Frozen Drone", frozen, std::size(frozen) },
    { "Reverse Swells", "Reverse Swell", reverse, std::size(reverse) },
    { "Glitch & Stutter", "Glitch Rhythm", glitch, std::size(glitch) },
    { "Pitch Clouds", "Pitch Cloud", cloud, std::size(cloud) },
    { "Echo Grains", "Tight Echo-Grain", echo, std::size(echo) },
    { "Lo-Fi / Degraded", "Lo-Fi Loop", lofi, std::size(lofi) },
    { "Guitar Textures", "Guitar Swell", guitar, std::size(guitar) }
}};
}
