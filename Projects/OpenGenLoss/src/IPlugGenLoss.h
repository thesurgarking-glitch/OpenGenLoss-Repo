#pragma once

#include "IPlug_include_in_plug_hdr.h"
#include "IControls.h"
#include <random>
#include <memory>

class GenLossDSP;

enum EParams {
  kParamWowRate = 0,
  kParamWowDepth,
  kParamFlutterRate,
  kParamFlutterDepth,
  kParamDropRate,
  kParamDropDepth,
  kParamDropTime,
  kParamSaturation,
  kParamNoise,
  kParamHum,
  kParamHum50Hz,
  kParamModelLPkHz,
  kParamModelHSGain,
  kParamTilt,
  kParamSpread,
  kParamMix,
  kParamOutGain,
  kParamTapeStop, // momentary (0/1)
  kNumParams
};

class IPlugGenLoss final : public iplug::Plugin {
public:
  IPlugGenLoss(const iplug::InstanceInfo& info);
  ~IPlugGenLoss() override;

#if IPLUG_DSP
  void OnReset() override;
  void ProcessBlock(sample** inputs, sample** outputs, int nFrames) override;
#endif

#if IPLUG_EDITOR
  void OnParentWindowResize(int width, int height) override;
#endif

private:
  std::unique_ptr<GenLossDSP> mDSP;
};
