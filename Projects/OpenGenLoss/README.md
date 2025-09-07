# OpenGenLoss (iPlug2)

A macOS-ready AU/VST3 audio effect inspired by VHS/tape "generation loss" characteristics (wow/flutter, dropouts, noise, saturation, tone shaping), implemented with the official **iPlug2** framework.

> **Note:** “Generation Loss” is a mark associated with the Chase Bliss pedals. This project is an original open-source recreation of *similar sonic behaviors* for educational purposes and is not affiliated with Chase Bliss.

---

## Features
- **Wow & Flutter** (FM-style pitch modulation via fractional delay)
- **Dropouts** (random amplitude dips with eased envelopes)
- **Saturation** (soft clip with pre-emphasis/de-emphasis)
- **Noise/Hiss** and **Hum** generator (50/60Hz selectable)
- **Model EQ** (simple lowpass + highshelf “tape/VHS-ish” curves)
- **Tilt EQ**
- **Stereo spread** decorrelation
- **Mix** and **Output** gain
- **Aux: Tape Stop** (momentary control)

## Build (macOS) – using iPlug2OOS (recommended)
1. **Clone iPlug2OOS template** (once):
   ```bash
   git clone https://github.com/iPlug2/iPlug2OOS.git OpenGenLoss-Repo
   cd OpenGenLoss-Repo
   git submodule update --init --recursive
   ```
2. Copy this project **folder** (`OpenGenLoss/`) into `OpenGenLoss-Repo/Projects/` so the structure is:
   ```
   OpenGenLoss-Repo/
     iPlug2/           # submodule
     Projects/
       OpenGenLoss/    # <- this folder
   ```
3. Configure + build AU/VST3 with CMake/Xcode (example):
   ```bash
   cd Projects/OpenGenLoss
   # Create build directory
   mkdir -p build-macos && cd build-macos
   cmake -G "Xcode" -DIPLUG2_ROOT=../../iPlug2 -DIPLUG2_TARGETS="VST3;AUv2" ..
   cmake --build . --config Release
   ```
   - AU will be at: `build-macos/OpenGenLoss_artefacts/Release/AUv2/OpenGenLoss.component`
   - VST3 at: `build-macos/OpenGenLoss_artefacts/Release/VST3/OpenGenLoss.vst3`

> If you prefer working *inside the iPlug2 repo* (not OOS), place `OpenGenLoss/` under `iPlug2/Examples/` and adapt the CMake cache `-DIPLUG2_ROOT=../..` accordingly.

## Parameters
- **Wow Rate** (0.1–6 Hz), **Wow Depth**
- **Flutter Rate** (6–30 Hz), **Flutter Depth**
- **Dropouts Rate** (events/min), **Dropout Depth**, **Dropout Time** (ms)
- **Saturation** (0–100%)
- **Noise** (dB FS), **Hum** (dB), **Hum 50/60** (toggle)
- **Model LP** (kHz), **Model HS** (+/- dB)
- **Tilt** (+/- dB)
- **Spread** (0–100%)
- **Mix** (0–100%)
- **Output** (dB)
- **Tape Stop** (momentary button)

## Licenses
- Code in this folder: MIT
- You must comply with iPlug2 license and third-party SDK licenses for plugin formats (e.g., Steinberg VST3).

## Credits / References
- iPlug2 framework and examples (Oli Larkin et al.)
- ChowDSP writings and plugins for tape modeling inspiration
- Academic/industry references on wow & flutter, tape saturation, dropouts

See repository README for citation links.
