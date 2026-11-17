# Stage E — asset, size and UI validation report

## Asset inventory

All sizes are decimal bytes; SHA-256 hashes refer to the source files. The two
unmodified Lato fonts are embedded in the plugin. They are licensed under the
SIL Open Font License 1.1, copyright Łukasz Dziedzic / tyPoland (2010–2014).
The complete notice is `assets/fonts/OFL.txt`; details and source are in
[`THIRD_PARTY_LICENSES.md`](../THIRD_PARTY_LICENSES.md).

| Asset | Bytes | Embedded | SHA-256 |
| --- | ---: | --- | --- |
| `assets/fonts/Lato-Regular.ttf` | 656,568 | yes | `d636e4683231f931eda222d588e944d082bfd3bdba02f928bee461c0f185b251` |
| `assets/fonts/Lato-SemiBold.ttf` | 668,548 | yes | `71b8b7decbe75a881ed267be539d402bd1e9420b799658aada4e0d1bd5af803c` |
| `assets/fonts/OFL.txt` | 4,407 | no; included in release ZIP | `74ba064d03f1f1c4a952da936c3eb71866c34404916734de3cae73b34357e59e` |

Total files under `assets/`: **1,329,523 bytes**. Embedded fonts: **1,325,116
bytes**. No bitmaps are used. The 60 MB hard asset budget counts all regular
files under `assets/`, including the license; CMake enforces it on configure.
`python tools/size_report.py` also reports hashes and enforces this budget.
`python tools/check_font_glyphs.py` checks both Unicode cmaps for printable ASCII,
± and ellipsis (including the required digits, %, dB, Hz and st) without relying
on OS font fallback. Arbitrary user-entered Unicode is not covered by that test.

Release ZIPs include `LICENSE`, `THIRD_PARTY_LICENSES.md` and `OFL.txt` alongside
the plugin. Keep these notices with redistributed copies. JUCE is separately
licensed; consult its upstream distribution for the applicable terms.

## Plugin sizes

The workflow uses `tools/size_report.py --bundle <path>` on each actual uncompressed
VST3 or AU bundle after linking. It sums regular file lengths, not ZIP size or disk
allocation; symbolic links are not counted twice. It warns at or above the **40 MB**
target and fails above the **200 MB** hard limit. Machine-readable asset and
per-format reports are uploaded as `Grainveil-size-reports-Windows` and
`Grainveil-size-reports-macOS`. Size checks are performed before ZIP packaging.

## Automated UI checks

Run on native Windows/macOS runners with `-DGRAINVEIL_BUILD_UI_TESTS=ON` and
`ctest --test-dir build -C Release --output-on-failure`:

- Design regions and visible sibling component bounds, viewport containment and
  transformed overlap at editor sizes 750×465, 1000×620, 2000×1240 and simulated
  100%, 150%, 200% pixel scales; preset-bar peers and slider value boxes are checked.
- Measured text fitting, extreme numeric strings, long names, ellipsis and clipping;
  font cmaps and slider value-field theme are checked independently.
- Off-screen normal and stress renders for the 3×3 size/scale matrix. Stress state
  uses a 32-character name, modified marker, ReverseSwarm and maximum knob values;
  the monitor fixture is explicitly synthetic. The full name tooltip is tested.
- Factory bank validation, all 45 complete parameter loads, category counts,
  original bank compatibility, host requests, user preset/state recall and telemetry.

CI uploads the PNGs as `Grainveil-UI-previews-Windows` and
`Grainveil-UI-previews-macOS`. These are off-screen renders with a simulated graphics
transform, **not screenshots of a DAW or physical-monitor DPI tests**. Bounds and
text tests do not exhaustively prove that every popup, editable field, host chrome,
translation or glyph will render without overlap.

## Manual acceptance checklist — not yet performed

Run in a DAW on Windows VST3, macOS VST3 and macOS AU where available; note DAW,
OS version, monitor and plugin version, and capture screenshots for each result.

- [ ] Test actual OS display scaling at 100%, 150%, 200%, including moving the
      plugin window between monitors and reopening the editor. Test min/default/max
      editor sizes at each setting, plus host-forced aspect/resize behavior.
- [ ] Inspect labels, values, preset names, editable text, popup categories, tooltips,
      freeze and waveform for clipping, overlap, focus, and readable contrast.
- [ ] Use 32-character preset names, modification markers, min/max values and all
      modes; test truncation and full-name tooltip. Verify mouse, wheel and keyboard.
- [ ] Cycle 45 factory presets and user slots; save, save-as, reload, modify/undo,
      and reopen sessions. Test old sessions and reassign old host program automation.
- [ ] Feed audio before using frozen presets; check monitor movement and behavior
      through transport stop/start and audio-device/sample-rate changes.
- [ ] Audition presets with representative sources; check levels, transients and
      switching noise. They have not been loudness-matched or certified click-free.
- [ ] Check host scan/installation and reopen; measure CPU and latency/xruns with
      representative projects. macOS artifacts are not notarized.

The automated tests do not substitute for these host, listening, accessibility and
real-monitor checks. No results should be marked passed without performing them.
