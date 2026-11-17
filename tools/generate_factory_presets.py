#!/usr/bin/env python3
"""Validate the versioned source bank and generate the C++ bank and preset table.

No generator is needed to compile a release: the generated header is checked in.
Run with --check in CI to ensure the checked-in files match the CSV.
"""
import argparse
import collections
import csv
import decimal
import json
import pathlib
import sys

ROOT = pathlib.Path(__file__).resolve().parents[1]
SOURCE = ROOT / "presets/factory-v2.csv"
HEADER = ROOT / "source/FactoryPresets.h"
TABLE = ROOT / "docs/FACTORY_PRESETS.md"
CATEGORIES = [
    "Ambient Wash", "Frozen Drones", "Reverse Swells", "Glitch & Stutter",
    "Pitch Clouds", "Echo Grains", "Lo-Fi / Degraded", "Guitar Textures",
    "Sound Design / Experimental",
]
# The order is shared with PresetManager and its APVTS serialization.
PARAMETERS = [
    "mix", "inGain", "outGain", "freeze", "captureTime", "feedback", "fbTone",
    "grainSize", "density", "scatter", "pitch", "pitchRand", "reverseAmt",
    "spread", "tone", "mode", "lfoRate", "lfoDepth", "tempoSync",
]
MODE_NAMES = ["Cloud", "EchoGrain", "ReverseSwarm", "IceHold", "Fracture", "PitchBloom", "Smear", "Ash"]
RANGES = {
    "mix": ("0", "1", ".001"), "inGain": ("-24", "12", ".1"),
    "outGain": ("-24", "12", ".1"), "freeze": ("0", "1", "1"),
    "captureTime": ("50", "4000", "1"), "feedback": ("0", ".60", ".001"),
    "fbTone": ("0", "1", ".001"), "grainSize": ("10", "500", ".5"),
    "density": ("1", "40", ".1"), "scatter": ("0", "1", ".001"),
    "pitch": ("-24", "24", ".1"), "pitchRand": ("0", "1", ".001"),
    "reverseAmt": ("0", "1", ".001"), "spread": ("0", "1", ".001"),
    "tone": ("0", "1", ".001"), "mode": ("0", "7", "1"),
    "lfoRate": (".05", "10", ".01"), "lfoDepth": ("0", "1", ".001"),
    "tempoSync": ("0", "1", "1"),
}
OLD_NAMES = ["Ambient Wash", "Frozen Drone", "Reverse Swell", "Glitch Rhythm", "Pitch Cloud",
             "Tight Echo-Grain", "Lo-Fi Loop", "Guitar Swell"]
D = decimal.Decimal


def load():
    with SOURCE.open(encoding="utf-8", newline="") as f:
        reader = csv.DictReader(f)
        expected = {"category", "name", *PARAMETERS}
        if reader.fieldnames is None or len(reader.fieldnames) != len(expected) or set(reader.fieldnames) != expected:
            raise ValueError("CSV columns must contain category, name and every parameter exactly once")
        rows = list(reader)
    if len(rows) != 45:
        raise ValueError(f"expected 45 presets, got {len(rows)}")
    counts = collections.Counter(row["category"] for row in rows)
    if counts != {c: 5 for c in CATEGORIES}:
        raise ValueError(f"expected five in each required category, got {counts}")
    if [r["name"] for r in rows[:8]] != OLD_NAMES:
        raise ValueError("the eight original program indices must remain stable")
    names = set()
    fingerprints = set()
    for row in rows:
        name = row["name"]
        if not name or len(name) > 24 or name != name.strip() or name.casefold() in names:
            raise ValueError(f"invalid or duplicated name {name!r}")
        names.add(name.casefold())
        vals = {}
        for key in PARAMETERS:
            try:
                value = D(row[key])
            except (decimal.InvalidOperation, TypeError):
                raise ValueError(f"{name}: invalid {key} {row[key]!r}") from None
            minimum, maximum, step = map(D, RANGES[key])
            if not value.is_finite() or not minimum <= value <= maximum or (value - minimum) % step != 0:
                raise ValueError(f"{name}: {key}={value} outside range/step {RANGES[key]}")
            vals[key] = value
        # The three reserved APVTS fields do not currently modulate DSP. Make it
        # explicit rather than suggesting that presets obtain variety from them.
        if (vals["lfoRate"], vals["lfoDepth"], vals["tempoSync"]) != (D("0.5"), D("0"), D("0")):
            raise ValueError(f"{name}: reserved parameters must use their defaults")
        if vals["inGain"] > 0 or vals["outGain"] > 0:
            raise ValueError(f"{name}: factory gain staging must not boost")
        fingerprint = tuple(vals[k] for k in PARAMETERS)
        if fingerprint in fingerprints:
            raise ValueError(f"{name}: parameter set duplicates another preset")
        fingerprints.add(fingerprint)
        row["values"] = vals
    return rows


def cfloat(value):
    text = format(value, "f")
    return text + (".0" if "." not in text else "") + "f"


def generate_header(rows):
    out = [
        "#pragma once", "", "#include \"Parameters.h\"", "#include <array>", "",
        "// Generated from presets/factory-v2.csv by tools/generate_factory_presets.py.",
        "// Values are in the APVTS parameters' native units; all 19 are explicit.",
        "namespace GrainveilFactory", "{", "constexpr int dataVersion = 2;",
        "inline constexpr std::array<const char*, 19> parameterIds {{",
    ]
    out.append("    " + ", ".join("GrainveilParams::" + p for p in PARAMETERS[:7]) + ",")
    out.append("    " + ", ".join("GrainveilParams::" + p for p in PARAMETERS[7:14]) + ",")
    out.append("    " + ", ".join("GrainveilParams::" + p for p in PARAMETERS[14:]))
    out += ["}};", "struct Preset", "{", "    const char* category;", "    const char* name;",
            "    std::array<float, 19> values;", "};", "inline constexpr std::array<Preset, 45> presets {{"]
    for row in rows:
        values = ", ".join(cfloat(row["values"][p]) for p in PARAMETERS)
        out.append("    { " + json.dumps(row["category"]) + ", " + json.dumps(row["name"]) + ", {{ " + values + " }} },")
    out += ["}};", "}", ""]
    return "\n".join(out)


def fmt(value):
    return format(value.normalize(), "f")


def percent(value):
    return fmt(value * 100) + "%"


def generate_table(rows):
    out = [
        "# Grainveil factory presets — bank v2", "",
        "Exactly 45 presets in nine categories, five per category. Values are in native parameter units.",
        "The original eight factory program indices remain 0–7; the table is grouped for browsing.",
        "Every preset also sets input gain to 0 dB, and the reserved LFO rate/depth and tempo-sync",
        "parameters to 0.5 Hz / 0 / off. These reserved controls currently do not modulate audio.", "",
        "**Freeze on** holds whatever is already in the buffer. Feed audio first; an empty buffer",
        "can produce little or no wet signal. The other Frozen Drones are capture-first: engage Freeze",
        "manually when you want to hold them.", "",
        "The levels are conservatively staged (no input/output boost and feedback ≤ 60%); this is",
        "not loudness matching. Distinctness, loudness and click-free switching need listening tests",
        "in a DAW with representative source material.", "",
        "| Category | Preset | Mode | Key parameter values |", "| --- | --- | --- | --- |",
    ]
    for category in CATEGORIES:
        for r in rows:
            if r["category"] != category:
                continue
            v = r["values"]
            values = (
                f"Mix {percent(v['mix'])}; size {fmt(v['grainSize'])} ms; density {fmt(v['density'])} Hz; "
                f"scatter {percent(v['scatter'])}; feedback {percent(v['feedback'])}; pitch {fmt(v['pitch'])} st; "
                f"capture {fmt(v['captureTime'])} ms; freeze {'on' if v['freeze'] else 'off'}; "
                f"reverse {percent(v['reverseAmt'])}; pitch rand {percent(v['pitchRand'])}; "
                f"spread {percent(v['spread'])}; tone {percent(v['tone'])}; "
                f"FB tone {percent(v['fbTone'])}; output {fmt(v['outGain'])} dB"
            )
            out.append(f"| {category} | {r['name']} | {MODE_NAMES[int(v['mode'])]} | {values} |")
    out.append("")
    return "\n".join(out)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--check", action="store_true")
    args = parser.parse_args()
    try:
        rows = load()
        outputs = {HEADER: generate_header(rows), TABLE: generate_table(rows)}
        if args.check:
            stale = [str(path.relative_to(ROOT)) for path, content in outputs.items()
                     if not path.exists() or path.read_text(encoding="utf-8") != content]
            if stale:
                raise ValueError("generated files out of date: " + ", ".join(stale))
            print("45 factory presets validated; generated files are current")
        else:
            for path, content in outputs.items():
                path.parent.mkdir(parents=True, exist_ok=True)
                path.write_text(content, encoding="utf-8", newline="\n")
            print("Generated factory bank and preset table")
    except (ValueError, OSError) as exc:
        print(f"factory preset error: {exc}", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
