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
- **No locks / I/O / blocking**: UI telemetry uses a lock-free sequence counter; no mutexes, file I/O, or debug logging exist on the audio thread.
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
