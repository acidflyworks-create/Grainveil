# Grainveil

Original stereo granular processor plugin (VST3 / AU) built with JUCE and CMake.

## Prebuilt binaries (Windows / macOS)

`.github/workflows/build.yml` builds the plugin on native GitHub Actions runners:

| Platform | Runner | Toolchain | Output |
| --- | --- | --- | --- |
| Windows x64 | `windows-2022` | Visual Studio 2022 (MSVC), Release | `Grainveil-VST3-Windows-x64.zip` |
| macOS universal (Intel + Apple Silicon) | `macos-15` | Apple Clang / Xcode, Release | `Grainveil-VST3-macOS-universal.zip` (+ `Grainveil-AU-macOS-universal.zip`) |

Start a build with **Actions → Build plugins (Windows / macOS) → Run workflow**, or push a
`v*` tag. Manual runs also publish a GitHub **Release** containing all three ZIP archives;
every run additionally keeps the archives as downloadable workflow artefacts for 90 days.
The macOS build is a single universal binary (`x86_64;arm64`, deployment target 11.0) and
is ad-hoc code signed.

Installation: unzip and copy the whole `Grainveil.vst3` bundle (on Windows JUCE 8
produces a folder `Contents/x86_64-win/Grainveil.vst3`, not a single DLL) into
`C:\Program Files\Common Files\VST3\` (Windows) or
`~/Library/Audio/Plug-Ins/VST3/` (macOS). The binaries are not notarized: on macOS run
`xattr -dr com.apple.quarantine ~/Library/Audio/Plug-Ins/VST3/Grainveil.vst3` before
the first scan, and allow the plugin in System Settings if Gatekeeper asks.

### Готовые сборки

Сборка под Windows и macOS выполняется в GitHub Actions (нативные раннеры, JUCE 8.0.4).
После запуска workflow архивы с `.vst3` появляются на вкладке **Releases** и в артефактах
запуска. Локально из Linux собрать эти два формата нельзя — нужен MSVC и Xcode/macOS SDK.

## Editor and presets (stage C)

The current editor uses a 1000 × 620 logical design grid. User resizing is constrained
from 750 × 465 (75%) to 2000 × 1240 (200%) at the same aspect ratio. All existing
controls live in one transformed design surface; if a host forces a different aspect
ratio the surface is centered with letterboxing. A procedural graphite panel, custom
rotary rings and freeze switch, selector/menu styling and embedded-font tooltips make
up the visuals. The preset bar switches programs, shows modified state and opens a
categorized menu. When a knob is dragged its name and formatted
value fade into the display's dedicated value strip; the editable numeric field remains
available below the knob. Mode and freeze indicators follow the existing UI attachments.

The screen shows the current preset, mode and freeze status. Its live waveform and
grain markers use a bounded, lock-free atomic telemetry exchange published roughly
30 times per second. Only telemetry publication changed; the granular signal path
and parameter IDs are unchanged. The monitor remains unavailable until audio has
been processed at least once.

### Preset bar and host programs

The arrows wrap across available programs; click the name for a categorized menu, use
the wheel over the name, or Left/Right when the editor has focus. `Save` updates a user
preset; on a factory or Init program it opens `Save As`. `Save As` refuses to overwrite
an existing name. A star indicates a difference from the stored parameter values and
clears if the values are returned to the baseline. User names are limited to 32
characters and validated as cross-platform filenames.

User presets are versioned `.gvpreset` XML files containing all 19 normalized
parameters, stored in `Documents/Grainveil/Presets` on Windows and
`~/Library/Audio/Presets/Grainveil` on macOS. UI file operations and host notifications
run on the message thread; a host program request is queued atomically and applied on
the next message-thread timer tick. The session state stores the selected preset identity
and baseline as well as the current values; old sessions without this metadata load as
Init, marked modified if their parameters differ from the defaults. If a user preset file is missing at project reopen, the audio parameters
are restored but the selection falls back to Init.

For stable VST3 program-change automation, each instance exposes **64 fixed user slots**
(in addition to Init and the current eight factory programs); unused slots may be visible
in the host's program list but are omitted from the plugin's menu and arrow navigation.
Slot numbers are persisted in the files. Multiple instances coordinate writes; files
created by another instance become visible when saving or on reopening the plugin. Avoid
manually duplicating a `.gvpreset` file, as two files with the same slot conflict. A host
may cache program names and require reopening the instance to refresh its list.

The stage C factory table retains the existing eight programs; the requested 45 original
presets are planned for stage D. Existing DSP smoothing covers many continuous parameters,
but not every discrete transition; click-free switching across all material has not been
verified and cannot be guaranteed without further audio-path work and listening tests.

The UI embeds Lato Regular and Semibold under the SIL OFL 1.1 (see
`THIRD_PARTY_LICENSES.md`). `UiTypography` measures single-line text, reduces it to a
specified minimum size, then adds an ellipsis and clips it to its rectangle. Knob
values and labels have separate rows. macOS and Windows host DPI scaling remains the
host/JUCE's responsibility; editor scale is independent of the OS display scale.

UI assets (bytes): `Lato-Regular.ttf` 656,568; `Lato-SemiBold.ttf` 668,548;
`OFL.txt` 4,407. Total under `assets/`: **1,329,523 bytes** (1,325,116 bytes
embedded). CMake lists and checks every file in `assets/` against a 60 MB (60,000,000-byte) budget.

For the automated design-grid and text-fitting harness, configure with
`-DGRAINVEIL_BUILD_UI_TESTS=ON`, build, and run `ctest --test-dir build -C Release
--output-on-failure`. It checks non-overlapping grid rectangles at min/default/max
editor scales and 100/150/200% simulated DPI, fitting/ellipsis and text ink clipping.
The tests also cover host program requests, wrap-around, user save/load, modified markers,
session recall and concurrent telemetry. They render the actual editor off-screen at
min/default/max sizes and simulated 100/150/200% DPI into `build/ui-previews/`, plus a
clearly named synthetic monitor fixture. CI uploads these images as `Grainveil-UI-previews-Windows` and
`Grainveil-UI-previews-macOS`. They are test renders, not screenshots from a DAW or
proof of behavior at real OS display scale settings.

## Build Instructions

### Windows (Visual Studio 2022, x64)

```powershell
cmake -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release
```

Compiled VST3 bundle location:
`build/Grainveil_artefacts/Release/VST3/Grainveil.vst3`

Copy `Grainveil.vst3` to:
`C:\Program Files\Common Files\VST3\`

---

### macOS (Xcode, Universal Binary: Intel + Apple Silicon)

```bash
cmake -B build -G Xcode
cmake --build build --config Release
```

Compiled plugin locations:
- **VST3**: `build/Grainveil_artefacts/Release/VST3/Grainveil.vst3`
- **AU**: `build/Grainveil_artefacts/Release/AU/Grainveil.component`

Copy to system plugin folders:
```bash
cp -R build/Grainveil_artefacts/Release/VST3/Grainveil.vst3 ~/Library/Audio/Plug-Ins/VST3/
cp -R build/Grainveil_artefacts/Release/AU/Grainveil.component ~/Library/Audio/Plug-Ins/Components/
```

---

## Real-Time Safety & Hardening Checklist
- **Zero allocations in `processBlock`**: All 32 `GrainVoice` instances, the 16-second stereo `CircularBuffer`, SVF filters, and `TelemetrySnapshot` are preallocated in `prepareToPlay`.
- **No locks / I/O / blocking**: no mutexes, file I/O, or debug logging exist on the audio thread. The telemetry handoff uses lock-free atomics with bounded reads; preset file I/O is confined to the message thread.
- **Denormals & Stability**: `juce::ScopedNoDenormals` in `processBlock`, NaN/Inf sanitization, `feedback` clamped to `0.95`, and `std::tanh` soft-clipping on feedback and wet sum.
- **Parameter Smoothing**: `mix`, `inGain`, `outGain`, `captureTime`, `feedback`, `fbTone`, `grainSize`, `density`, `pitch`, and `tone` are smoothed via `juce::SmoothedValue`.

## Known Limitations
- **Stereo-only bus**: Requires stereo input and stereo output (`AudioChannelSet::stereo()`).
- **Reserved APVTS parameters**: `tempoSync`, `lfoRate`, and `lfoDepth` are registered in APVTS for state compatibility, but are not yet modulated in the DSP engine.
- **32-voice cap**: Fixed 32-grain pool with oldest-voice stealing to guarantee bounded CPU usage at 48 kHz / 128 samples.

---

## Reaper Test Protocol
1. **CPU & Buffer Size**: Set Reaper to `48 kHz / 128 samples`. Insert `Grainveil` on a stereo guitar/synth track, select **Cloud** or **Fracture**, and verify < 5% single-core CPU with zero xruns.
2. **Sample Rate & Block Size Switching**: Switch Reaper between `44.1 kHz`, `48 kHz`, `96 kHz`, and `192 kHz` (and buffers `32` to `2048`); verify clean audio and accurate pitch/grain timing.
3. **Freeze Spam & Silence**: Rapidly toggle `FREEZE` on/off while feeding audio and during silence; verify no clicks, no buffer corruption, and clean tail decay.
4. **High Feedback & Huge Hits**: Set `Feedback` to `0.95` and send a loud transient; verify the cloud blooms and stabilizes smoothly without clipping or NaN/Inf lockup.
5. **Preset & State Recall**: Cycle all 8 factory presets, save the `.rpp` project, close, and reopen; verify exact parameter and UI state restoration.
