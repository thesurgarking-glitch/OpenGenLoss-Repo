# OpenGenLoss-Repo

This is a ready-to-build macOS AU/VST3 plugin inspired by VHS/tape generation loss, built with [iPlug2](https://github.com/iPlug2/iPlug2).

## Build

Clone this repo **with submodules**:

```bash
git clone --recurse-submodules https://github.com/YOURNAME/OpenGenLoss-Repo.git
cd OpenGenLoss-Repo/Projects/OpenGenLoss
mkdir build-macos && cd build-macos
cmake -G "Xcode" -DIPLUG2_ROOT=../../iPlug2 -DIPLUG2_TARGETS="VST3;AUv2" ..
cmake --build . --config Release
```

Artifacts will be built under `build-macos/OpenGenLoss_artefacts/Release/`.
