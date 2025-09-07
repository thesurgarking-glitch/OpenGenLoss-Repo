#pragma once
#include <vector>
#include <cmath>
#include <random>
#include <array>
#include <algorithm>

class GenLossDSP {
public:
  GenLossDSP() = default;

  void reset(double sr, int /*block*/) {
    sampleRate = sr;
    initDelay();
    initFilters();
    rng.seed(0xBADDCAFE);
    tapeStopPhase = 0.0;
    tapeStopping = false;
    dropoutEnv = 1.0;
    dropoutPhase = 0;
    scheduleNextDropout();
  }

  void setParams(double wowRate, double wowDepth, double flutterRate, double flutterDepth,
                 double dropRatePerMin, double dropDepth, double dropMs,
                 double saturation, double noiseDB, double humDB, bool hum50Hz,
                 double modelLPkHz, double modelHSGainDB, double tiltDB,
                 double spreadPct, double mix, double outDB, bool tapeStopBtn)
  {
    wr = wowRate; wd = wowDepth;
    fr = flutterRate; fd = flutterDepth;
    dropRate = dropRatePerMin;
    dropAmt = dropDepth;
    dropTimeSamps = std::max(1, (int) std::round(dropMs * 0.001 * sampleRate));
    satAmt = saturation * 0.01;
    noiseAmp = dbToLin(noiseDB);
    humAmp = dbToLin(humDB);
    hum50 = hum50Hz;
    modelLP = modelLPkHz * 1000.0;
    modelHS = modelHSGainDB;
    tilt = tiltDB;
    spread = spreadPct * 0.01;
    wetMix = mix;
    outGain = dbToLin(outDB);

    tapeStopping = tapeStopBtn;
  }

  void process(float** in, float** out, int nFrames, int nChans) {
    if(nChans < 2) nChans = 2; // ensure stereo paths exist
    ensureBuffers(nFrames);

    for(int s=0; s<nFrames; ++s) {
      // Tape stop envelope
      double speed = 1.0;
      if(tapeStopping) {
        // Exponential ramp to zero over ~1.5s
        tapeStopPhase += 1.0 / (sampleRate * 1.5);
        speed = std::max(0.0, std::exp(-5.0 * tapeStopPhase));
      } else {
        tapeStopPhase = 0.0;
      }

      // Wow/Flutter combined fractional delay modulation per channel
      double wLFO = wd * std::sin(phaseW); // wow slow
      phaseW += 2.0 * M_PI * (wr * speed) / sampleRate;
      if(phaseW > 2.0*M_PI) phaseW -= 2.0*M_PI;

      // Flutter: use higher freq + small random jitter
      flutterJitter += 0.0005 * (uni(rng) - 0.5);
      flutterJitter = std::clamp(flutterJitter, -0.002, 0.002);
      double fLFO = fd * std::sin(phaseF) + flutterJitter;
      phaseF += 2.0 * M_PI * (fr * speed) / sampleRate;
      if(phaseF > 2.0*M_PI) phaseF -= 2.0*M_PI;

      double mod = (wLFO + fLFO) * 0.005; // +/- 0.5% speed -> about +/- 5 ms on 1s buffer; scaled later

      // Dropouts envelope (random AM)
      advanceDropout();

      for(int c=0; c<2; ++c) {
        double x = (in[c] ? in[c][s] : 0.0);

        // decorrelate modulation with small channel offset
        double spreadMod = mod * (c==0 ? (1.0 - 0.5*spread) : (1.0 + 0.5*spread));

        // fractional delay (short, ~10ms max) to simulate speed fluctuation (FM)
        double delayed = readFracDelay(c, spreadMod);

        // model filter (LP + HS shelf) + tilt
        double y = modelFilter(c, delayed);
        y = tiltFilter(c, y);

        // saturation with simple pre/de-emphasis
        y = saturate(c, y, satAmt);

        // apply dropout envelope (amplitude dip)
        y *= (1.0 - dropAmt*(1.0 - dropoutEnv));

        // add noise & hum
        double n = noiseAmp * tpdfNoise();
        double mains = humAmp * (hum50 ? std::sin(phaseHum50) + 0.3*std::sin(2*phaseHum50) + 0.15*std::sin(3*phaseHum50)
                                       : std::sin(phaseHum60) + 0.3*std::sin(2*phaseHum60) + 0.15*std::sin(3*phaseHum60));
        phaseHum50 += 2.0 * M_PI * 50.0 / sampleRate;
        phaseHum60 += 2.0 * M_PI * 60.0 / sampleRate;
        if(phaseHum50 > 2.0*M_PI) phaseHum50 -= 2.0*M_PI;
        if(phaseHum60 > 2.0*M_PI) phaseHum60 -= 2.0*M_PI;

        double wet = y + n + mains;

        // mix
        double dry = (in[c] ? in[c][s] : 0.0);
        double outS = (1.0 - wetMix)*dry + (wetMix)*wet;

        out[c][s] = (float)(outS * outGain);

        // write current input to delay buffer (after tape-stop speed affects the read pointer only)
        writeDelay(c, dry);
      }
    }
  }

private:
  // ===== Utility =====
  static double dbToLin(double db) { return std::pow(10.0, db/20.0); }
  double tpdfNoise() {
    return (uni(rng) - 0.5 + uni(rng) - 0.5);
  }

  // ===== Delay for wow/flutter =====
  static constexpr int kMaxDelaySamps = 2048; // ~46ms at 44.1k
  std::array<std::vector<double>, 2> delayBuf {{ std::vector<double>(kMaxDelaySamps, 0.0),
                                                  std::vector<double>(kMaxDelaySamps, 0.0) }};
  std::array<int,2> wIdx{{0,0}};
  double readFracDelay(int ch, double mod)
  {
    // base delay ~5ms, plus modulation up to +/-5ms * mod amount
    double base = 0.005 * sampleRate;
    double depth = 0.005 * sampleRate;
    double dSamps = std::clamp(base + mod * depth, 1.0, (double)kMaxDelaySamps-2);

    // read index behind write index
    double r = (double)wIdx[ch] - dSamps;
    while(r < 0) r += kMaxDelaySamps;
    int i0 = ((int)r) % kMaxDelaySamps;
    int i1 = (i0 + 1) % kMaxDelaySamps;
    double frac = r - std::floor(r);
    double y = delayBuf[ch][i0] * (1.0 - frac) + delayBuf[ch][i1] * frac;
    return y;
  }
  void writeDelay(int ch, double x)
  {
    delayBuf[ch][wIdx[ch]] = x;
    wIdx[ch] = (wIdx[ch] + 1) % kMaxDelaySamps;
  }
  void initDelay()
  {
    for(int c=0;c<2;++c){
      std::fill(delayBuf[c].begin(), delayBuf[c].end(), 0.0);
      wIdx[c]=0;
    }
    phaseW=phaseF=0.0;
    flutterJitter=0.0;
  }

  // ===== Filters =====
  struct BQ { double b0=1,b1=0,b2=0,a1=0,a2=0, z1=0,z2=0; };
  std::array<BQ,2> lp, hs, tiltLo, tiltHi;

  double biquad(BQ& s, double x)
  {
    double y = s.b0*x + s.z1;
    s.z1 = s.b1*x - s.a1*y + s.z2;
    s.z2 = s.b2*x - s.a2*y;
    return y;
  }

  void calcLP(BQ& s, double fc)
  {
    double w0 = 2.0*M_PI*fc/sampleRate;
    double alpha = std::sin(w0)/std::sqrt(2.0);
    double c = std::cos(w0);
    double b0 = (1-c)/2;
    double b1 = 1-c;
    double b2 = (1-c)/2;
    double a0 = 1 + alpha;
    double a1 = -2*c;
    double a2 = 1 - alpha;
    s.b0=b0/a0; s.b1=b1/a0; s.b2=b2/a0; s.a1=a1/a0; s.a2=a2/a0;
  }

  void calcHShelf(BQ& s, double fc, double gainDB)
  {
    double A = std::pow(10.0, gainDB/40.0);
    double w0 = 2.0*M_PI*fc/sampleRate;
    double c = std::cos(w0);
    double alpha = std::sin(w0)/2.0 * std::sqrt((A + 1/A)*(1/0.707 - 1) + 2);
    double b0 =    A*((A+1) + (A-1)*c + 2*std::sqrt(A)*alpha);
    double b1 = -2*A*((A-1) + (A+1)*c);
    double b2 =    A*((A+1) + (A-1)*c - 2*std::sqrt(A)*alpha);
    double a0 =       (A+1) - (A-1)*c + 2*std::sqrt(A)*alpha;
    double a1 =  2*((A-1) - (A+1)*c);
    double a2 =       (A+1) - (A-1)*c - 2*std::sqrt(A)*alpha;
    s.b0=b0/a0; s.b1=b1/a0; s.b2=b2/a0; s.a1=a1/a0; s.a2=a2/a0;
  }

  void calcTiltPair(BQ& lo, BQ& hi, double tiltDB)
  {
    // Simple 1st-order tilt approximated with two shelves crossing ~1kHz
    double fc = 1000.0;
    calcHShelf(hi, fc,  tiltDB);
    calcHShelf(lo, fc, -tiltDB);
  }

  double modelFilter(int ch, double x)
  {
    // LP around modelLP and high-shelf adjust around 6kHz (crude VHS/tape tone sketch)
    calcLP(lp[ch], std::clamp(modelLP, 1000.0, sampleRate*0.45));
    calcHShelf(hs[ch], 6000.0, modelHS);
    double y = biquad(lp[ch], x);
    y = biquad(hs[ch], y);
    return y;
  }

  double tiltFilter(int ch, double x)
  {
    calcTiltPair(tiltLo[ch], tiltHi[ch], tilt);
    double y = biquad(tiltLo[ch], x);
    y = biquad(tiltHi[ch], y);
    return y;
  }

  void initFilters()
  {
    for(int c=0;c<2;++c){ lp[c]=BQ{}; hs[c]=BQ{}; tiltLo[c]=BQ{}; tiltHi[c]=BQ{}; }
  }

  // ===== Saturation =====
  double saturate(int /*ch*/, double x, double amt)
  {
    // pre-emphasis
    double pre = x + 0.1*(x - prevX);
    prevX = x;
    double drive = (1.0 + 9.0*amt);
    double y = std::tanh(pre * drive);
    // de-emphasis
    double out = 0.9*y + 0.1*prevY;
    prevY = out;
    return out;
  }

  // ===== Dropouts =====
  void scheduleNextDropout()
  {
    // Poisson process: exponential inter-arrival in seconds
    double ratePerSec = dropRate / 60.0;
    if(ratePerSec <= 0.0) {
      nextDropIn = std::numeric_limits<int>::max();
      return;
    }
    double u = std::max(1e-9, uni(rng));
    double secs = -std::log(u) / ratePerSec;
    nextDropIn = std::max(1, (int)std::round(secs * sampleRate));
  }

  void advanceDropout()
  {
    if(nextDropIn==std::numeric_limits<int>::max()){
      dropoutEnv = 1.0;
      return;
    }
    if(nextDropIn > 0) {
      --nextDropIn;
      dropoutEnv = std::min(1.0, dropoutEnv + 0.005); // relax toward 1
    } else {
      // in dropout: ease down then up over dropTimeSamps
      ++dropoutPhase;
      double t = (double)dropoutPhase / std::max(1, dropTimeSamps);
      if(t<0.5) dropoutEnv = 1.0 - 2.0*t; // down
      else      dropoutEnv = 2.0*t - 1.0; // up
      dropoutEnv = std::clamp(dropoutEnv, 0.0, 1.0);
      if(dropoutPhase >= dropTimeSamps) {
        dropoutPhase = 0;
        scheduleNextDropout();
      }
    }
  }

  // ===== State =====
  double sampleRate = 44100.0;

  // wow/flutter
  double wr=1.0, wd=0.2, fr=12.0, fd=0.1;
  double phaseW=0.0, phaseF=0.0, flutterJitter=0.0;

  // Tape stop
  bool tapeStopping=false;
  double tapeStopPhase=0.0;

  // Filters/tilt
  double modelLP = 9000.0;
  double modelHS = 0.0;
  double tilt = 0.0;

  // saturation
  double satAmt=0.25, prevX=0.0, prevY=0.0;

  // noise/hum
  double noiseAmp = 0.0, humAmp = 0.0; bool hum50 = false;
  double phaseHum50=0.0, phaseHum60=0.0;

  // spread/mix/out
  double spread=0.5, wetMix=1.0, outGain=1.0;

  // dropouts
  double dropRate=3.0, dropAmt=0.6; int dropTimeSamps=200;
  int nextDropIn = 1000000000;
  int dropoutPhase = 0;
  double dropoutEnv = 1.0;

  // RNG
  std::mt19937 rng {0xBADDCAFE};
  std::uniform_real_distribution<double> uni {0.0, 1.0};

  // buffers
  void ensureBuffers(int /*nFrames*/) {}
};
