#pragma once

// Klangbild Stimme – Bausteine
//  Trittschallfilter : Hochpass 12 dB/Okt gegen Rumpeln, Ständergeräusche, Atemstöße
//  De-Esser          : Split-Band – nur das Zischband oberhalb der Frequenz wird abgesenkt,
//                      und nur wenn es im Verhältnis zur Stimme zu laut ist (lautstärkeunabhängig)
//  Kompressor        : weicher 3:1-Kompressor mit langsamem Programmverhalten
//  Klang             : Wärme (250 Hz), Präsenz (4,5 kHz), Luft (12 kHz)

#include <cmath>
#include <algorithm>
#include "EQDsp.h"

namespace stimme
{
class DeEsser
{
public:
    void prepare (double sampleRate)
    {
        fs = sampleRate;
        aFastUp   = coef (0.0005);
        aFastDown = coef (0.040);
        aFull     = coef (0.020);
        aGrDown   = coef (0.030);
        sib = full = gr = 0.0f;
        for (auto* arr : { &lp1, &lp2, &hp1, &hp2, &scState })
            for (auto& s : *arr) s.reset();
    }

    void setFrequency (double f)
    {
        // Linkwitz-Riley 4. Ordnung: Tief- und Hochband addieren sich wieder lückenlos
        lpC = eq::makeLowPass  (fs, f, 0.70710678);
        hpC = eq::makeHighPass (fs, f, 0.70710678);
        sc  = eq::makeHighPass (fs, f * 0.85, 0.9);
    }

    // amount 0..10; gibt das Ergebnis für einen Kanal zurück, Regelung ist gekoppelt (computeGain einmal je Sample)
    float computeGain (float mono, float amount) noexcept
    {
        const float s = scState[0].process (sc, mono);
        const float sAbs = std::abs (s), fAbs = std::abs (mono);
        sib  = sAbs + (sib - sAbs) * (sAbs > sib ? aFastUp : aFastDown);
        full = fAbs + (full - fAbs) * (fAbs > full ? aFastUp : aFastDown);   // gleiche Ballistik wie das Zischband
        if (amount < 0.01f)
        {
            gr = 0.0f;
            return 1.0f;
        }
        const float sibDb  = 20.0f * std::log10 (sib + 1.0e-7f);
        const float fullDb = 20.0f * std::log10 (full + 1.0e-7f);
        const float offset = 2.0f - 1.7f * amount;                     // je mehr Stärke, desto früher greift er
        const float excess = sibDb - (fullDb + offset);
        const float target = std::clamp (excess * 0.85f, 0.0f, 14.0f);
        gr = target + (gr - target) * (target > gr ? 0.3f : aGrDown);
        return std::pow (10.0f, -gr / 20.0f);
    }

    // Nur das Zischband wird abgesenkt; listen = nur das Zischband hören
    inline float apply (int ch, float x, float gain, bool listen) noexcept
    {
        const auto c = (size_t) ch;
        const float lo = lp2[c].process (lpC, lp1[c].process (lpC, x));
        const float hi = hp2[c].process (hpC, hp1[c].process (hpC, x));
        return listen ? hi * gain : lo + hi * gain;
    }

    float getReductionDb() const noexcept { return gr; }

private:
    float coef (double seconds) const { return (float) std::exp (-1.0 / (seconds * fs)); }
    double fs = 48000.0;
    eq::Coeffs lpC, hpC, sc;
    std::array<eq::BiquadState, 2> lp1, lp2, hp1, hp2, scState;
    float aFastUp = 0, aFastDown = 0, aFull = 0, aGrDown = 0, sib = 0, full = 0, gr = 0;
};

class Compressor
{
public:
    void prepare (double sampleRate)
    {
        fs = sampleRate;
        aAtt = coef (0.006);
        aRel = coef (0.120);
        aSlow = coef (0.8);
        aDet = coef (0.010);
        det = env = slow = 0.0;
    }
    void setAmount (float amount) { threshold = -6.0 - 3.2 * amount; }   // -6 .. -38 dBFS
    double process (float sidechain) noexcept
    {
        const double s = (double) sidechain * (double) sidechain;
        det = s + (det - s) * aDet;
        const double lvl = 10.0 * std::log10 (det + 1.0e-12) + 3.0;
        constexpr double ratio = 3.0, knee = 10.0;
        const double over = lvl - threshold;
        double tgt = 0.0;
        if (over >= knee * 0.5)       tgt = over * (1.0 - 1.0 / ratio);
        else if (over > -knee * 0.5) { const double k = over + knee * 0.5; tgt = (1.0 - 1.0 / ratio) * k * k / (2.0 * knee); }
        env = tgt + (env - tgt) * (tgt > env ? aAtt : aRel);
        slow = env + (slow - env) * (env > slow ? aAtt : aSlow);
        return std::max (env, 0.6 * slow);
    }
private:
    double coef (double seconds) const { return std::exp (-1.0 / (seconds * fs)); }
    double fs = 48000.0, aAtt = 0, aRel = 0, aSlow = 0, aDet = 0;
    double det = 0, env = 0, slow = 0, threshold = -15.0;
};
} // namespace stimme
