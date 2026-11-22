#pragma once

// MPChainDSP.h (v1.2.0) - Portable, zero-allocation C++17 DSP core for MPCHAIN by MIXCHAIN.
// Matches plugins/MPCHAIN/src/MPCHAIN.cpp algorithm-for-algorithm:
//   Signal Flow (EQ, Glue Comp, Air, Width, Auto-Gain are ALL independent & PRE-LOUDER):
//     1. Input Multimeter + In Gain (-12..+12 dB)
//     2. 1.5:1 Feed-Forward "Glue Comp" (dual Peak + RMS, 8 dB knee, < HPF uncompressed)
//     3. Always-On 4-Band Linear-Phase EQ (513-tap symmetric FIR via 1024-pt OLA FFT, 512-sample PDC):
//          Low (30-120 Hz), Low Mid (120 Hz-1.5 kHz), Mid (1.5-7 kHz), High (> 7 kHz)
//        + Smooth +1.0 dB Linear-Phase "Air" Shelf + Dynamic Treble "Clean"
//     4. Stereo Width Crossfader (0%..200%, protected below shared 0-240 Hz LR4 HPF)
//     5. Gated K-Weighted Auto-Gain (Off / -14 / -11 / -9 LUFS)
//     6. "LOUDER" Stage (0..+24 dB) inside HQ Oversampling (Native / 2x 96k / 4x 192k / 8x 384k):
//          * At 0.0 dB: zero impact on incoming signal (gain/sat/clip bypassed)
//          * Subtle Stereo-Bus Saturation (~1% wet per +0.5 dB of Louder):
//              4 selectable & bypassable modes: Tape, Tube, Transformer (Xfmr), Class-A
//          * Above +9.0 dB: smoothly engages frequency-balanced smooth-knee gentle peak-shaving
//          * Slow-release transparent mastering limiter bounded at user Ceiling (-14.0..0.0 dB, default 0.0 dB)
//     7. Out Gain (-12..+12 dB) + Output Multimeter + Lock-Free Spectrum Ring Tap

#include <algorithm>
#include <atomic>
#include <cmath>
#include <cstdint>
#include <cstring>

namespace mixchain {

constexpr float kPi           = 3.14159265358979323846f;
constexpr float kTwoPi        = 6.28318530717958647692f;
constexpr float kInvSqrt2     = 0.70710678118654752440f;
constexpr float kClipOnsetDb  = 9.0f;  // LOUDER > +9.0 dB smoothly introduces gentle clipping
constexpr int   kLinEqLatency = 512;

enum SatMode : int
{
    kSatOff    = 0,
    kSatTape   = 1,
    kSatTube   = 2,
    kSatXfmr   = 3,
    kSatClassA = 4
};

enum AutoTarget : int
{
    kAutoOff = 0,
    kAuto14  = 1, // -14 LUFS
    kAuto11  = 2, // -11 LUFS
    kAuto9   = 3  //  -9 LUFS
};

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
    if (x > 4.5f) return 1.0f;
    if (x < -4.5f) return -1.0f;
    const float x2 = x * x;
    return x * (27.0f + x2) / (27.0f + 9.0f * x2);
}

struct Smoother
{
    float cur = 0.0f, tgt = 0.0f, coef = 0.01f;
    void setTime(float ms, double sr) { coef = onePoleCoeff(ms, sr); }
    void snap(float v)                { cur = tgt = v; }
    void setTarget(float v)           { tgt = v; }
    inline float process()            { cur = flushDenorm(cur + coef * (tgt - cur)); return cur; }
};

struct BiquadCoeffs
{
    float b0 = 1, b1 = 0, b2 = 0, a1 = 0, a2 = 0;

    static BiquadCoeffs lowpass(double sr, float freq, float q)
    {
        freq = clampf(freq, 10.0f, (float)sr * 0.45f); q = std::fmax(q, 0.1f);
        const float w = kTwoPi * freq / (float)sr, cw = std::cos(w), sw = std::sin(w), al = sw / (2.0f * q), inv = 1.0f / (1.0f + al);
        return { (1.0f - cw) * 0.5f * inv, (1.0f - cw) * inv, (1.0f - cw) * 0.5f * inv, -2.0f * cw * inv, (1.0f - al) * inv };
    }
    static BiquadCoeffs highpass(double sr, float freq, float q)
    {
        freq = clampf(freq, 10.0f, (float)sr * 0.45f); q = std::fmax(q, 0.1f);
        const float w = kTwoPi * freq / (float)sr, cw = std::cos(w), sw = std::sin(w), al = sw / (2.0f * q), inv = 1.0f / (1.0f + al);
        return { (1.0f + cw) * 0.5f * inv, -(1.0f + cw) * inv, (1.0f + cw) * 0.5f * inv, -2.0f * cw * inv, (1.0f - al) * inv };
    }
    static BiquadCoeffs bandpass(double sr, float freq, float q)
    {
        freq = clampf(freq, 20.0f, (float)sr * 0.45f); q = std::fmax(q, 0.1f);
        const float w = kTwoPi * freq / (float)sr, cw = std::cos(w), sw = std::sin(w), al = sw / (2.0f * q), inv = 1.0f / (1.0f + al);
        return { al * inv, 0.0f, -al * inv, -2.0f * cw * inv, (1.0f - al) * inv };
    }
    static BiquadCoeffs peak(double sr, float freq, float q, float gainDb)
    {
        freq = clampf(freq, 20.0f, (float)sr * 0.45f); q = std::fmax(q, 0.1f);
        const float A = std::pow(10.0f, gainDb / 40.0f), w = kTwoPi * freq / (float)sr, cw = std::cos(w), sw = std::sin(w), al = sw / (2.0f * q);
        const float inv = 1.0f / (1.0f + al / A);
        return { (1.0f + al * A) * inv, -2.0f * cw * inv, (1.0f - al * A) * inv, -2.0f * cw * inv, (1.0f - al / A) * inv };
    }
    static BiquadCoeffs lowShelf(double sr, float freq, float q, float gainDb)
    {
        freq = clampf(freq, 20.0f, (float)sr * 0.45f); q = std::fmax(q, 0.1f);
        const float A = std::pow(10.0f, gainDb / 40.0f), w = kTwoPi * freq / (float)sr, cw = std::cos(w), sw = std::sin(w);
        const float al = sw / (2.0f * q), sA = 2.0f * std::sqrt(A) * al;
        const float inv = 1.0f / ((A + 1.0f) + (A - 1.0f) * cw + sA);
        return { A * ((A + 1.0f) - (A - 1.0f) * cw + sA) * inv, 2.0f * A * ((A - 1.0f) - (A + 1.0f) * cw) * inv,
                 A * ((A + 1.0f) - (A - 1.0f) * cw - sA) * inv, -2.0f * ((A - 1.0f) + (A + 1.0f) * cw) * inv,
                 ((A + 1.0f) + (A - 1.0f) * cw - sA) * inv };
    }
    static BiquadCoeffs highShelf(double sr, float freq, float q, float gainDb)
    {
        freq = clampf(freq, 20.0f, (float)sr * 0.45f); q = std::fmax(q, 0.1f);
        const float A = std::pow(10.0f, gainDb / 40.0f), w = kTwoPi * freq / (float)sr, cw = std::cos(w), sw = std::sin(w);
        const float al = sw / (2.0f * q), sA = 2.0f * std::sqrt(A) * al;
        const float inv = 1.0f / ((A + 1.0f) - (A - 1.0f) * cw + sA);
        return { A * ((A + 1.0f) + (A - 1.0f) * cw + sA) * inv, -2.0f * A * ((A - 1.0f) + (A + 1.0f) * cw) * inv,
                 A * ((A + 1.0f) + (A - 1.0f) * cw - sA) * inv, 2.0f * ((A - 1.0f) - (A + 1.0f) * cw) * inv,
                 ((A + 1.0f) - (A - 1.0f) * cw - sA) * inv };
    }
};

struct StereoBiquad
{
    BiquadCoeffs c;
    float s1L = 0, s2L = 0, s1R = 0, s2R = 0;
    void setCoeffs(const BiquadCoeffs& nc) { c = nc; }
    void reset() { s1L = s2L = s1R = s2R = 0.0f; }
    inline void process(float& l, float& r)
    {
        const float yL = c.b0 * l + s1L;
        s1L = flushDenorm(c.b1 * l - c.a1 * yL + s2L);
        s2L = flushDenorm(c.b2 * l - c.a2 * yL);
        l = yL;
        const float yR = c.b0 * r + s1R;
        s1R = flushDenorm(c.b1 * r - c.a1 * yR + s2R);
        s2R = flushDenorm(c.b2 * r - c.a2 * yR);
        r = yR;
    }
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
    inline float downsample(float y0, float y1) { return 0.5f * (dnA1.process(dnA0.process(y0)) + dnB1.process(dnB0.process(y1))); }
};

struct HalfBand2
{
    Allpass1 upA, upB, dnA, dnB;
    HalfBand2() { upA.a = dnA.a = 0.1617584983f; upB.a = dnB.a = 0.7330289323f; }
    void reset() { upA.reset(); upB.reset(); dnA.reset(); dnB.reset(); }
    inline void upsample(float x, float& y0, float& y1) { y0 = upA.process(x); y1 = upB.process(x); }
    inline float downsample(float y0, float y1) { return 0.5f * (dnA.process(y0) + dnB.process(y1)); }
};

struct Oversampler8x
{
    HalfBand4 s1; HalfBand2 s2, s3;
    void reset() { s1.reset(); s2.reset(); s3.reset(); }
    inline void upsample(int factor, float x, float* out)
    {
        if (factor == 2) { s1.upsample(x, out[0], out[1]); }
        else if (factor == 4) { float a, b; s1.upsample(x, a, b); s2.upsample(a, out[0], out[1]); s2.upsample(b, out[2], out[3]); }
        else { float a, b, c0, c1, c2, c3; s1.upsample(x, a, b); s2.upsample(a, c0, c1); s2.upsample(b, c2, c3);
               s3.upsample(c0, out[0], out[1]); s3.upsample(c1, out[2], out[3]); s3.upsample(c2, out[4], out[5]); s3.upsample(c3, out[6], out[7]); }
    }
    inline float downsample(int factor, const float* in)
    {
        if (factor == 2) return s1.downsample(in[0], in[1]);
        if (factor == 4) return s1.downsample(s2.downsample(in[0], in[1]), s2.downsample(in[2], in[3]));
        const float c0 = s3.downsample(in[0], in[1]), c1 = s3.downsample(in[2], in[3]);
        const float c2 = s3.downsample(in[4], in[5]), c3 = s3.downsample(in[6], in[7]);
        return s1.downsample(s2.downsample(c0, c1), s2.downsample(c2, c3));
    }
};

// Always-On 4-Band Linear-Phase EQ + Smooth +1.0 dB Linear-Phase Air Shelf
// Crossovers:
//   Low     :  30 - 120 Hz
//   Low Mid : 120 - 1.5 kHz
//   Mid     : 1.5 - 7.0 kHz
//   High    : Above 7.0 kHz
//   Air     : Smooth +1.0 dB upper-air shelf (zero phase distortion)
class LinearPhaseEQ4
{
public:
    static constexpr int kOrder  = 10;
    static constexpr int kN      = 1 << kOrder; // 1024
    static constexpr int kHop    = 256;
    static constexpr int kFirLen = 513;
    static constexpr int kCenter = 256;

    void prepare(double sampleRate)
    {
        sr_ = sampleRate > 8000.0 ? sampleRate : 44100.0;
        for (int i = 0; i < kN; ++i)
        {
            int r = 0, x = i;
            for (int b = 0; b < kOrder; ++b) { r = (r << 1) | (x & 1); x >>= 1; }
            bitRev_[i] = r;
        }
        for (int i = 0; i < kN / 2; ++i)
        {
            const float a = -kTwoPi * (float)i / (float)kN;
            twRe_[i] = std::cos(a); twIm_[i] = std::sin(a);
        }
        for (int n = 0; n < kFirLen; ++n)
        {
            const float p = kTwoPi * (float)n / (float)(kFirLen - 1);
            win_[n] = 0.42f - 0.50f * std::cos(p) + 0.08f * std::cos(2.0f * p);
        }
        lastLow_ = lastLowMid_ = lastMid_ = lastHigh_ = 999.0f;
        lastAir_ = -1;
        setGains(0.0f, 0.0f, 0.0f, 0.0f, false);
        reset();
    }

    void reset()
    {
        std::memset(inL_, 0, sizeof(inL_)); std::memset(inR_, 0, sizeof(inR_));
        std::memset(outRingL_, 0, sizeof(outRingL_)); std::memset(outRingR_, 0, sizeof(outRingR_));
        inPos_ = 0; outRead_ = 0; outWrite_ = kHop;
    }

    void setGains(float lowDb, float lowMidDb, float midDb, float highDb, bool airOn)
    {
        const int airI = airOn ? 1 : 0;
        if (std::fabs(lowDb - lastLow_) < 0.01f &&
            std::fabs(lowMidDb - lastLowMid_) < 0.01f &&
            std::fabs(midDb - lastMid_) < 0.01f &&
            std::fabs(highDb - lastHigh_) < 0.01f &&
            airI == lastAir_) return;
        lastLow_ = lowDb; lastLowMid_ = lowMidDb; lastMid_ = midDb; lastHigh_ = highDb; lastAir_ = airI;
        rebuildKernel(lowDb, lowMidDb, midDb, highDb, airOn);
    }

    inline void processSample(float& l, float& r)
    {
        const float yL = outRingL_[outRead_], yR = outRingR_[outRead_];
        outRingL_[outRead_] = 0.0f; outRingR_[outRead_] = 0.0f;
        outRead_ = (outRead_ + 1) & (kN - 1);
        inL_[inPos_] = l; inR_[inPos_] = r;
        if (++inPos_ >= kHop)
        {
            convolveHop(inL_, outRingL_); convolveHop(inR_, outRingR_);
            outWrite_ = (outWrite_ + kHop) & (kN - 1); inPos_ = 0;
        }
        l = yL; r = yR;
    }

private:
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
        if (inverse) { const float inv = 1.0f / (float)kN; for (int i = 0; i < kN; ++i) { re[i] *= inv; im[i] *= inv; } }
    }

    void rebuildKernel(float lowDb, float lowMidDb, float midDb, float highDb, bool airOn)
    {
        float re[kN] = {}, im[kN] = {};
        const bool isFlat = (std::fabs(lowDb) < 0.01f && std::fabs(lowMidDb) < 0.01f &&
                             std::fabs(midDb) < 0.01f && std::fabs(highDb) < 0.01f && !airOn);
        if (isFlat) { re[kCenter] = 1.0f; }
        else
        {
            const float sr = (float)sr_;
            for (int k = 0; k <= kN / 2; ++k)
            {
                const float f = ((float)k * sr) / (float)kN;
                float totalDb = 0.0f;
                if (f > 1.0f)
                {
                    // Exact complementary Linkwitz-Riley-style partition of unity:
                    //   Low: 30-120 Hz | Low Mid: 120-1.5 kHz | Mid: 1.5-7 kHz | High: > 7 kHz
                    const float rSub = f / 26.0f,   rSub4 = rSub * rSub * rSub * rSub;
                    const float r1   = f / 120.0f,  r1_4  = r1 * r1 * r1 * r1;
                    const float r2   = f / 1500.0f, r2_4  = r2 * r2 * r2 * r2;
                    const float r3   = f / 7000.0f, r3_4  = r3 * r3 * r3 * r3;

                    const float s1 = 1.0f / (1.0f + r1_4);
                    const float s2 = 1.0f / (1.0f + r2_4);
                    const float s3 = 1.0f / (1.0f + r3_4);
                    const float subHp = rSub4 / (1.0f + rSub4); // preserves DC neutrality below ~26 Hz

                    const float wLow    = s1 * subHp;
                    const float wLowMid = s2 - s1;
                    const float wMid    = s3 - s2;
                    const float wHigh   = 1.0f - s3;

                    totalDb = lowDb * wLow + lowMidDb * wLowMid + midDb * wMid + highDb * wHigh;
                    if (airOn)
                    {
                        // Ultra-smooth +1.0 dB mastering air shelf gently rising from ~8 kHz into upper air
                        const float ra = f / 11000.0f;
                        const float ra3 = ra * ra * ra;
                        totalDb += 1.0f * (ra3 / (1.0f + ra3));
                    }
                }
                const float mag = dbToGain(totalDb);
                const float ph  = -kTwoPi * (float)k * (float)kCenter / (float)kN;
                re[k] = mag * std::cos(ph); im[k] = mag * std::sin(ph);
                if (k > 0 && k < kN / 2) { re[kN - k] = re[k]; im[kN - k] = -im[k]; }
            }
            fft(re, im, true);
            for (int n = 0; n < kFirLen; ++n) { re[n] *= win_[n]; im[n] = 0.0f; }
            for (int n = 0; n < kCenter; ++n) { const float s = 0.5f * (re[n] + re[kFirLen - 1 - n]); re[n] = re[kFirLen - 1 - n] = s; }
            for (int n = kFirLen; n < kN; ++n) { re[n] = 0.0f; im[n] = 0.0f; }
        }
        fft(re, im, false);
        std::memcpy(kernRe_, re, sizeof(re)); std::memcpy(kernIm_, im, sizeof(im));
    }

    void convolveHop(const float* inBuf, float* outRing)
    {
        std::memcpy(fftRe_, inBuf, kHop * sizeof(float));
        std::memset(fftRe_ + kHop, 0, (kN - kHop) * sizeof(float));
        std::memset(fftIm_, 0, sizeof(fftIm_));
        fft(fftRe_, fftIm_, false);
        for (int k = 0; k < kN; ++k)
        {
            const float a = fftRe_[k], b = fftIm_[k], c = kernRe_[k], d = kernIm_[k];
            fftRe_[k] = a * c - b * d; fftIm_[k] = a * d + b * c;
        }
        fft(fftRe_, fftIm_, true);
        int wp = outWrite_;
        for (int n = 0; n < kHop + kFirLen - 1; ++n) { outRing[wp] += fftRe_[n]; wp = (wp + 1) & (kN - 1); }
    }

    double sr_ = 44100.0;
    int bitRev_[kN] = {};
    float twRe_[kN / 2] = {}, twIm_[kN / 2] = {}, win_[kFirLen] = {};
    float kernRe_[kN] = {}, kernIm_[kN] = {}, fftRe_[kN] = {}, fftIm_[kN] = {};
    float inL_[kHop] = {}, inR_[kHop] = {}, outRingL_[kN] = {}, outRingR_[kN] = {};
    int inPos_ = 0, outRead_ = 0, outWrite_ = kHop;
    float lastLow_ = 999.0f, lastLowMid_ = 999.0f, lastMid_ = 999.0f, lastHigh_ = 999.0f;
    int lastAir_ = -1;
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
        if (msEnergy > 3.7e-7f) { intSum += (double)msEnergy; ++intCount; }
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
    float louderDb    = 0.0f;   // 0..+24 dB (0 = zero impact)
    float widthPct    = 100.0f; // 0..200%
    float eqLowDb     = 0.0f;   // 30-120 Hz
    float eqLowMidDb  = 0.0f;   // 120-1.5 kHz
    float eqMidDb     = 0.0f;   // 1.5-7 kHz
    float eqHighDb    = 0.0f;   // > 7 kHz
    bool  trebleClean = false;
    bool  airBoost    = false;  // +1.0 dB smooth linear-phase air shelf
    float gluePct     = 35.0f;  // 0..100% Glue Comp
    int   autoTarget  = kAutoOff; // 0=Off, 1=-14, 2=-11, 3=-9 LUFS
    int   hqIndex     = 1;      // 0=1x, 1=2x(96k), 2=4x(192k), 3=8x(384k)
    float hpfHz       = 150.0f; // 0..240 Hz protecting Glue Comp & Width
    float ceilingDb   = 0.0f;   // -14.0..0.0 dB slow-release ceiling limiter
    int   satMode     = kSatOff;// 0=Off, 1=Tape, 2=Tube, 3=Xfmr, 4=Class-A
};

struct MeterSnapshot
{
    float inPeakDb = -96.0f, inLufsDb = -96.0f;
    float outPeakDb = -96.0f, outLufsDb = -96.0f;
    float outLufsShortDb = -96.0f, outLufsIntDb = -96.0f;
    float outRmsDb = -96.0f, outPlrDb = 0.0f;
};

// High-clarity smooth-knee gentle peak-shaving clipper strictly bounded at ceilLin
inline float processGentleClip(float x, float ceilLin)
{
    const float knee = 0.76f * ceilLin;
    const float head = std::fmax(1.0e-5f, ceilLin - knee);
    const float ax   = std::fabs(x);
    if (ax <= knee) return x;
    const float u = (ax - knee) / head;
    const float shaped = knee + head * fastTanh(u);
    const float clamped = shaped > ceilLin ? ceilLin : shaped;
    return x >= 0.0f ? clamped : -clamped;
}

class MPChainDSP
{
public:
    void prepare(double sampleRate)
    {
        sr_ = sampleRate > 8000.0 ? sampleRate : 44100.0;
        inGainSm_.setTime(12.0f, sr_); outGainSm_.setTime(12.0f, sr_);
        loudGainSm_.setTime(15.0f, sr_); loudDbSm_.setTime(15.0f, sr_);
        widthSm_.setTime(15.0f, sr_); glueSm_.setTime(20.0f, sr_);
        hpfBlendSm_.setTime(15.0f, sr_); ceilLinSm_.setTime(15.0f, sr_);

        linEq_.prepare(sr_);
        glueXover_.prepare(sr_, 150.0f); widthXover_.prepare(sr_, 150.0f);
        lastHpfHz_ = 150.0f;

        gluePkAtk_ = onePoleCoeff(7.0f, sr_); gluePkRel_ = onePoleCoeff(48.0f, sr_);
        glueRmsAtk_ = onePoleCoeff(24.0f, sr_); glueRmsRel_ = onePoleCoeff(130.0f, sr_);

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
        widthSm_.snap(1.0f); glueSm_.snap(0.35f); hpfBlendSm_.snap(1.0f); ceilLinSm_.snap(1.0f);
        linEq_.reset(); glueXover_.reset(); widthXover_.reset();
        gluePkEnv_ = glueRmsSq_ = 0.0f; glueGain_ = glueGainTgt_ = 1.0f; glueSubCount_ = 0;
        cleanDetBp_.reset(); cleanDynBell_.reset(); cleanAirShelf_.reset(); cleanPkEnv_ = cleanRmsSq_ = 0.0f;
        autoPreShelf_.reset(); autoRlbHp_.reset(); autoMs_ = 0.0f; autoGainLin_ = autoGainTgt_ = 1.0f;
        clipPreSub_.reset(); clipPreMid_.reset(); clipPostSub_.reset(); clipPostAir_.reset();
        limGain_ = 1.0f;
        dcX1L_ = dcY1L_ = dcX1R_ = dcY1R_ = 0.0f;
        tapeSlewL_ = tapeSlewR_ = xfmrFluxL_ = xfmrFluxR_ = 0.0f;
        osL_.reset(); osR_.reset(); inMeter_.reset(); outMeter_.reset();
        std::memset(specRing_, 0, sizeof(specRing_));
        specWritePos_.store(0u, std::memory_order_relaxed);
    }

    MeterSnapshot processBlock(float* chL, float* chR, int numFrames, const MPChainParams& p)
    {
        inGainSm_.setTarget(dbToGain(clampf(p.inGainDb, -12.0f, 12.0f)));
        outGainSm_.setTarget(dbToGain(clampf(p.outGainDb, -12.0f, 12.0f)));
        const float louderDb = clampf(p.louderDb, 0.0f, 24.0f);
        loudGainSm_.setTarget(dbToGain(louderDb));
        loudDbSm_.setTarget(louderDb);
        widthSm_.setTarget(clampf(p.widthPct * 0.01f, 0.0f, 2.0f));
        glueSm_.setTarget(clampf(p.gluePct * 0.01f, 0.0f, 1.0f));
        const float ceilDb = clampf(p.ceilingDb, -14.0f, 0.0f);
        ceilLinSm_.setTarget(dbToGain(ceilDb));

        // Target internal sample rates: 0=Native, 1=96k (2x@48k), 2=192k (4x@48k), 3=384k (8x@48k)
        int osFactor = 1;
        if (p.hqIndex > 0)
        {
            const double targetSr = (p.hqIndex == 1) ? 96000.0 : (p.hqIndex == 2 ? 192000.0 : 384000.0);
            while (osFactor < 8 && sr_ * (double)osFactor < targetSr * 0.90)
                osFactor <<= 1;
        }
        const double osSr = sr_ * (double)osFactor;
        // Slow-release transparent mastering limiter (~220 ms smooth release)
        const float limRelOs = onePoleCoeff(220.0f, osSr);
        const float dcCoefOs = 1.0f - onePoleCoeff(28.0f, osSr);
        const float tapeLpOs = onePoleCoeff(0.018f, osSr); // subtle magnetic tape high-end slew glue
        const float xfmrLpOs = onePoleCoeff(1.6f, osSr);   // ~100 Hz transformer core flux weight

        linEq_.setGains(clampf(p.eqLowDb, -6.0f, 6.0f),
                        clampf(p.eqLowMidDb, -6.0f, 6.0f),
                        clampf(p.eqMidDb, -6.0f, 6.0f),
                        clampf(p.eqHighDb, -6.0f, 6.0f),
                        p.airBoost);

        const float targetHpfHz = clampf(p.hpfHz, 0.0f, 240.0f);
        hpfBlendSm_.setTarget(clampf(targetHpfHz * 0.1f, 0.0f, 1.0f));
        const float effHz = std::fmax(10.0f, targetHpfHz);
        if (std::fabs(effHz - lastHpfHz_) > 0.25f)
        {
            lastHpfHz_ += 0.35f * (effHz - lastHpfHz_);
            if (std::fabs(effHz - lastHpfHz_) < 0.35f) lastHpfHz_ = effHz;
            glueXover_.setCutoff(sr_, lastHpfHz_); widthXover_.setCutoff(sr_, lastHpfHz_);
        }

        if (p.trebleClean)
        {
            const float hDet = 0.55f * cleanPkEnv_ + 0.45f * std::sqrt(cleanRmsSq_);
            const float excess = std::fmax(0.0f, gainToDb(hDet, -96.0f) - (-22.0f));
            const float dynCut = -clampf(excess * 0.22f, 0.0f, 2.2f);
            if (std::fabs(dynCut - lastCleanCutDb_) > 0.05f)
            {
                lastCleanCutDb_ = dynCut;
                cleanDynBell_.setCoeffs(BiquadCoeffs::peak(sr_, 4400.0f, 1.30f, dynCut));
            }
        }

        const float balAmt = clampf((louderDb - kClipOnsetDb) / (24.0f - kClipOnsetDb), 0.0f, 1.0f);
        if (std::fabs(balAmt - lastBalAmt_) > 0.015f) updateClipBalanceFilters(balAmt);

        if (p.autoTarget != kAutoOff)
        {
            const float targetLufs = (p.autoTarget == kAuto9) ? -9.0f : (p.autoTarget == kAuto11 ? -11.0f : -14.0f);
            const float curLufs = autoMs_ > 1.0e-10f ? (-0.691f + 10.0f * std::log10(autoMs_)) : -96.0f;
            if (curLufs > -55.0f) autoGainTgt_ = dbToGain(clampf(targetLufs - curLufs, -12.0f, 14.0f));
        }
        else autoGainTgt_ = 1.0f;

        const int satMode = clampf((float)p.satMode, 0.0f, 4.0f) > 0.5f ? p.satMode : kSatOff;

        for (int i = 0; i < numFrames; ++i)
        {
            float l = sanitize(chL[i]), r = sanitize(chR ? chR[i] : chL[i]);
            inMeter_.process(l, r);

            const float inG = inGainSm_.process();
            l *= inG; r *= inG;

            const float hpfBlend = hpfBlendSm_.process();
            const float glue = glueSm_.process();

            // 1. PRE-LOUDER: 1.5:1 Bus Glue Compressor (uncompressed below HPF)
            if (glue > 0.001f)
            {
                float loL = 0.0f, loR = 0.0f, hiL = l, hiR = r;
                if (hpfBlend > 0.0005f)
                {
                    float xLoL, xLoR, xHiL, xHiR; glueXover_.split(l, r, xLoL, xLoR, xHiL, xHiR);
                    loL = xLoL * hpfBlend; loR = xLoR * hpfBlend; hiL = lerpf(l, xHiL, hpfBlend); hiR = lerpf(r, xHiR, hpfBlend);
                }
                const float instPk = std::fmax(std::fabs(hiL), std::fabs(hiR));
                gluePkEnv_ = flushDenorm(gluePkEnv_ + (instPk > gluePkEnv_ ? gluePkAtk_ : gluePkRel_) * (instPk - gluePkEnv_));
                const float instSq = 0.5f * (hiL * hiL + hiR * hiR);
                glueRmsSq_ = flushDenorm(glueRmsSq_ + (instSq > glueRmsSq_ ? glueRmsAtk_ : glueRmsRel_) * (instSq - glueRmsSq_));
                if ((glueSubCount_++ & 7) == 0)
                {
                    const float detLvl = 0.72f * std::sqrt(glueRmsSq_) + 0.28f * gluePkEnv_;
                    const float over = gainToDb(detLvl, -96.0f) - (-22.0f * glue);
                    constexpr float kneeW = 8.0f, kSlope = 1.0f / 3.0f;
                    float grDb = 0.0f;
                    if (over > 0.5f * kneeW) grDb = -kSlope * over;
                    else if (over > -0.5f * kneeW) { const float x = over + 0.5f * kneeW; grDb = -kSlope * (x * x) / (2.0f * kneeW); }
                    glueGainTgt_ = dbToGain(grDb + 1.4f * glue * 0.35f);
                }
                glueGain_ = flushDenorm(glueGain_ + 0.25f * (glueGainTgt_ - glueGain_));
                l = loL + hiL * glueGain_; r = loR + hiR * glueGain_;
            }

            // 2. PRE-LOUDER: Always-On 4-Band Linear-Phase EQ + Smooth +1 dB Air Shelf
            linEq_.processSample(l, r);

            // 3. PRE-LOUDER: Dynamic Treble Clean
            if (p.trebleClean)
            {
                float dl = l, dr = r; cleanDetBp_.process(dl, dr);
                const float hPk = std::fmax(std::fabs(dl), std::fabs(dr));
                cleanPkEnv_ = flushDenorm(cleanPkEnv_ + (hPk > cleanPkEnv_ ? cleanPkAtk_ : cleanPkRel_) * (hPk - cleanPkEnv_));
                cleanRmsSq_ = flushDenorm(cleanRmsSq_ + cleanRmsCoef_ * (0.5f * (dl * dl + dr * dr) - cleanRmsSq_));
                cleanDynBell_.process(l, r); cleanAirShelf_.process(l, r);
            }

            // 4. PRE-LOUDER: Stereo Width Crossfader (protected below shared HPF)
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

            // 5. PRE-LOUDER: Gated K-Weighted Auto-Gain (-14 / -11 / -9 LUFS)
            {
                float kl = l, kr = r; autoPreShelf_.process(kl, kr); autoRlbHp_.process(kl, kr);
                autoMs_ = flushDenorm(autoMs_ + autoLufsCoef_ * ((kl * kl + kr * kr) - autoMs_));
                autoGainLin_ = flushDenorm(autoGainLin_ + autoGainCoef_ * (autoGainTgt_ - autoGainLin_));
                l *= autoGainLin_; r *= autoGainLin_;
            }

            // 6. LOUDER Stage (0..+24 dB) + Subtle Stereo-Bus Saturation + >+9 dB Gentle Clip + Slow-Release Ceiling Limiter
            const float loudGain  = loudGainSm_.process();
            const float curLoudDb = loudDbSm_.process();
            const float ceilLin   = ceilLinSm_.process();

            // True-zero transparency: when LOUDER == 0.0 dB and Ceiling == 0.0 dB, zero processing impact!
            const bool louderActive = (curLoudDb > 0.005f);
            const bool ceilingActive = (ceilLin < 0.999f) || louderActive;

            if (ceilingActive)
            {
                // ~1% wet saturation per +0.5 dB of LOUDER (= 2% per dB, capped at 48% wet at +24 dB)
                const float satWet = (louderActive && satMode != kSatOff) ? clampf(curLoudDb * 0.02f, 0.0f, 0.48f) : 0.0f;
                const float clipBlend = clampf((curLoudDb - kClipOnsetDb) / (24.0f - kClipOnsetDb), 0.0f, 1.0f);

                if (clipBlend > 0.001f) { clipPreSub_.process(l, r); clipPreMid_.process(l, r); }

                if (osFactor > 1 && (louderActive || satWet > 0.0005f))
                {
                    float uL[8], uR[8]; osL_.upsample(osFactor, l, uL); osR_.upsample(osFactor, r, uR);
                    for (int s = 0; s < osFactor; ++s)
                        processLouderSample(uL[s], uR[s], louderActive ? loudGain : 1.0f, satMode, satWet,
                                            clipBlend, ceilLin, limRelOs, dcCoefOs, tapeLpOs, xfmrLpOs);
                    l = osL_.downsample(osFactor, uL); r = osR_.downsample(osFactor, uR);
                }
                else
                {
                    processLouderSample(l, r, louderActive ? loudGain : 1.0f, satMode, satWet,
                                        clipBlend, ceilLin, limRelOs, dcCoefOs, tapeLpOs, xfmrLpOs);
                }

                if (clipBlend > 0.001f)
                {
                    clipPostSub_.process(l, r); clipPostAir_.process(l, r);
                    l = processGentleClip(l, ceilLin); r = processGentleClip(r, ceilLin);
                }
                else
                {
                    l = clampf(l, -ceilLin, ceilLin); r = clampf(r, -ceilLin, ceilLin);
                }
            }

            // 7. Output Trim + Output Multimeter + Spectrum Ring Tap
            const float outG = outGainSm_.process();
            const float finL = sanitize(l * outG), finR = sanitize(r * outG);
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

    inline float shapeChannel(float x, int mode, float& tapeSlew, float& xfmrFlux, float tapeLp, float xfmrLp)
    {
        // Level-matched stereo-bus saturation models (clean, cohesive presence & power)
        switch (mode)
        {
            case kSatTape:
            {
                // Magnetic Tape: smooth odd-dominant hysteresis + gentle high-frequency slew polish
                tapeSlew = flushDenorm(tapeSlew + tapeLp * (x - tapeSlew));
                const float driven = lerpf(x, tapeSlew, 0.14f) * 1.45f;
                return fastTanh(driven) * 0.78f + 0.10f * fastTanh(driven * 0.55f);
            }
            case kSatTube:
            {
                // Triode Tube: warm asymmetric 2nd + 3rd harmonic richness & open upper-mid glow
                const float d = x * 1.35f;
                const float asym = d + 0.16f * d * d - 0.04f * d * d * d;
                return fastTanh(asym) * 0.80f;
            }
            case kSatXfmr:
            {
                // Mastering Output Transformer: iron-core low-end flux weight + punchy forward presence
                xfmrFlux = flushDenorm(xfmrFlux + xfmrLp * (x - xfmrFlux));
                const float core = x * 1.38f + 0.28f * fastTanh(xfmrFlux * 1.9f);
                return (2.0f / kPi) * std::atan(core * 1.15f) * 0.92f;
            }
            case kSatClassA:
            default:
            {
                // Class-A Solid-State Console: crisp, tight, transparent harmonic presence
                const float d = x * 1.28f;
                return (d / std::sqrt(1.0f + 0.85f * d * d)) * 0.88f;
            }
        }
    }

    inline void processLouderSample(float& l, float& r, float loudGain, int satMode, float satWet,
                                    float clipBlend, float ceilLin, float limRel, float dcCoef,
                                    float tapeLp, float xfmrLp)
    {
        l *= loudGain; r *= loudGain;

        if (satWet > 0.0005f && satMode != kSatOff)
        {
            const float wL = shapeChannel(l, satMode, tapeSlewL_, xfmrFluxL_, tapeLp, xfmrLp);
            const float wR = shapeChannel(r, satMode, tapeSlewR_, xfmrFluxR_, tapeLp, xfmrLp);
            l = lerpf(l, wL, satWet);
            r = lerpf(r, wR, satWet);
            // Gentle DC blocker (important for asymmetric Tube harmonic bias)
            const float dL = flushDenorm(l - dcX1L_ + dcCoef * dcY1L_); dcX1L_ = l; dcY1L_ = dL; l = dL;
            const float dR = flushDenorm(r - dcX1R_ + dcCoef * dcY1R_); dcX1R_ = r; dcY1R_ = dR; r = dR;
        }

        // Slow-release transparent mastering ceiling limiter
        const float pk = std::fmax(std::fabs(l), std::fabs(r));
        const float needed = pk > ceilLin ? (ceilLin / pk) : 1.0f;
        limGain_ = needed < limGain_ ? needed : flushDenorm(limGain_ + limRel * (needed - limGain_));

        if (clipBlend <= 0.0005f)
        {
            l = clampf(l * limGain_, -ceilLin, ceilLin);
            r = clampf(r * limGain_, -ceilLin, ceilLin);
        }
        else
        {
            const float g = lerpf(limGain_, 1.0f, clipBlend * 0.85f);
            l = processGentleClip(l * g, ceilLin);
            r = processGentleClip(r * g, ceilLin);
        }
    }

    double sr_ = 44100.0;
    Smoother inGainSm_, outGainSm_, loudGainSm_, loudDbSm_, widthSm_, glueSm_, hpfBlendSm_, ceilLinSm_;
    LinearPhaseEQ4 linEq_;
    LR4Crossover glueXover_, widthXover_;
    float lastHpfHz_ = 150.0f;
    float gluePkEnv_ = 0.0f, glueRmsSq_ = 0.0f, glueGain_ = 1.0f, glueGainTgt_ = 1.0f;
    int glueSubCount_ = 0;
    float gluePkAtk_ = 0.01f, gluePkRel_ = 0.001f, glueRmsAtk_ = 0.005f, glueRmsRel_ = 0.001f;
    StereoBiquad cleanDetBp_, cleanDynBell_, cleanAirShelf_;
    float cleanPkEnv_ = 0.0f, cleanRmsSq_ = 0.0f, cleanPkAtk_ = 0.01f, cleanPkRel_ = 0.001f, cleanRmsCoef_ = 0.001f, lastCleanCutDb_ = 999.0f;
    StereoBiquad autoPreShelf_, autoRlbHp_;
    float autoMs_ = 0.0f, autoGainLin_ = 1.0f, autoGainTgt_ = 1.0f, autoLufsCoef_ = 0.001f, autoGainCoef_ = 0.001f;
    StereoBiquad clipPreSub_, clipPreMid_, clipPostSub_, clipPostAir_;
    float lastBalAmt_ = -1.0f, limGain_ = 1.0f;
    float dcX1L_ = 0.0f, dcY1L_ = 0.0f, dcX1R_ = 0.0f, dcY1R_ = 0.0f;
    float tapeSlewL_ = 0.0f, tapeSlewR_ = 0.0f, xfmrFluxL_ = 0.0f, xfmrFluxR_ = 0.0f;
    Oversampler8x osL_, osR_;
    LoudnessMeter inMeter_, outMeter_;
    static constexpr uint32_t kSpecRingSize = 4096;
    float specRing_[kSpecRingSize] = {};
    std::atomic<uint32_t> specWritePos_{ 0 };
};

} // namespace mixchain
