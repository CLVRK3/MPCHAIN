# MPCHAIN Desktop (v1.1.0) — Native Windows (VST3) & macOS (Universal VST3 + AU)

**Manufacturer:** MIXCHAIN  
**Formats:** Windows 64-bit VST3 · macOS Universal (`arm64` + `x86_64`) VST3 & Audio Unit (AU) · Standalone  
**GUI:** Native resizable **Two-Zone Mastering Console** with all 3 switchable MPC themes (**MPC Gold**, **MPC Red**, **Retro 3000**), luminous upper telemetry zone (`SAFE / OPTIMAL / CRUSH` meter, live LUFS/Peak/Clip readout, sculpted spectrum crest), 45 fps Peak + LUFS edge multimeters, monumental beveled central **LOUD** dial with `+4.5 dB` clip-onset arc, recessed `|——#——|` **WIDTH** crossfader, **Always-On Linear-Phase 3-Band EQ** (513-tap FFT OLA, 512-sample latency reported to DAW), **TDR Nova-voiced** 1.5:1 Vibe Compressor, **0–240 Hz Low HPF** protecting both Compression and Width, unified **−1.5 dBFS** high-clarity peak shaving (> `+4.5 dB` Loud), **1x / 2x / 4x / 8x** anti-aliasing oversampling, and 16 built-in mastering presets.

## Building on Windows (VST3)
Requires **CMake 3.22+** and **Visual Studio 2022** (or Build Tools for Visual Studio with "Desktop development with C++").

```powershell
cmake -S plugins/MPCHAIN-Desktop -B plugins/MPCHAIN-Desktop/build-win -G "Visual Studio 17 2022" -A x64
cmake --build plugins/MPCHAIN-Desktop/build-win --config Release --target MPCHAIN_VST3
```
Output: `plugins/MPCHAIN-Desktop/build-win/MPCHAIN_artefacts/Release/VST3/MPCHAIN.vst3`  
Copy to `C:\Program Files\Common Files\VST3\`.

## Building on macOS (Universal VST3 + AU)
Requires **Xcode** and **CMake 3.22+**.

```bash
cmake -S plugins/MPCHAIN-Desktop -B plugins/MPCHAIN-Desktop/build-mac -G Xcode -DCMAKE_OSX_ARCHITECTURES="arm64;x86_64"
cmake --build plugins/MPCHAIN-Desktop/build-mac --config Release --target MPCHAIN_VST3 MPCHAIN_AU
```
Outputs:
- `build-mac/MPCHAIN_artefacts/Release/VST3/MPCHAIN.vst3` → copy to `~/Library/Audio/Plug-Ins/VST3/`
- `build-mac/MPCHAIN_artefacts/Release/AU/MPCHAIN.component` → copy to `~/Library/Audio/Plug-Ins/Components/`

## Building Both Automatically in the Cloud (GitHub Actions)
Copy `.github/workflows/build-desktop.yml` into your repository root's `.github/workflows/` folder and push — GitHub Actions compiles both **Windows VST3** and **Universal macOS VST3 + AU** automatically and attaches downloadable artifacts.
