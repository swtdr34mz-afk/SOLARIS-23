/* SPDX-License-Identifier: MPL-2.0
 * Copyright (C) 2026 swtdr34mz (SOLARIS-23) */
/* SOLARIS-23's engine checked without JUCE or the MPC: every program at several rates (sound, no NaN, no overs),
 * any buffer size the same bit for bit, every parameter swept while it plays, every effect, the CPU.
 *
 *   engine_tests                   all checks
 *   engine_tests bench             the CPU, each program
 *   engine_tests render <program> <seconds> <out.wav> [rate]   to listen (a chord of keys held from 1 s) */
#include "solaris.h"
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

using namespace sol;

static int failures = 0;
#define CHECK(cond, ...)                                                                                               \
    do {                                                                                                               \
        if (!(cond)) {                                                                                                 \
            failures++;                                                                                                \
            std::printf("FAIL %s:%d: ", __FILE__, __LINE__);                                                           \
            std::printf(__VA_ARGS__);                                                                                  \
            std::printf("\n");                                                                                         \
        }                                                                                                              \
    } while (0)

static void load(Engine &e, int prog)
{
    std::vector<float> v(NPARAMS);
    presetValues(prog, v.data());
    for (int i = 0; i < NPARAMS; i++)
        e.set(i, v[i]);
}

struct Stats {
    double peak = 0, rms = 0;
    bool finite = true;
};

/* a program played: keys down at 1 s (C3, then G3 at 2 s), up at 4 s; block: the host's buffer size (0: varied) */
static Stats play(int prog, double rate, double secs, int block, std::vector<float> *keep = nullptr,
                  void (*each)(Engine &, int) = nullptr)
{
    Engine e;
    e.prepare(rate);
    load(e, prog);
    e.tempo(110.0);
    const int n = (int)(secs * rate);
    std::vector<float> L(4096), R(4096);
    Stats s;
    double sum = 0;
    int done = 0, k = 0;
    const int on1 = (int)rate, on2 = (int)(2 * rate), off = (int)(4 * rate);
    static const int VAR[11] = {1, 3, 16, 17, 64, 128, 333, 512, 1024, 2048, 31};
    while (done < n) {
        int b = block > 0 ? block : VAR[k++ % 11];
        if (b > n - done)
            b = n - done;
        /* events land inside the block where they fall */
        int end = done + b, at = done;
        auto ev = [&](int t, auto f) {
            if (t >= at && t < end) {
                if (t > at) {
                    e.render(L.data(), R.data(), t - at);
                    for (int i = 0; i < t - at; i++) {
                        float l = L[(size_t)i], r = R[(size_t)i];
                        if (!std::isfinite(l) || !std::isfinite(r))
                            s.finite = false;
                        s.peak = std::fmax(s.peak, std::fmax(std::fabs(l), std::fabs(r)));
                        sum += (double)l * l + (double)r * r;
                        if (keep) {
                            keep->push_back(l);
                            keep->push_back(r);
                        }
                    }
                    at = t;
                }
                f();
            }
        };
        ev(on1, [&] { e.noteOn(48, 0.8f); });
        ev(on2, [&] { e.noteOn(55, 0.6f); });
        ev(off, [&] {
            e.noteOff(48);
            e.noteOff(55);
        });
        if (end > at) {
            e.render(L.data(), R.data(), end - at);
            for (int i = 0; i < end - at; i++) {
                float l = L[(size_t)i], r = R[(size_t)i];
                if (!std::isfinite(l) || !std::isfinite(r))
                    s.finite = false;
                s.peak = std::fmax(s.peak, std::fmax(std::fabs(l), std::fabs(r)));
                sum += (double)l * l + (double)r * r;
                if (keep) {
                    keep->push_back(l);
                    keep->push_back(r);
                }
            }
        }
        if (each)
            each(e, done);
        done = end;
    }
    s.rms = std::sqrt(sum / (2.0 * n));
    return s;
}

static void writeWav(const char *path, const std::vector<float> &lr, int rate)
{
    FILE *f = std::fopen(path, "wb");
    if (!f)
        return;
    uint32_t frames = (uint32_t)(lr.size() / 2), bytes = frames * 4;
    auto u32 = [&](uint32_t v) { std::fwrite(&v, 4, 1, f); };
    auto u16 = [&](uint16_t v) { std::fwrite(&v, 2, 1, f); };
    std::fwrite("RIFF", 1, 4, f);
    u32(36 + bytes);
    std::fwrite("WAVEfmt ", 1, 8, f);
    u32(16);
    u16(1);
    u16(2);
    u32((uint32_t)rate);
    u32((uint32_t)rate * 4);
    u16(4);
    u16(16);
    std::fwrite("data", 1, 4, f);
    u32(bytes);
    for (float x : lr) {
        int v = (int)std::lround(std::fmax(-1.0f, std::fmin(1.0f, x)) * 32767.0f);
        u16((uint16_t)(int16_t)v);
    }
    std::fclose(f);
}

int main(int argc, char **argv)
{
    if (argc >= 5 && !std::strcmp(argv[1], "render")) {
        int prog = std::atoi(argv[2]);
        double secs = std::atof(argv[3]), rate = argc > 5 ? std::atof(argv[5]) : 44100.0;
        std::vector<float> lr;
        Stats s = play(prog, rate, secs, 256, &lr);
        writeWav(argv[4], lr, (int)rate);
        std::printf("%s: peak %.3f, rms %.4f\n", presetName(prog), s.peak, s.rms);
        return 0;
    }
    if (argc >= 2 && !std::strcmp(argv[1], "bench")) {
        for (int p = 0; p < numPresets(); p++) {
            auto t0 = std::chrono::steady_clock::now();
            play(p, 44100.0, 10.0, 256);
            double ms = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - t0).count();
            std::printf("%-13s %5.1f%% of one core (10 s at 44.1 kHz in %.0f ms)\n", presetName(p), ms / 100.0, ms);
        }
        return 0;
    }

    /* the parameters: unique ids, defaults in range, texts */
    for (int i = 0; i < NPARAMS; i++) {
        CHECK(paramIndex(PARAMS[i].id) == i, "%s: id not unique", PARAMS[i].id);
        CHECK(PARAMS[i].def >= 0.0f && PARAMS[i].def <= paramMax(i), "%s: default %g", PARAMS[i].id, PARAMS[i].def);
        CHECK(std::strlen(PARAMS[i].shortName) <= 12, "%s: short name too long", PARAMS[i].id);
        char t[64];
        for (float v : {0.0f, 0.5f, paramMax(i)}) {
            paramText(i, v, t, sizeof t);
            CHECK(t[0] != 0, "%s: no text", PARAMS[i].id);
        }
    }
    CHECK(NPARAMS == 200, "the parameters' count changed: %d (append only)", NPARAMS);

    /* every program at three rates: it sounds, nothing over 1, nothing not a number */
    for (int p = 0; p < numPresets(); p++)
        for (double rate : {44100.0, 48000.0, 96000.0}) {
            Stats s = play(p, rate, 6.0, 256);
            CHECK(s.finite, "%s @ %.0f: not finite", presetName(p), rate);
            CHECK(s.peak <= 1.0, "%s @ %.0f: peak %.3f", presetName(p), rate, s.peak);
            CHECK(s.rms > 0.004, "%s @ %.0f: silent (rms %.5f)", presetName(p), rate, s.rms);
            if (rate == 44100.0)
                std::printf("  %-13s peak %.2f  rms %.3f\n", presetName(p), s.peak, s.rms);
        }

    /* any buffer size: the same samples */
    for (int p : {1, 7, 14}) {
        std::vector<float> a, b, c;
        play(p, 48000.0, 5.0, 512, &a);
        play(p, 48000.0, 5.0, 0, &b);
        play(p, 48000.0, 5.0, 1, &c);
        CHECK(a.size() == b.size() && a == b, "%s: varied buffers differ", presetName(p));
        CHECK(a.size() == c.size() && a == c, "%s: 1-sample buffers differ", presetName(p));
    }

    /* every effect program under the drones, X Y Z at their ends */
    for (int fx = 0; fx < NFX; fx++)
        for (float xyz : {0.0f, 1.0f}) {
            static int sfx;
            static float sxyz;
            sfx = fx;
            sxyz = xyz;
            Stats s = play(1, 48000.0, 4.0, 128, nullptr, [](Engine &e, int) {
                e.set(FXL, (float)sfx);
                e.set(FXR, (float)sfx);
                e.set(FX_X, sxyz);
                e.set(FX_Y, sxyz);
                e.set(FX_Z, sxyz);
                e.set(BLEND, 1.0f);
            });
            CHECK(s.finite && s.peak <= 1.0, "%s xyz %.0f: peak %.3f finite %d", fxName(fx), xyz, s.peak, (int)s.finite);
            CHECK(s.rms > 0.001 || (fx == 12 && xyz == 0.0f) || (fx == 14 && xyz == 1.0f), "%s xyz %.0f: silent (rms %.5f)", fxName(fx), xyz, s.rms);
        }

    /* every parameter swept from end to end while a full program plays */
    for (int i = 0; i < NPARAMS; i++) {
        static int si;
        si = i;
        Stats s = play(13, 44100.0, 2.5, 256, nullptr, [](Engine &e, int at) {
            float ph = (float)(at % 44100) / 44100.0f;
            e.set(si, ph * paramMax(si));
        });
        CHECK(s.finite && s.peak <= 1.0, "%s swept: peak %.3f finite %d", PARAMS[i].id, s.peak, (int)s.finite);
    }

    /* everything on at once, at its worst: all drones, FM, full resonance, the distortion, a black hole */
    {
        Stats s = play(0, 44100.0, 5.0, 64, nullptr, [](Engine &e, int) {
            for (int g : {D1_GATE, D2_GATE, D3_GATE, D4_GATE, D5_GATE, D6_GATE})
                e.set(g, 1);
            for (int v : {D1_VOLT, D2_VOLT, D4_VOLT, D5_VOLT})
                e.set(v, 1.0f);
            for (int v = D1_VOL; v <= D6_VOL; v++)
                e.set(v, 1.0f);
            e.set(FL_RES, 1.0f);
            e.set(FR_RES, 1.0f);
            e.set(DIST, 1.0f);
            e.set(GAIN, 1.0f);
            e.set(FXL, 15);
            e.set(FXR, 0);
            e.set(FX_X, 1);
            e.set(FX_Y, 1);
            e.set(BLEND, 1);
            e.set(VOLUME, 1);
        });
        CHECK(s.finite && s.peak <= 1.0, "all at once: peak %.3f", s.peak);
    }

    /* panic: silent at once */
    {
        Engine e;
        e.prepare(44100.0);
        load(e, 2);
        std::vector<float> L(44100), R(44100);
        e.render(L.data(), R.data(), 44100 * 1);
        e.panic();
        e.set(D1_HOLD, 0);
        e.set(D2_HOLD, 0);
        e.render(L.data(), R.data(), 64);
        e.render(L.data(), R.data(), 4410);
        float pk = 0;
        for (int i = 2205; i < 4410; i++)
            pk = std::fmax(pk, std::fabs(L[(size_t)i]));
        CHECK(pk < 0.01f, "panic: still %.4f after 50 ms", pk);
    }

    std::printf(failures ? "%d FAILED\n" : "all passed\n", failures);
    return failures ? 1 : 0;
}
