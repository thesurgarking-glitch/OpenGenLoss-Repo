#include "IPlugGenLoss.h"
#include "IPlug_include_in_plug_src.h"
#include "GenLossDSP.h"

using namespace iplug;
using namespace igraphics;

IPlugGenLoss::IPlugGenLoss(const InstanceInfo& info)
: Plugin(info, MakeConfig(kNumParams, 1))
{
  // Parameters
  GetParam(kParamWowRate)->InitDouble("Wow Rate", 1.0, 0.1, 6.0, 0.01, "Hz");
  GetParam(kParamWowDepth)->InitDouble("Wow Depth", 0.2, 0.0, 1.0, 0.01, "");
  GetParam(kParamFlutterRate)->InitDouble("Flutter Rate", 12.0, 6.0, 30.0, 0.1, "Hz");
  GetParam(kParamFlutterDepth)->InitDouble("Flutter Depth", 0.1, 0.0, 1.0, 0.01, "");
  GetParam(kParamDropRate)->InitDouble("Dropouts / min", 3.0, 0.0, 30.0, 0.1, "ev/min");
  GetParam(kParamDropDepth)->InitDouble("Dropout Depth", 0.6, 0.0, 1.0, 0.01, "");
  GetParam(kParamDropTime)->InitDouble("Dropout Time", 40.0, 5.0, 300.0, 1.0, "ms");
  GetParam(kParamSaturation)->InitDouble("Saturation", 25.0, 0.0, 100.0, 0.1, "%");
  GetParam(kParamNoise)->InitDouble("Noise (Hiss)", -60.0, -90.0, -30.0, 0.1, "dBFS");
  GetParam(kParamHum)->InitDouble("Hum", -70.0, -90.0, -30.0, 0.1, "dBFS");
  GetParam(kParamHum50Hz)->InitBool("Hum 50Hz", false, "Mains");
  GetParam(kParamModelLPkHz)->InitDouble("Model LP", 9.0, 2.0, 20.0, 0.1, "kHz");
  GetParam(kParamModelHSGain)->InitDouble("Model HS", 0.0, -12.0, 12.0, 0.1, "dB");
  GetParam(kParamTilt)->InitDouble("Tilt", 0.0, -6.0, 6.0, 0.1, "dB");
  GetParam(kParamSpread)->InitDouble("Spread", 50.0, 0.0, 100.0, 1.0, "%");
  GetParam(kParamMix)->InitDouble("Mix", 100.0, 0.0, 100.0, 0.1, "%");
  GetParam(kParamOutGain)->InitDouble("Output", 0.0, -24.0, 24.0, 0.1, "dB");
  GetParam(kParamTapeStop)->InitBool("Tape Stop", false);

#if IPLUG_EDITOR
  mMakeGraphicsFunc = [&]() {
    return MakeGraphics(*this, PLUG_WIDTH, PLUG_HEIGHT, PLUG_FPS, GetScaleForScreen(PLUG_WIDTH, PLUG_HEIGHT));
  };

  mLayoutFunc = [&](IGraphics* pGraphics) {
    pGraphics->AttachCornerResizer(EUIResizerMode::Scale, false);
    pGraphics->AttachPanelBackground(COLOR_DARK_GRAY);
    pGraphics->LoadFont("Roboto-Regular", ROBOTO_FN);

    IRECT b = pGraphics->GetBounds().GetPadded(-20);
    const int cols = 4;
    const int rows = 5;
    IRECT grid = b;
    float cellW = grid.W() / cols;
    float cellH = grid.H() / rows;

    auto addKnob = [&](int col, int row, int param) {
      IRECT r = IRECT(grid.L + col*cellW, grid.T + row*cellH, grid.L + (col+1)*cellW, grid.T + (row+1)*cellH).GetPadded(-10);
      pGraphics->AttachControl(new IVKnobControl(r, param));
    };

    addKnob(0,0,kParamWowRate);
    addKnob(1,0,kParamWowDepth);
    addKnob(2,0,kParamFlutterRate);
    addKnob(3,0,kParamFlutterDepth);

    addKnob(0,1,kParamDropRate);
    addKnob(1,1,kParamDropDepth);
    addKnob(2,1,kParamDropTime);
    addKnob(3,1,kParamSaturation);

    addKnob(0,2,kParamNoise);
    addKnob(1,2,kParamHum);
    pGraphics->AttachControl(new IVToggleSwitchControl(IRECT(grid.L + 2*cellW + 10, grid.T + 2*cellH + 10, grid.L + 3*cellW - 10, grid.T + 3*cellH - 10), kParamHum50Hz));

    addKnob(0,3,kParamModelLPkHz);
    addKnob(1,3,kParamModelHSGain);
    addKnob(2,3,kParamTilt);
    addKnob(3,3,kParamSpread);

    addKnob(0,4,kParamMix);
    addKnob(1,4,kParamOutGain);
    pGraphics->AttachControl(new IVButtonControl(IRECT(grid.L + 2*cellW + 10, grid.T + 4*cellH + 10, grid.L + 3*cellW - 10, grid.T + 5*cellH - 10), SplashClickActionFunc, "Tape Stop", DEFAULT_STYLE, kParamTapeStop));
  };
#endif

  mDSP = std::make_unique<GenLossDSP>();
}

IPlugGenLoss::~IPlugGenLoss() = default;

#if IPLUG_DSP
void IPlugGenLoss::OnReset()
{
  mDSP->reset(GetSampleRate(), GetBlockSize());
}

void IPlugGenLoss::ProcessBlock(sample** inputs, sample** outputs, int nFrames)
{
  const int nChans = NOutChansConnected();
  mDSP->setParams(
    GetParam(kParamWowRate)->Value(),
    GetParam(kParamWowDepth)->Value(),
    GetParam(kParamFlutterRate)->Value(),
    GetParam(kParamFlutterDepth)->Value(),
    GetParam(kParamDropRate)->Value(),
    GetParam(kParamDropDepth)->Value(),
    GetParam(kParamDropTime)->Value(),
    GetParam(kParamSaturation)->Value(),
    GetParam(kParamNoise)->Value(),
    GetParam(kParamHum)->Value(),
    GetParam(kParamHum50Hz)->Bool(),
    GetParam(kParamModelLPkHz)->Value(),
    GetParam(kParamModelHSGain)->Value(),
    GetParam(kParamTilt)->Value(),
    GetParam(kParamSpread)->Value(),
    GetParam(kParamMix)->Value() / 100.0,
    GetParam(kParamOutGain)->Value(),
    GetParam(kParamTapeStop)->Bool()
  );

  mDSP->process(inputs, outputs, nFrames, nChans);
}
#endif

#if IPLUG_EDITOR
void IPlugGenLoss::OnParentWindowResize(int width, int height)
{
  GetUI()->SetSizeConstraints(PLUG_WIDTH, PLUG_HEIGHT, width, height);
}
#endif
