#include "audio.h"
#include <SDL.h>
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <vector>

namespace {
    const int RATE = 44100;

    // Generated crowd/applause are made from random noise and sound like static. They are OFF by default.
    // Put real recordings in assets/ambience.wav and assets/applause.wav instead (see the notes in the README).
    const bool USE_SYNTHETIC = false;

    const int FADE_IN  = 220;     // samples (5 ms): stops a click when a sound starts
    const int FADE_OUT = 1100;    // samples (25 ms): stops a click when a sound is cut off

    struct Sound { std::vector<int16_t> data; };                       // mono, 44100 Hz, 16-bit
    struct Voice { int sound; size_t pos; float volume; bool loop; bool active; int fadeIn; int fadeOut; };   // fadeOut -1 = not stopping

    SDL_AudioDeviceID dev = 0;
    std::vector<Sound> sounds;
    std::vector<Voice> voices;
    bool muted = false;
    int applauseId = -1, ambienceId = -1, speechId = -1;

    // ---------- tiny deterministic random numbers ----------
    uint32_t rngState = 987654321u;
    float rnd01() { rngState = rngState * 1664525u + 1013904223u; return float(rngState >> 8) / 16777216.0f; }
    float rndSigned() { return rnd01() * 2.0f - 1.0f; }

    int addSound(const std::vector<float>& f, float peak) {
        float mx = 1e-6f;
        for (float v : f) mx = std::max(mx, std::fabs(v));
        Sound s;
        s.data.resize(f.size());
        for (size_t i = 0; i < f.size(); i++) s.data[i] = (int16_t)(f[i] / mx * peak * 32767.0f);
        sounds.push_back(std::move(s));
        return (int)sounds.size() - 1;
    }

    // ---------- generated applause: many short claps + a soft crowd bed ----------
    int makeApplause() {
        const float seconds = 6.0f;
        const int n = (int)(seconds * RATE);
        std::vector<float> buf(n, 0.0f);

        auto envelope = [&](float t) {                                  // swell up, hold, die away
            float up = std::min(1.0f, t / 0.6f);
            float down = std::min(1.0f, (seconds - t) / 2.2f);
            return up * down;
        };

        // claps: short bursts of noise that fade out quickly
        for (int i = 0; i < n; i++) {
            float t = float(i) / RATE;
            float density = 170.0f * envelope(t);                       // claps per second
            if (rnd01() < density / RATE) {
                int len = 300 + int(rnd01() * 700);
                float amp = 0.25f + 0.75f * rnd01();
                float bright = 0.3f + 0.7f * rnd01();                   // 0 = dull thud, 1 = sharp clap
                float prev = 0;
                for (int k = 0; k < len && i + k < n; k++) {
                    float decay = std::exp(-6.0f * k / len);
                    float w = rndSigned();
                    prev = prev + bright * (w - prev);                  // simple low-pass: controls brightness
                    buf[i + k] += prev * decay * amp;
                }
            }
        }
        // soft crowd bed underneath
        float lp = 0;
        for (int i = 0; i < n; i++) {
            lp += 0.05f * (rndSigned() - lp);
            buf[i] += lp * 1.2f * envelope(float(i) / RATE);
        }
        return addSound(buf, 0.7f);
    }

    // ---------- generated crowd murmur that loops without a click ----------
    int makeAmbience() {
        const int L = RATE * 8, X = RATE / 2;                           // 8 s loop, 0.5 s crossfade
        std::vector<float> a(L + X);
        float lp1 = 0, lp2 = 0;
        for (int i = 0; i < L + X; i++) {
            lp1 += 0.02f * (rndSigned() - lp1);
            lp2 += 0.004f * (rndSigned() - lp2);
            float swell = 0.75f + 0.25f * std::sin(6.2831853f * i / (RATE * 4.0f));
            a[i] = (lp1 * 1.0f + lp2 * 3.0f) * swell;
        }
        std::vector<float> out(L);
        for (int i = 0; i < L; i++) {
            if (i < X) { float w = float(i) / X; out[i] = a[i] * w + a[L + i] * (1.0f - w); }
            else out[i] = a[i];
        }
        return addSound(out, 0.5f);
    }

    // ---------- the mixer: runs on the audio thread ----------
    void callback(void*, Uint8* stream, int bytes) {
        static int32_t acc[8192];                                       // static: nothing is allocated on the audio thread
        int16_t* out = (int16_t*)stream;
        int frames = std::min(bytes / (int)sizeof(int16_t), 8192);
        for (int i = 0; i < frames; i++) acc[i] = 0;
        if (!muted) {
            for (Voice& v : voices) {
                if (!v.active) continue;
                const std::vector<int16_t>& d = sounds[v.sound].data;
                for (int i = 0; i < frames; i++) {
                    if (v.pos >= d.size()) {
                        if (v.loop && !d.empty()) v.pos = 0; else { v.active = false; break; }
                    }
                    float g = v.volume;
                    if (v.fadeIn < FADE_IN) g *= float(v.fadeIn++) / FADE_IN;
                    if (v.fadeOut >= 0) {
                        g *= float(v.fadeOut) / FADE_OUT;
                        if (v.fadeOut-- == 0) { v.active = false; break; }
                    }
                    acc[i] += (int32_t)(d[v.pos++] * g);
                }
            }
        }
        for (int i = 0; i < frames; i++)
            out[i] = (int16_t)std::max(-32768, std::min(32767, acc[i]));   // clip instead of wrapping
        for (int i = frames; i < bytes / (int)sizeof(int16_t); i++) out[i] = 0;
    }
}

namespace audio {

bool init() {
    if (SDL_InitSubSystem(SDL_INIT_AUDIO) != 0) { std::printf("Audio unavailable: %s\n", SDL_GetError()); return false; }
    SDL_AudioSpec want{}, have{};
    want.freq = RATE; want.format = AUDIO_S16SYS; want.channels = 1; want.samples = 2048; want.callback = callback;
    dev = SDL_OpenAudioDevice(nullptr, 0, &want, &have, 0);             // 0 = SDL converts to exactly what we asked for
    if (!dev) { std::printf("No audio device: %s\n", SDL_GetError()); return false; }

    if (USE_SYNTHETIC) { applauseId = makeApplause(); ambienceId = makeAmbience(); }
    if (ambienceId < 0) ambienceId = load("assets/ambience.wav");       // optional real recordings: skipped if the file isn't there
    if (applauseId < 0) applauseId = load("assets/applause.wav");
    speechId = load("assets/speech.wav");
    SDL_PauseAudioDevice(dev, 0);
    if (ambienceId >= 0) play(ambienceId, 0.30f, true);                 // quiet crowd murmur from the start
    return true;
}

void shutdown() {
    if (dev) { SDL_CloseAudioDevice(dev); dev = 0; }
    SDL_QuitSubSystem(SDL_INIT_AUDIO);
}

int load(const char* path) {
    if (!dev) return -1;                                                // no audio device (e.g. --at mode): nothing to load into
    SDL_AudioSpec spec; Uint8* buf = nullptr; Uint32 len = 0;
    if (!SDL_LoadWAV(path, &spec, &buf, &len)) return -1;

    SDL_AudioCVT cvt;
    int r = SDL_BuildAudioCVT(&cvt, spec.format, spec.channels, spec.freq, AUDIO_S16SYS, 1, RATE);
    if (r < 0) { SDL_FreeWAV(buf); return -1; }

    Sound s;
    if (r == 0) {                                                       // already mono / 44100 / 16-bit
        s.data.resize(len / 2);
        std::memcpy(s.data.data(), buf, s.data.size() * 2);
    } else {
        cvt.len = (int)len;
        cvt.buf = (Uint8*)SDL_malloc((size_t)len * cvt.len_mult);
        std::memcpy(cvt.buf, buf, len);
        SDL_ConvertAudio(&cvt);
        s.data.resize(cvt.len_cvt / 2);
        std::memcpy(s.data.data(), cvt.buf, s.data.size() * 2);
        SDL_free(cvt.buf);
    }
    SDL_FreeWAV(buf);
    SDL_LockAudioDevice(dev);
    sounds.push_back(std::move(s));
    int id = (int)sounds.size() - 1;
    SDL_UnlockAudioDevice(dev);
    return id;
}

void play(int id, float volume, bool loop) {
    if (!dev || id < 0 || id >= (int)sounds.size()) return;
    SDL_LockAudioDevice(dev);
    for (Voice& v : voices)                                             // reuse a finished slot
        if (!v.active) { v = {id, 0, volume, loop, true, 0, -1}; SDL_UnlockAudioDevice(dev); return; }
    voices.push_back({id, 0, volume, loop, true, 0, -1});
    SDL_UnlockAudioDevice(dev);
}

void stopAll(bool keepAmbience) {
    if (!dev) return;
    SDL_LockAudioDevice(dev);
    for (Voice& v : voices)
        if (v.active && v.fadeOut < 0 && !(keepAmbience && v.loop)) v.fadeOut = FADE_OUT;   // fade out, no click (looping background sounds are kept when keepAmbience is set)
    SDL_UnlockAudioDevice(dev);
}

float length(int id) {
    if (id < 0 || id >= (int)sounds.size()) return 0.0f;
    return float(sounds[id].data.size()) / float(RATE);
}

void setMuted(bool m) { muted = m; }
int  applause()       { return applauseId; }

void update() {
    if (!dev) return;
    static bool pa = false, pv = false, pm = false;
    const Uint8* k = SDL_GetKeyboardState(nullptr);
    bool a = k[SDL_SCANCODE_A] != 0, v = k[SDL_SCANCODE_V] != 0, m = k[SDL_SCANCODE_M] != 0;
    if (a && !pa) play(applauseId, 0.9f);
    if (v && !pv) play(speechId, 1.0f);
    if (m && !pm) setMuted(!muted);
    pa = a; pv = v; pm = m;
}

}