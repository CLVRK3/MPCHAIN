#pragma once

// MPChainDSP.h (v1.1) - Portable, zero-allocation C++17 DSP core for MPCHAIN by MIXCHAIN.
// Matches plugins/MPCHAIN/src/MPCHAIN.cpp algorithm-for-algorithm:
//   - Always-On Linear-Phase 3-Band EQ (513-tap exact symmetric FIR via 1024-pt OLA FFT, 512-sample latency)
//   - 512-sample latency-matched Dry delay line for zero-comb parallel Mix
//   - High-clarity TDR Nova-voiced 1.5:1 Vibe Compressor (dual Peak + RMS, 8 dB transparent knee)
//   - Shared 0..240 Hz LR4 Low HPF protecting BOTH Compression AND Width
//   - Transparent dynamic Treble Clean (3.6-6.2 kHz harshness peaks only + 14 kHz crystal air)
//   - Stereo Width Crossfader (0% Mono .. 100% Unity .. 200% Wide, protected below Low HPF)
//   - Gated K-Weighted Auto-Gain targeting -14 LUFS
//   - Selectable 1x / 2x / 4x / 8x polyphase half-band anti-aliasing oversampling
//   - Unified Loud + -1.5 dBFS Ceiling stage:
//       * 0..+4.5 dB: transparent gain + ultra-subtle cohesion (<0.02% THD) + transparent -1.5 dBFS limiter
//       * >+4.5 dB: smoothly blends subtle pre/post frequency balancing + high-clarity smooth-knee
//         gentle peak-shaving clipping strictly bounded at -1.5 dBFS (no harshness or pumping)

#include <algorithm>
#include <atomic>
#include <cmath>
#include <cstdint>
#include <cstring>

namespace mixchain {

constexpr float kPi           = 3.14159265358979323846f;
constexpr float kTwoPi        = 6.28318530717958647692f;
constexpr float kInvSqrt2     = 0.70710678118654752440f;
constexpr float kCeilingLin   = 0.84139514f; // -1.5 dBFS maximum ceiling
constexpr float kClipOnsetDb  = 4.5f;        // Loud > +4.5 dB naturally introduces gentle clipping
constexpr float kTargetLufs   = -14.0f;
constexpr int   kLinEqLatency = 512;

inline float clampf(float v, float lo, float hi) { return v < lo ? lo : (v > hi ? hi : v); }
inline float lerpf(float a, float b, float t)    { return a + (b - a) * t; }
inline float flushDenorm(float x)                { return std::fabs(x) < 1.0e-15f ? 0.0f : x; }
inline float sanitize(float x)                   { return std::isfinite(x) ? x : 0.0f; }
inline float dbToGain(float db)                  { return std::pow(10.0f, db * 0.05f); }
inline float gainToDb(float g, float fl = -96.0f){ return g > 1.0e-6f ? 20.0f * std::log10(g) : fl; }

inline float onePoleCoeff(float ms, double sr)
{
    if (ms <= 0.0f) return 1.0f;
    return 1.0f - std::exp(-1.0f / (ms * 0.001f * (float)sr));
}

inline float fastTanh(float x)
{
    if (x > 4.97f) return 1.0f;
    if (x < -4.97f) return -1.0f;
    const float x2 = x * x;
    const float a  = x * (135135.0f + x2 * (17325.0f + x2 * (378.0f + x2)));
    const float b  = 135135.0f + x2 * (62370.0f + x2 * (3150.0f + x2 * 28.0f));
    return a / b;
}

inline float processGentleClip(float x)
{
    constexpr float knee    = 0.76f * kCeilingLin;
    constexpr float head    = kCeilingLin - knee;
    constexpr float invHead = 1.0f / head;
    const float ax = std::fabs(x);
    if (ax <= knee) return x;
    const float shaped = knee + head * fastTanh((ax - knee) * invHead);
    const float c = shaped > kCeilingLin ? kCeilingLin : shaped;
    return x >= 0.0f ? c : -c;
}

struct Smoother
{
    float value = 0.0f, target = 0.0f, coeff = 1.0f;
    void setTime(float ms, double sr) { coeff = onePoleCoeff(ms, sr); }
    void setTarget(float t) { target = t; }
    void snap(float v) { value = target = v; }
    inline float process()
    {
        value += coeff * (target - value);
        if (std::fabs(target - value) < 1.0e-6f) value = target;
        return value;
    }
};

struct BiquadCoeffs
{
    float b0 = 1.0f, b1 = 0.0f, b2 = 0.0f, a1 = 0.0f, a2 = 0.0f;
    static BiquadCoeffs identity() { return {}; }
    static void common(double sr, float hz, float q, float& c, float& alpha)
    {
        hz = clampf(hz, 1.0f, 0.49f * (float)sr);
        if (q < 0.025f) q = 0.025f;
        const float w = kTwoPi * hz / (float)sr;
        c = std::cos(w);
        alpha = std::sin(w) / (2.0f * q);
    }
    static BiquadCoeffs norm(float b0, float b1, float b2, float a0, float a1, float a2)
    {
        BiquadCoeffs k; const float inv = 1.0f / a0;
        k.b0 = b0 * inv; k.b1 = b1 * inv; k.b2 = b2 * inv; k.a1 = a1 * inv; k.a2 = a2 * inv;
        return k;
    }
    static BiquadCoeffs lowpass(double sr, float hz, float q = kInvSqrt2)
    { float c, a; common(sr, hz, q, c, a); return norm((1.0f - c) * 0.5f, 1.0f - c, (1.0f - c) * 0.5f, 1.0f + a, -2.0f * c, 1.0f - a); }
    static BiquadCoeffs highpass(double sr, float hz, float q = kInvSqrt2)
    { float c, a; common(sr, hz, q, c, a); return norm((1.0f + c) * 0.5f, -(1.0f + c), (1.0f + c) * 0.5f, 1.0f + a, -2.0f * c, 1.0f - a); }
    static BiquadCoeffs bandpass(double sr, float hz, float q = 1.0f)
    { float c, a; common(sr, hz, q, c, a); return norm(a, 0.0f, -a, 1.0f + a, -2.0f * c, 1.0f - a); }
    static BiquadCoeffs peak(double sr, float hz, float q, float gainDb)
    { float c, a; common(sr, hz, q, c, a); const float A = std::pow(10.0f, gainDb / 40.0f);
      return norm(1.0f + a * A, -2.0f * c, 1.0f - a * A, 1.0f + a / A, -2.0f * c, 1.0f - a / A); }
    static BiquadCoeffs lowShelf(double sr, float hz, float q, float gainDb)
    { float c, a; common(sr, hz, q, c, a); const float A = std::pow(10.0f, gainDb / 40.0f); const float sa = 2.0f * std::sqrt(A) * a;
      return norm(A*((A+1)-(A-1)*c+sa), 2*A*((A-1)-(A+1)*c), A*((A+1)-(A-1)*c-sa), (A+1)+(A-1)*c+sa, -2*((A-1)+(A+1)*c), (A+1)+(A-1)*c-sa); }
    static BiquadCoeffs highShelf(double sr, float hz, float q, float gainDb)
    { float c, a; common(sr, hz, q, c, a); const float A = std::pow(10.0f, gainDb / 40.0f); const float sa = 2.0f * std::sqrt(A) * a;
      return norm(A*((A+1)+(A-1)*c+sa), -2*A*((A-1)+(A+1)*c), A*((A+1)+(A-1)*c-sa), (A+1)-(A-1)*c+sa, 2*((A-1)-(A+1)*c), (A+1)-(A-1)*c-sa); }
};

struct Biquad
{
    BiquadCoeffs c; float z1 = 0.0f, z2 = 0.0f;
    void setCoeffs(const BiquadCoeffs& k) { c = k; }
    void reset() { z1 = z2 = 0.0f; }
    inline float process(float x) { const float y = c.b0 * x + z1; z1 = flushDenorm(c.b1 * x - c.a1 * y + z2); z2 = flushDenorm(c.b2 * x - c.a2 * y); return y; }
};

struct StereoBiquad
{
    Biquad l, r;
    void setCoeffs(const BiquadCoeffs& k) { l.setCoeffs(k); r.setCoeffs(k); }
    void reset() { l.reset(); r.reset(); }
    inline void process(float& xL, float& xR) { xL = l.process(xL); xR = r.process(xR); }
};

struct Allpass1
{
    float a = 0.0f, x1 = 0.0f, y1 = 0.0f;
    void reset() { x1 = y1 = 0.0f; }
    inline float process(float x) { const float y = flushDenorm(a * (x - y1) + x1); x1 = x; y1 = y; return y; }
};

struct HalfBand4
{
    Allpass1 upA0, upA1, upB0, upB1, dnA0, dnA1, dnB0, dnB1;
    HalfBand4()
    {
        upA0.a = dnA0.a = 0.0798664262f; upA1.a = dnA1.a = 0.5453536510f;
        upB0.a = dnB0.a = 0.2838293448f; upB1.a = dnB1.a = 0.8344118914f;
    }
    void reset() { upA0.reset(); upA1.reset(); upB0.reset(); upB1.reset(); dnA0.reset(); dnA1.reset(); dnB0.reset(); dnB1.reset(); }
    inline void upsample(float x, float& y0, float& y1) { y0 = upA1.process(upA0.process(x)); y1 = upB1.process(upB0.process(x)); }
    inline float downsample(float x0, float x1) { return 0.5f * (dnA1.process(dnA0.process(x1)) + dnB1.process(dnB0.process(x0))); }
};

struct HalfBand2
{
    Allpass1 upA, upB, dnA, dnB;
    HalfBand2() { upA.a = dnA.a = 0.1617584983f; upB.a = dnB.a = 0.7330289323f; }
    void reset() { upA.reset(); upB.reset(); dnA.reset(); dnB.reset(); }
    inline void upsample(float x, float& y0, float& y1) { y0 = upA.process(x); y1 = upB.process(x); }
    inline float downsample(float x0, float x1) { return 0.5f * (dnA.process(x1) + dnB.process(x0)); }
};

struct Oversampler8x
{
    HalfBand4 s2; HalfBand2 s4, s8;
    void reset() { s2.reset(); s4.reset(); s8.reset(); }
    inline void upsample(int factor, float x, float out[8])
    {
        if (factor <= 1) { out[0] = x; return; }
        float b2[2]; s2.upsample(x, b2[0], b2[1]);
        if (factor == 2) { out[0] = b2[0]; out[1] = b2[1]; return; }
        float b4[4]; s4.upsample(b2[0], b4[0], b4[1]); s4.upsample(b2[1], b4[2], b4[3]);
        if (factor == 4) { for (int i = 0; i < 4; ++i) out[i] = b4[i]; return; }
        for (int i = 0; i < 4; ++i) s8.upsample(b4[i], out[i * 2], out[i * 2 + 1]);
    }
    inline float downsample(int factor, const float in[8])
    {
        if (factor <= 1) return in[0];
        if (factor == 2) return s2.downsample(in[0], in[1]);
        if (factor == 4) return s2.downsample(s4.downsample(in[0], in[1]), s4.downsample(in[2], in[3]));
        float b4[4];
        for (int i = 0; i < 4; ++i) b4[i] = s8.downsample(in[i * 2], in[i * 2 + 1]);
        return s2.downsample(s4.downsample(b4[0], b4[1]), s4.downsample(b4[2], b4[3]));
    }
};

class LinearPhaseEQ
{
public:
    static constexpr int kN = 1024, kLogN = 10, kHop = 256, kFirLen = 513, kHalfFir = 256;

    void prepare(double sr)
    {
        sr_ = sr > 8000.0 ? sr : 44100.0;
        for (int i = 0; i < kN; ++i)
        {
            int rev = 0, x = i;
            for (int b = 0; b < kLogN; ++b) { rev = (rev << 1) | (x & 1); x >>= 1; }
            bitRev_[i] = rev;
        }
        for (int i = 0; i < kN / 2; ++i)
        {
            const float ang = -kTwoPi * (float)i / (float)kN;
            twRe_[i] = std::cos(ang); twIm_[i] = std::sin(ang);
        }
        for (int n = 0; n < kFirLen; ++n)
        {
            const float p = kTwoPi * (float)n / (float)(kFirLen - 1);
            win_[n] = 0.42f - 0.50f * std::cos(p) + 0.08f * std::cos(2.0f * p);
        }
        lastLow_ = lastMid_ = lastHigh_ = 999.0f;
        updateKernel(0.0f, 0.0f, 0.0f);
        reset();
    }

    void reset()
    {
        std::memset(inL_, 0, sizeof(inL_)); std::memset(inR_, 0, sizeof(inR_));
        std::memset(outRingL_, 0, sizeof(outRingL_)); std::memset(outRingR_, 0, sizeof(outRingR_));
        inPos_ = 0; outRead_ = 0; outWrite_ = kHop;
    }

    void setGains(float lowDb, float midDb, float highDb)
    {
        if (std::fabs(lowDb - lastLow_) > 0.025f || std::fabs(midDb - lastMid_) > 0.025f || std::fabs(highDb - lastHigh_) > 0.025f)
            updateKernel(lowDb, midDb, highDb);
    }

    inline void processSample(float& l, float& r)
    {
        inL_[inPos_] = l; inR_[inPos_] = r; ++inPos_;
        const float yL = outRingL_[outRead_], yR = outRingR_[outRead_];
        outRingL_[outRead_] = 0.0f; outRingR_[outRead_] = 0.0f;
        outRead_ = (outRead_ + 1) & (kN - 1);
        if (inPos_ == kHop)
        {
            convolveHop(inL_, outRingL_);
            convolveHop(inR_, outRingR_);
            outWrite_ = (outWrite_ + kHop) & (kN - 1);
            inPos_ = 0;
        }
        l = yL; r = yR;
    }

private:
    static inline float biquadMag(const BiquadCoeffs& c, float cw, float sw, float c2w, float s2w)
    {
        const float nR = c.b0 + c.b1 * cw + c.b2 * c2w, nI = -c.b1 * sw - c.b2 * s2w;
        const float dR = 1.0f + c.a1 * cw + c.a2 * c2w, dI = -c.a1 * sw - c.a2 * s2w;
        const float d2 = dR * dR + dI * dI;
        return d2 < 1.0e-12f ? 1.0f : std::sqrt((nR * nR + nI * nI) / d2);
    }

    void fft(float* re, float* im, bool inverse) const
    {
        for (int i = 0; i < kN; ++i) { const int j = bitRev_[i]; if (j > i) { std::swap(re[i], re[j]); std::swap(im[i], im[j]); } }
        const float sign = inverse ? -1.0f : 1.0f;
        for (int len = 2; len <= kN; len <<= 1)
        {
            const int half = len >> 1, step = kN / len;
            for (int i = 0; i < kN; i += len)
            {
                int tw = 0;
                for (int j = 0; j < half; ++j)
                {
                    const float wr = twRe_[tw], wi = twIm_[tw] * sign; tw += step;
                    const int u = i + j, v = u + half;
                    const float tr = re[v] * wr - im[v] * wi, ti = re[v] * wi + im[v] * wr;
                    re[v] = re[u] - tr; im[v] = im[u] - ti; re[u] += tr; im[u] += ti;
                }
            }
        }
        if (inverse) { constexpr float invN = 1.0f / (float)kN; for (int i = 0; i < kN; ++i) { re[i] *= invN; im[i] *= invN; } }
    }

    void updateKernel(float lowDb, float midDb, float highDb)
    {
        lastLow_ = lowDb; lastMid_ = midDb; lastHigh_ = highDb;
        const BiquadCoeffs cLow  = BiquadCoeffs::lowShelf (sr_, 85.0f,   0.62f + 0.04f  * std::fabs(lowDb),  lowDb);
        const BiquadCoeffs cMid  = BiquadCoeffs::peak     (sr_, 1000.0f, 0.55f + 0.095f * std::fabs(midDb),  midDb);
        const BiquadCoeffs cHigh = BiquadCoeffs::highShelf(sr_, 8200.0f, 0.64f + 0.035f * std::fabs(highDb), highDb * 0.88f);
        const BiquadCoeffs cAir  = BiquadCoeffs::peak     (sr_, std::fmin(16500.0f, (float)(sr_ * 0.40)), 0.70f, highDb * 0.24f);

        for (int k = 0; k <= kN / 2; ++k)
        {
            const float w = kTwoPi * (float)k / (float)kN;
            const float cw = std::cos(w), sw = std::sin(w), c2w = std::cos(2.0f * w), s2w = std::sin(2.0f * w);
            const float m = biquadMag(cLow, cw, sw, c2w, s2w) * biquadMag(cMid, cw, sw, c2w, s2w)
                          * biquadMag(cHigh, cw, sw, c2w, s2w) * biquadMag(cAir, cw, sw, c2w, s2w);
            fftRe_[k] = m; fftIm_[k] = 0.0f;
            if (k > 0 && k < kN / 2) { fftRe_[kN - k] = m; fftIm_[kN - k] = 0.0f; }
        }
        fft(fftRe_, fftIm_, true);
        float h[kFirLen];
        for (int n = 0; n < kFirLen; ++n) h[n] = fftRe_[(n - kHalfFir + kN) & (kN - 1)] * win_[n];
        for (int n = 0; n < kHalfFir; ++n) { const float avg = 0.5f * (h[n] + h[kFirLen - 1 - n]); h[n] = h[kFirLen - 1 - n] = avg; }
        for (int i = 0; i < kN; ++i) { kernRe_[i] = (i < kFirLen) ? h[i] : 0.0f; kernIm_[i] = 0.0f; }
        fft(kernRe_, kernIm_, false);
    }

    void convolveHop(const float* hopIn, float* outRing)
    {
        for (int i = 0; i < kHop; ++i) { fftRe_[i] = hopIn[i]; fftIm_[i] = 0.0f; }
        for (int i = kHop; i < kN; ++i) { fftRe_[i] = 0.0f;    fftIm_[i] = 0.0f; }
        fft(fftRe_, fftIm_, false);
        for (int i = 0; i < kN; ++i)
        {
            const float r = fftRe_[i] * kernRe_[i] - fftIm_[i] * kernIm_[i];
            const float im = fftRe_[i] * kernIm_[i] + fftIm_[i] * kernRe_[i];
            fftRe_[i] = r; fftIm_[i] = im;
        }
        fft(fftRe_, fftIm_, true);
        for (int i = 0; i < kHop + kFirLen - 1; ++i) outRing[(outWrite_ + i) & (kN - 1)] += fftRe_[i];
    }

    double sr_ = 44100.0;
    int bitRev_[kN] = {};
    float twRe_[kN / 2] = {}, twIm_[kN / 2] = {}, win_[kFirLen] = {};
    float kernRe_[kN] = {}, kernIm_[kN] = {}, fftRe_[kN] = {}, fftIm_[kN] = {};
    float inL_[kHop] = {}, inR_[kHop] = {}, outRingL_[kN] = {}, outRingR_[kN] = {};
    int inPos_ = 0, outRead_ = 0, outWrite_ = kHop;
    float lastLow_ = 999.0f, lastMid_ = 999.0f, lastHigh_ = 999.0f;
};

struct LR4Crossover
{
    StereoBiquad lp1, lp2, hp1, hp2;
    void setCutoff(double sr, float hz)
    {
        hz = clampf(hz, 10.0f, 240.0f);
        auto lp = BiquadCoeffs::lowpass(sr, hz, kInvSqrt2), hp = BiquadCoeffs::highpass(sr, hz, kInvSqrt2);
        lp1.setCoeffs(lp); lp2.setCoeffs(lp); hp1.setCoeffs(hp); hp2.setCoeffs(hp);
    }
    void prepare(double sr, float hz) { setCutoff(sr, hz); reset(); }
    void reset() { lp1.reset(); lp2.reset(); hp1.reset(); hp2.reset(); }
    inline void split(float inL, float inR, float& loL, float& loR, float& hiL, float& hiR)
    {
        loL = inL; loR = inR; lp1.process(loL, loR); lp2.process(loL, loR);
        hiL = inL; hiR = inR; hp1.process(hiL, hiR); hp2.process(hiL, hiR);
    }
};

struct LoudnessMeter
{
    StereoBiquad preShelf, rlbHp;
    float peakEnv = 0.0f, peakHold = 0.0f;
    float msEnergy = 0.0f, shortEnergy = 0.0f, rmsEnergy = 0.0f;
    float peakRel = 0.001f, peakHoldRel = 0.0005f, lufsCoef = 0.001f, shortCoef = 0.0002f, rmsCoef = 0.001f;
    double intSum = 0.0;
    uint64_t intCount = 0;

    void prepare(double sr)
    {
        preShelf.setCoeffs(BiquadCoeffs::highShelf(sr, 1681.974f, 0.707175f, 3.99984f));
        rlbHp.setCoeffs(BiquadCoeffs::highpass(sr, 38.13547f, 0.500327f));
        peakRel     = onePoleCoeff(180.0f,  sr);
        peakHoldRel = onePoleCoeff(1400.0f, sr);
        lufsCoef    = onePoleCoeff(400.0f,  sr);
        shortCoef   = onePoleCoeff(3000.0f, sr);
        rmsCoef     = onePoleCoeff(300.0f,  sr);
        reset();
    }
    void reset()
    {
        preShelf.reset(); rlbHp.reset();
        peakEnv = peakHold = 0.0f;
        msEnergy = shortEnergy = rmsEnergy = 0.0f;
        resetIntegrated();
    }
    void resetIntegrated() { intSum = 0.0; intCount = 0; }

    inline void process(float l, float r)
    {
        const float pk = std::fmax(std::fabs(l), std::fabs(r));
        peakEnv  = pk > peakEnv  ? pk : flushDenorm(peakEnv  + peakRel     * (pk - peakEnv));
        peakHold = pk > peakHold ? pk : flushDenorm(peakHold + peakHoldRel * (pk - peakHold));
        const float sq = 0.5f * (l * l + r * r);
        rmsEnergy = flushDenorm(rmsEnergy + rmsCoef * (sq - rmsEnergy));

        float kl = l, kr = r; preShelf.process(kl, kr); rlbHp.process(kl, kr);
        const float kSq = kl * kl + kr * kr;
        msEnergy    = flushDenorm(msEnergy    + lufsCoef  * (kSq - msEnergy));
        shortEnergy = flushDenorm(shortEnergy + shortCoef * (kSq - shortEnergy));
        // Absolute gate at -65 LUFS (~3.7e-7 linear K-weighted energy)
        if (msEnergy > 3.7e-7f)
        {
            intSum += (double)msEnergy;
            ++intCount;
        }
    }
    float peakDb()      const { return gainToDb(peakEnv, -96.0f); }
    float peakHoldDb()  const { return gainToDb(peakHold, -96.0f); }
    float rmsDb()       const { return rmsEnergy <= 1.0e-10f ? -96.0f : (10.0f * std::log10(rmsEnergy)); }
    float lufsDb()      const { return msEnergy    <= 1.0e-10f ? -96.0f : (-0.691f + 10.0f * std::log10(msEnergy)); }
    float lufsShortDb() const { return shortEnergy <= 1.0e-10f ? -96.0f : (-0.691f + 10.0f * std::log10(shortEnergy)); }
    float lufsIntDb()   const
    {
        if (intCount == 0) return -96.0f;
        const double mean = intSum / (double)intCount;
        return mean <= 1.0e-10 ? -96.0f : (float)(-0.691 + 10.0 * std::log10(mean));
    }
    float plrDb() const
    {
        const float s = lufsShortDb();
        if (s <= -60.0f) return 0.0f;
        return clampf(peakHoldDb() - s, 0.0f, 30.0f);
    }
};

struct MPChainParams
{
    float outGainDb   = 0.0f;
    float inGainDb    = 0.0f;
    float loudDb      = 0.0f;
    float widthPct    = 100.0f;
    float eqLowDb     = 0.0f;
    float eqMidDb     = 0.0f;
    float eqHighDb    = 0.0f;
    bool  trebleClean = false;
    float vibePct     = 35.0f;
    bool  autoGain    = false;
    float mixPct      = 100.0f;
    int   hqIndex     = 1;     // 0=1x, 1=2x, 2=4x, 3=8x
    float lowHpfHz    = 150.0f;
};

struct MeterSnapshot
{
    float inPeakDb = -96.0f, inLufsDb = -96.0f;
    float outPeakDb = -96.0f, outLufsDb = -96.0f;
    float outLufsShortDb = -96.0f, outLufsIntDb = -96.0f;
    float outRmsDb = -96.0f, outPlrDb = 0.0f;
};

class MPChainDSP
{
public:
    void prepare(double sampleRate)
    {
        sr_ = sampleRate > 8000.0 ? sampleRate : 44100.0;
        inGainSm_.setTime(12.0f, sr_); outGainSm_.setTime(12.0f, sr_);
        loudGainSm_.setTime(15.0f, sr_); loudDbSm_.setTime(15.0f, sr_);
        widthSm_.setTime(15.0f, sr_); mixSm_.setTime(12.0f, sr_);
        vibeSm_.setTime(20.0f, sr_); hpfBlendSm_.setTime(15.0f, sr_);

        linEq_.prepare(sr_);
        vibeXover_.prepare(sr_, 150.0f); widthXover_.prepare(sr_, 150.0f);
        lastLowHpfHz_ = 150.0f;

        vibePkAtk_ = onePoleCoeff(7.0f, sr_); vibePkRel_ = onePoleCoeff(48.0f, sr_);
        vibeRmsAtk_ = onePoleCoeff(24.0f, sr_); vibeRmsRel_ = onePoleCoeff(130.0f, sr_);

        cleanDetBp_.setCoeffs(BiquadCoeffs::bandpass(sr_, 4300.0f, 1.45f));
        cleanAirShelf_.setCoeffs(BiquadCoeffs::highShelf(sr_, 14000.0f, 0.707f, 0.55f));
        cleanPkAtk_ = onePoleCoeff(4.0f, sr_); cleanPkRel_ = onePoleCoeff(50.0f, sr_); cleanRmsCoef_ = onePoleCoeff(32.0f, sr_);

        autoPreShelf_.setCoeffs(BiquadCoeffs::highShelf(sr_, 1681.974f, 0.707175f, 3.99984f));
        autoRlbHp_.setCoeffs(BiquadCoeffs::highpass(sr_, 38.13547f, 0.500327f));
        autoLufsCoef_ = onePoleCoeff(650.0f, sr_); autoGainCoef_ = onePoleCoeff(320.0f, sr_);

        lastBalAmt_ = -1.0f; updateClipBalanceFilters(0.0f);
        inMeter_.prepare(sr_); outMeter_.prepare(sr_);
        lastCleanCutDb_ = 999.0f;
        reset();
    }

    void reset()
    {
        inGainSm_.snap(1.0f); outGainSm_.snap(1.0f); loudGainSm_.snap(1.0f); loudDbSm_.snap(0.0f);
        widthSm_.snap(1.0f); mixSm_.snap(1.0f); vibeSm_.snap(0.35f); hpfBlendSm_.snap(1.0f);
        std::memset(dryDelayL_, 0, sizeof(dryDelayL_)); std::memset(dryDelayR_, 0, sizeof(dryDelayR_)); dryWrite_ = 0;
        linEq_.reset(); vibeXover_.reset(); widthXover_.reset();
        vibePkEnv_ = vibeRmsSq_ = 0.0f; vibeGain_ = vibeGainTgt_ = 1.0f; vibeSubCount_ = 0;
        cleanDetBp_.reset(); cleanDynBell_.reset(); cleanAirShelf_.reset(); cleanPkEnv_ = cleanRmsSq_ = 0.0f;
        autoPreShelf_.reset(); autoRlbHp_.reset(); autoMs_ = 0.0f; autoGainLin_ = autoGainTgt_ = 1.0f;
        clipPreSub_.reset(); clipPreMid_.reset(); clipPostSub_.reset(); clipPostAir_.reset();
        limGain_ = 1.0f; dcX1L_ = dcY1L_ = dcX1R_ = dcY1R_ = 0.0f;
        osL_.reset(); osR_.reset(); inMeter_.reset(); outMeter_.reset();
        std::memset(specRing_, 0, sizeof(specRing_));
        specWritePos_.store(0u, std::memory_order_relaxed);
    }

    MeterSnapshot processBlock(float* chL, float* chR, int numFrames, const MPChainParams& p)
    {
        inGainSm_.setTarget(dbToGain(clampf(p.inGainDb, -12.0f, 12.0f)));
        outGainSm_.setTarget(dbToGain(clampf(p.outGainDb, -12.0f, 12.0f)));
        const float loudDb = clampf(p.loudDb, 0.0f, 12.0f);
        loudGainSm_.setTarget(dbToGain(loudDb));
        loudDbSm_.setTarget(loudDb);
        widthSm_.setTarget(clampf(p.widthPct * 0.01f, 0.0f, 2.0f));
        vibeSm_.setTarget(clampf(p.vibePct * 0.01f, 0.0f, 1.0f));
        mixSm_.setTarget(clampf(p.mixPct * 0.01f, 0.0f, 1.0f));

        const int osFactor = 1 << (p.hqIndex < 0 ? 0 : (p.hqIndex > 3 ? 3 : p.hqIndex));
        const float limRelOs = onePoleCoeff(68.0f, sr_ * (double)osFactor);
        const float dcCoefOs = 1.0f - onePoleCoeff(32.0f, sr_ * (double)osFactor);

        linEq_.setGains(clampf(p.eqLowDb, -6.0f, 6.0f), clampf(p.eqMidDb, -6.0f, 6.0f), clampf(p.eqHighDb, -6.0f, 6.0f));

        const float targetHpfHz = clampf(p.lowHpfHz, 0.0f, 240.0f);
        hpfBlendSm_.setTarget(clampf(targetHpfHz * 0.1f, 0.0f, 1.0f));
        const float effHz = std::fmax(10.0f, targetHpfHz);
        if (std::fabs(effHz - lastLowHpfHz_) > 0.25f)
        {
            lastLowHpfHz_ += 0.35f * (effHz - lastLowHpfHz_);
            if (std::fabs(effHz - lastLowHpfHz_) < 0.35f) lastLowHpfHz_ = effHz;
            vibeXover_.setCutoff(sr_, lastLowHpfHz_); widthXover_.setCutoff(sr_, lastLowHpfHz_);
        }

        const float clipBlendBlock = clampf((loudDb - kClipOnsetDb) / (12.0f - kClipOnsetDb), 0.0f, 1.0f);
        if (std::fabs(clipBlendBlock - lastBalAmt_) > 0.015f) updateClipBalanceFilters(clipBlendBlock);

        if (p.trebleClean)
        {
            const float detDb = gainToDb(0.65f * std::sqrt(cleanRmsSq_) + 0.35f * cleanPkEnv_, -80.0f);
            float cutDb = detDb > -28.0f ? clampf(-(detDb + 28.0f) * 0.18f, -3.8f, 0.0f) : 0.0f;
            if (std::fabs(cutDb - lastCleanCutDb_) > 0.04f)
            {
                cleanDynBell_.setCoeffs(BiquadCoeffs::peak(sr_, 4300.0f, 1.25f + 0.18f * std::fabs(cutDb), cutDb));
                lastCleanCutDb_ = cutDb;
            }
        }

        autoGainTgt_ = (p.autoGain && autoMs_ > 3.16e-6f)
            ? dbToGain(clampf(kTargetLufs - (-0.691f + 10.0f * std::log10(autoMs_)), -15.0f, 18.0f)) : 1.0f;

        for (int i = 0; i < numFrames; ++i)
        {
            const float rawL = sanitize(chL[i]), rawR = chR ? sanitize(chR[i]) : rawL;
            inMeter_.process(rawL, rawR);

            const float dryL = dryDelayL_[dryWrite_], dryR = dryDelayR_[dryWrite_];
            dryDelayL_[dryWrite_] = rawL; dryDelayR_[dryWrite_] = rawR;
            dryWrite_ = (dryWrite_ + 1) & (kLinEqLatency - 1);

            const float inG = inGainSm_.process();
            float l = rawL * inG, r = rawR * inG;
            const float hpfBlend = hpfBlendSm_.process();

            const float vibe = vibeSm_.process();
            if (vibe > 0.0005f)
            {
                float loL = 0.0f, loR = 0.0f, hiL = l, hiR = r;
                if (hpfBlend > 0.0005f)
                {
                    float xLoL, xLoR, xHiL, xHiR; vibeXover_.split(l, r, xLoL, xLoR, xHiL, xHiR);
                    loL = xLoL * hpfBlend; loR = xLoR * hpfBlend; hiL = lerpf(l, xHiL, hpfBlend); hiR = lerpf(r, xHiR, hpfBlend);
                }
                const float instPk = std::fmax(std::fabs(hiL), std::fabs(hiR));
                vibePkEnv_ = flushDenorm(vibePkEnv_ + (instPk > vibePkEnv_ ? vibePkAtk_ : vibePkRel_) * (instPk - vibePkEnv_));
                const float instSq = 0.5f * (hiL * hiL + hiR * hiR);
                vibeRmsSq_ = flushDenorm(vibeRmsSq_ + (instSq > vibeRmsSq_ ? vibeRmsAtk_ : vibeRmsRel_) * (instSq - vibeRmsSq_));
                if ((vibeSubCount_++ & 7) == 0)
                {
                    const float detLvl = 0.72f * std::sqrt(vibeRmsSq_) + 0.28f * vibePkEnv_;
                    const float over = gainToDb(detLvl, -96.0f) - (-22.0f * vibe);
                    constexpr float kneeW = 8.0f, kSlope = 1.0f / 3.0f;
                    float grDb = 0.0f;
                    if (over > 0.5f * kneeW) grDb = -kSlope * over;
                    else if (over > -0.5f * kneeW) { const float x = over + 0.5f * kneeW; grDb = -kSlope * (x * x) / (2.0f * kneeW); }
                    vibeGainTgt_ = dbToGain(grDb + 1.4f * vibe * 0.35f);
                }
                vibeGain_ = flushDenorm(vibeGain_ + 0.25f * (vibeGainTgt_ - vibeGain_));
                l = loL + hiL * vibeGain_; r = loR + hiR * vibeGain_;
            }

            linEq_.processSample(l, r);

            if (p.trebleClean)
            {
                float dl = l, dr = r; cleanDetBp_.process(dl, dr);
                const float hPk = std::fmax(std::fabs(dl), std::fabs(dr));
                cleanPkEnv_ = flushDenorm(cleanPkEnv_ + (hPk > cleanPkEnv_ ? cleanPkAtk_ : cleanPkRel_) * (hPk - cleanPkEnv_));
                cleanRmsSq_ = flushDenorm(cleanRmsSq_ + cleanRmsCoef_ * (0.5f * (dl * dl + dr * dr) - cleanRmsSq_));
                cleanDynBell_.process(l, r); cleanAirShelf_.process(l, r);
            }

            const float width = widthSm_.process();
            if (std::fabs(width - 1.0f) > 0.0005f)
            {
                float wLoL = 0.0f, wLoR = 0.0f, wHiL = l, wHiR = r;
                if (hpfBlend > 0.0005f)
                {
                    float xLoL, xLoR, xHiL, xHiR; widthXover_.split(l, r, xLoL, xLoR, xHiL, xHiR);
                    wLoL = xLoL * hpfBlend; wLoR = xLoR * hpfBlend; wHiL = lerpf(l, xHiL, hpfBlend); wHiR = lerpf(r, xHiR, hpfBlend);
                }
                const float mid = 0.5f * (wHiL + wHiR), side = 0.5f * (wHiL - wHiR) * width;
                const float comp = width > 1.0f ? (1.0f / (1.0f + 0.10f * (width - 1.0f))) : 1.0f;
                l = wLoL + (mid + side) * comp; r = wLoR + (mid - side) * comp;
            }
            else if (hpfBlend > 0.0005f) { float d1, d2, d3, d4; widthXover_.split(l, r, d1, d2, d3, d4); }

            {
                float kl = l, kr = r; autoPreShelf_.process(kl, kr); autoRlbHp_.process(kl, kr);
                autoMs_ = flushDenorm(autoMs_ + autoLufsCoef_ * ((kl * kl + kr * kr) - autoMs_));
                autoGainLin_ = flushDenorm(autoGainLin_ + autoGainCoef_ * (autoGainTgt_ - autoGainLin_));
                l *= autoGainLin_; r *= autoGainLin_;
            }

            const float loudGain = loudGainSm_.process(), curLoudDb = loudDbSm_.process();
            const float loudAmt = curLoudDb * (1.0f / 12.0f);
            const float clipBlend = clampf((curLoudDb - kClipOnsetDb) / (12.0f - kClipOnsetDb), 0.0f, 1.0f);

            if (clipBlend > 0.001f) { clipPreSub_.process(l, r); clipPreMid_.process(l, r); }

            if (osFactor > 1)
            {
                float uL[8], uR[8]; osL_.upsample(osFactor, l, uL); osR_.upsample(osFactor, r, uR);
                for (int s = 0; s < osFactor; ++s) processLoudAndCeiling(uL[s], uR[s], loudGain, loudAmt, clipBlend, limRelOs, dcCoefOs);
                l = osL_.downsample(osFactor, uL); r = osR_.downsample(osFactor, uR);
            }
            else
            {
                processLoudAndCeiling(l, r, loudGain, loudAmt, clipBlend, limRelOs, dcCoefOs);
            }

            if (clipBlend > 0.001f) { clipPostSub_.process(l, r); clipPostAir_.process(l, r); l = processGentleClip(l); r = processGentleClip(r); }
            else                    { l = clampf(l, -kCeilingLin, kCeilingLin); r = clampf(r, -kCeilingLin, kCeilingLin); }

            const float mix = mixSm_.process(), outG = outGainSm_.process();
            const float finL = sanitize(lerpf(dryL, l, mix) * outG), finR = sanitize(lerpf(dryR, r, mix) * outG);
            outMeter_.process(finL, finR);
            const uint32_t wp = specWritePos_.load(std::memory_order_relaxed);
            specRing_[wp & (kSpecRingSize - 1)] = 0.5f * (finL + finR);
            specWritePos_.store(wp + 1u, std::memory_order_release);
            chL[i] = finL; if (chR) chR[i] = finR;
        }
        return {
            inMeter_.peakDb(), inMeter_.lufsDb(),
            outMeter_.peakDb(), outMeter_.lufsDb(),
            outMeter_.lufsShortDb(), outMeter_.lufsIntDb(),
            outMeter_.rmsDb(), outMeter_.plrDb()
        };
    }

    void copyRecentAudio(float* dst, int numSamples) const
    {
        if (!dst || numSamples <= 0 || numSamples > (int)kSpecRingSize) return;
        const uint32_t wp = specWritePos_.load(std::memory_order_acquire);
        uint32_t rp = wp - (uint32_t)numSamples;
        for (int i = 0; i < numSamples; ++i)
            dst[i] = specRing_[(rp + (uint32_t)i) & (kSpecRingSize - 1)];
    }

    void resetIntegratedLoudness() { outMeter_.resetIntegrated(); }
    double sampleRate() const { return sr_; }

private:
    void updateClipBalanceFilters(float amt)
    {
        lastBalAmt_ = amt;
        clipPreSub_.setCoeffs (BiquadCoeffs::lowShelf (sr_, 65.0f,    0.707f, -0.45f * amt));
        clipPreMid_.setCoeffs (BiquadCoeffs::peak     (sr_, 3400.0f,  1.25f,  -0.55f * amt));
        clipPostSub_.setCoeffs(BiquadCoeffs::lowShelf (sr_, 65.0f,    0.707f, +0.45f * amt));
        clipPostAir_.setCoeffs(BiquadCoeffs::highShelf(sr_, 11500.0f, 0.707f, +0.40f * amt));
    }

    inline void processLoudAndCeiling(float& l, float& r, float loudGain, float loudAmt, float clipBlend, float limRel, float dcCoef)
    {
        l *= loudGain; r *= loudGain;
        if (loudAmt > 0.001f)
        {
            const float satMix = loudAmt * 0.018f, asym = 0.0008f * loudAmt;
            l = lerpf(l, fastTanh(l * 1.05f + asym * l * l), satMix);
            r = lerpf(r, fastTanh(r * 1.05f + asym * r * r), satMix);
            const float dL = flushDenorm(l - dcX1L_ + dcCoef * dcY1L_); dcX1L_ = l; dcY1L_ = dL; l = dL;
            const float dR = flushDenorm(r - dcX1R_ + dcCoef * dcY1R_); dcX1R_ = r; dcY1R_ = dR; r = dR;
        }
        const float pk = std::fmax(std::fabs(l), std::fabs(r));
        const float needed = pk > kCeilingLin ? (kCeilingLin / pk) : 1.0f;
        limGain_ = needed < limGain_ ? needed : flushDenorm(limGain_ + limRel * (needed - limGain_));
        if (clipBlend <= 0.0005f) { l = clampf(l * limGain_, -kCeilingLin, kCeilingLin); r = clampf(r * limGain_, -kCeilingLin, kCeilingLin); }
        else                      { const float g = lerpf(limGain_, 1.0f, clipBlend * 0.85f); l = processGentleClip(l * g); r = processGentleClip(r * g); }
    }

    double sr_ = 44100.0;
    Smoother inGainSm_, outGainSm_, loudGainSm_, loudDbSm_, widthSm_, mixSm_, vibeSm_, hpfBlendSm_;
    float dryDelayL_[kLinEqLatency] = {}, dryDelayR_[kLinEqLatency] = {};
    int dryWrite_ = 0;
    LinearPhaseEQ linEq_;
    LR4Crossover vibeXover_, widthXover_;
    float lastLowHpfHz_ = 150.0f;
    float vibePkEnv_ = 0.0f, vibeRmsSq_ = 0.0f, vibeGain_ = 1.0f, vibeGainTgt_ = 1.0f;
    int vibeSubCount_ = 0;
    float vibePkAtk_ = 0.01f, vibePkRel_ = 0.001f, vibeRmsAtk_ = 0.005f, vibeRmsRel_ = 0.001f;
    StereoBiquad cleanDetBp_, cleanDynBell_, cleanAirShelf_;
    float cleanPkEnv_ = 0.0f, cleanRmsSq_ = 0.0f, cleanPkAtk_ = 0.01f, cleanPkRel_ = 0.001f, cleanRmsCoef_ = 0.001f, lastCleanCutDb_ = 999.0f;
    StereoBiquad autoPreShelf_, autoRlbHp_;
    float autoMs_ = 0.0f, autoGainLin_ = 1.0f, autoGainTgt_ = 1.0f, autoLufsCoef_ = 0.001f, autoGainCoef_ = 0.001f;
    StereoBiquad clipPreSub_, clipPreMid_, clipPostSub_, clipPostAir_;
    float lastBalAmt_ = -1.0f, limGain_ = 1.0f;
    float dcX1L_ = 0.0f, dcY1L_ = 0.0f, dcX1R_ = 0.0f, dcY1R_ = 0.0f;
    Oversampler8x osL_, osR_;
    LoudnessMeter inMeter_, outMeter_;
    static constexpr uint32_t kSpecRingSize = 4096;
    float specRing_[kSpecRingSize] = {};
    std::atomic<uint32_t> specWritePos_{ 0 };
};

} // namespace mixchain
