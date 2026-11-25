#include "locations.h"
#include "render.h"
#include "animation.h"
#include "font.h"
#include <algorithm>
#include <cmath>
#include <vector>

typedef std::vector<Vec2> Poly;

namespace {

const float PI = 3.14159265f;

float hash(int n) {
    unsigned x = (unsigned)n * 2654435761u;
    x ^= x >> 15; x *= 2246822519u; x ^= x >> 13;
    return float(x & 0xFFFF) / 65535.0f;
}
float zoomOf(const Mat3& M) { return std::hypot(M.m[0], M.m[3]); }
Color mix(Color a, Color b, float t) {
    return { uint8_t(a.r + (b.r - a.r) * t), uint8_t(a.g + (b.g - a.g) * t), uint8_t(a.b + (b.b - a.b) * t) };
}

// ---------- shape helpers ----------
Poly rect(float x, float y, float w, float h) { return {{x, y}, {x + w, y}, {x + w, y + h}, {x, y + h}}; }
Poly ellipse(float cx, float cy, float rx, float ry, int n = 28) {
    Poly p;
    for (int i = 0; i < n; i++) { float a = 2 * PI * i / n; p.push_back({cx + std::cos(a) * rx, cy + std::sin(a) * ry}); }
    return p;
}
void fill(const Mat3& M, const Poly& p, Color c)                    { render::fillPolygon(transformPoly(M, p), c); }
void grad(const Mat3& M, const Poly& p, Color a, Color b, bool h = false) { render::fillPolygonGradient(transformPoly(M, p), a, b, h); }
void shade(const Mat3& M, const Poly& p, Color c, float a)           { render::fillPolygonAlpha(transformPoly(M, p), c, a); }
void line(const Mat3& M, Vec2 a, Vec2 b, Color c)                    { render::drawLine(M * a, M * b, c); }
void disc(const Mat3& M, Vec2 c, float r, Color col)                 { render::fillCircle(M * c, r * zoomOf(M), col); }
void glow(const Mat3& M, Vec2 c, float r, Color col, float a)        { shade(M, ellipse(c.x, c.y, r, r, 16), col, a); }

// Crescent moon: the part of a disc (radius R) that a slightly smaller disc, shifted to the right, does not cover.
Poly crescent(Vec2 c, float R, float tilt) {
    const float R2 = 0.95f * R, d = 0.30f * R;                       // the circle that takes the bite
    float xi = (R * R - R2 * R2 + d * d) / (2 * d);                  // where the two outlines cross
    float yi = std::sqrt(std::max(0.0f, R * R - xi * xi));
    float a0 = std::atan2(yi, xi), b0 = std::atan2(yi, xi - d);
    Poly raw;
    const int n = 20;
    for (int i = 0; i <= n; i++) { float a = a0 + (2 * PI - 2 * a0) * i / n; raw.push_back({R * std::cos(a), R * std::sin(a)}); }                       // outer edge, round the left
    for (int i = 0; i <= n; i++) { float b = (2 * PI - b0) - (2 * PI - 2 * b0) * i / n; raw.push_back({d + R2 * std::cos(b), R2 * std::sin(b)}); }     // inner edge, back along the bite
    float cs = std::cos(tilt), sn = std::sin(tilt);
    for (Vec2& v : raw) v = {c.x + v.x * cs - v.y * sn, c.y + v.x * sn + v.y * cs};
    return raw;
}

// ---------- sky / ground palettes ----------
struct Pal { Color top, bot, gTop, gBot, tint; float tintA; bool night; Color sun; float sx, sy, sr; };
Pal palette(int sky) {
    switch (sky) {
        case 0:  return {{60, 90, 150},  {250, 190, 130}, {95, 105, 75},  {55, 70, 50},  {255, 170, 90}, 0.06f, false, {255, 205, 110}, 560, 372, 26};
        case 1:  return {{70, 130, 200}, {200, 225, 245}, {100, 130, 80}, {55, 85, 50},  {0, 0, 0},      0.00f, false, {255, 224, 90}, 150, 110, 22};
        case 2:  return {{35, 105, 200}, {225, 238, 250}, {135, 140, 85}, {95, 105, 60}, {255, 255, 235}, 0.05f, false, {255, 232, 100}, 400, 70, 26};
        case 3:  return {{45, 45, 105},  {245, 130, 80},  {70, 75, 70},   {35, 40, 45},  {255, 120, 60}, 0.10f, false, {255, 170, 90}, 640, 378, 34};
        case 4:  return {{8, 10, 30},    {38, 42, 78},    {30, 38, 45},   {12, 16, 22},  {20, 30, 90},   0.18f, true,  {240, 238, 215}, 620, 70, 18};
        default: return {{20, 30, 70},   {125, 110, 145}, {45, 55, 60},   {20, 28, 32},  {90, 100, 180}, 0.10f, false, {255, 200, 120}, 420, 385, 20};
    }
}

void drawSky(const Mat3& M, const Pal& p, float horizon, float t) {
    fill(M, rect(-40, -2000, 880, 1800), p.top);
    grad(M, rect(-40, -200, 880, horizon + 200), p.top, p.bot);
    if (p.night) {                                                   // twinkling stars
        for (int i = 0; i < 46; i++) {
            float tw = 0.5f + 0.5f * std::sin(t * 2 + i);
            Color c = mix({60, 60, 90}, {255, 255, 255}, tw);
            disc(M, {hash(i) * 800, hash(i + 99) * (horizon - 60)}, 1.2f, c);
        }
    }
    if (p.night) {                                                   // crescent moon with a cool glow and a faint dark side
        Vec2 mc = {p.sx, p.sy};
        glow(M, mc, p.sr * 3.0f, {170, 185, 255}, 0.07f);
        glow(M, mc, p.sr * 1.8f, {200, 210, 255}, 0.10f);
        shade(M, ellipse(mc.x, mc.y, p.sr * 1.25f, p.sr * 1.25f, 32), {150, 160, 200}, 0.10f);
        fill(M, crescent(mc, p.sr * 1.25f, -0.45f), p.sun);
    } else {                                                         // sun
        glow(M, {p.sx, p.sy}, p.sr * 3.2f, p.sun, 0.10f);
        glow(M, {p.sx, p.sy}, p.sr * 1.8f, p.sun, 0.15f);
        disc(M, {p.sx, p.sy}, p.sr, p.sun);
    }
    {   // drifting clouds
        static const int cnt[6] = {4, 6, 3, 6, 3, 4};
        int sky = p.night ? 4 : (p.sx > 600 && p.sy > 300 ? 3 : 1);
        (void)sky;
        for (int k = 0; k < 6; k++) {
            if (k >= (p.night ? 3 : (p.tintA > 0.08f ? 6 : (p.tintA > 0.04f ? 3 : 5)))) break;
            float cx = std::fmod(hash(k + 7) * 1100.0f + t * (2.5f + hash(k) * 4.0f), 1100.0f) - 150.0f;
            float cy = 50 + hash(k + 40) * (horizon - 230), w = 70 + hash(k + 80) * 110;
            Color cc = p.night ? Color{45, 50, 85} : mix({255, 255, 255}, p.bot, 0.35f);
            for (int q = 0; q < 4; q++) glow(M, {cx + (q - 1.5f) * w * 0.35f, cy + (q % 2) * 6.0f}, w * (0.45f + 0.1f * (q % 2)), cc, 0.22f);
        }
        if (!p.night && p.tintA < 0.09f)                                 // a few birds
            for (int b = 0; b < 3; b++) {
                float bx = std::fmod(t * (22.0f + b * 5) + b * 260.0f, 1000.0f) - 100.0f, by = 90 + b * 30 + std::sin(t * 0.8f + b) * 10;
                float fl = std::sin(t * 9 + b * 2) * 5;
                line(M, {bx - 8, by - fl}, {bx, by}, {30, 30, 35}); line(M, {bx, by}, {bx + 8, by - fl}, {30, 30, 35});
            }
    }
    grad(M, rect(-40, horizon, 880, 700 - horizon), p.gTop, p.gBot);
    fill(M, rect(-40, 700, 880, 1300), p.gBot);
}

// ---------- Kenyan flag (waves using animation.cpp) ----------
// flagWave() measures the wave from a pole at x = 702, so a flag drawn anywhere else would not move at all.
// This puts each point at that reference position, waves it, and keeps only the vertical movement.
// The time is offset by the pole's x, so two flags in one shot do not wave in step.
Vec2 wavePoint(float along, float y, float poleX, float t) {
    Vec2 w = flagWave({702.0f + along, y}, t + poleX * 0.017f);
    return {poleX + along + (w.x - (702.0f + along)), w.y};
}
void flag(const Mat3& M, float x, float topY, float w, float h, float t) {
    struct S { float h; Color a, b; };
    const S s[5] = {{0.27f, {25, 25, 25}, {0, 0, 0}}, {0.04f, {245, 245, 245}, {215, 215, 215}},
                    {0.38f, {200, 30, 30}, {140, 15, 15}}, {0.04f, {245, 245, 245}, {215, 215, 215}},
                    {0.27f, {30, 140, 65}, {12, 90, 40}}};
    float y = topY;
    for (int k = 0; k < 5; k++) {
        float hh = h * s[k].h;
        Poly strip;
        for (float xx = 0; xx <= w; xx += 6) strip.push_back(wavePoint(xx, y, x, t));
        for (float xx = w; xx >= 0; xx -= 6)  strip.push_back(wavePoint(xx, y + hh, x, t));
        grad(M, strip, s[k].a, s[k].b, true);
        y += hh;
    }
    Vec2 c = wavePoint(w / 2, topY + h / 2, x, t);                 // shield and spears
    line(M, {c.x - w * 0.18f, c.y + h * 0.30f}, {c.x + w * 0.18f, c.y - h * 0.30f}, {235, 235, 235});
    line(M, {c.x + w * 0.18f, c.y + h * 0.30f}, {c.x - w * 0.18f, c.y - h * 0.30f}, {235, 235, 235});
    Poly sh = ellipse(c.x, c.y, w * 0.10f, h * 0.30f);
    fill(M, sh, {195, 28, 28});
    render::drawPolygon(transformPoly(M, sh), {245, 245, 245});
}
void flagPole(const Mat3& M, float x, float baseY, float topY, bool halfMast, float t, float w = 74, float h = 46) {
    line(M, {x, baseY}, {x, topY}, {95, 95, 100});
    line(M, {x + 1, baseY}, {x + 1, topY}, {170, 170, 175});
    disc(M, {x, topY - 3}, 3.0f, {215, 180, 70});
    flag(M, x + 2, halfMast ? topY + 46 : topY + 2, w, h, t);
}

// ---------- articulated people (walk cycle, arms, hair, clothes, ground shadow) ----------
struct Look { Color skin, top, bottom, hair, sign; int hairStyle; bool dress; };
enum ArmMode { ARM_DOWN, ARM_FIST, ARM_PLACARD, ARM_PHONE, ARM_CANDLE, ARM_BOTH_UP };

Look makeLook(int id, int seed, const Mood& m) {
    static const Color skin[5]   = {{124, 74, 36}, {108, 64, 32}, {94, 56, 30}, {70, 42, 24}, {116, 69, 34}};
    static const Color top[10]   = {{40, 60, 110}, {150, 40, 40}, {50, 100, 65}, {90, 90, 100}, {170, 130, 45}, {70, 45, 100}, {205, 205, 208}, {35, 35, 40}, {200, 110, 40}, {60, 120, 150}};
    static const Color bottom[5] = {{35, 45, 80}, {30, 30, 34}, {62, 56, 50}, {48, 62, 92}, {25, 25, 28}};
    static const Color hair[3]   = {{20, 16, 14}, {36, 26, 18}, {56, 40, 28}};
    static const Color sign[5]   = {{245, 245, 240}, {245, 210, 70}, {228, 85, 70}, {95, 178, 108}, {125, 172, 232}};
    int h = id * 7 + seed * 13;
    Look L;
    L.skin   = skin[int(hash(h + 1) * 5) % 5];
    L.top    = m.mourning ? (hash(h + 2) < 0.7f ? Color{25, 25, 30} : Color{205, 205, 210}) : top[int(hash(h + 2) * 10) % 10];
    L.bottom = bottom[int(hash(h + 3) * 5) % 5];
    L.hair   = hair[int(hash(h + 4) * 3) % 3];
    L.sign   = sign[int(hash(h + 8) * 5) % 5];
    L.dress  = hash(h + 5) < 0.28f;
    L.hairStyle = L.dress ? (hash(h + 6) < 0.5f ? 2 : 1) : (hash(h + 6) < 0.2f ? 3 : 0);
    if (m.sky == 4) { L.top = mix(L.top, {10, 12, 30}, 0.5f); L.bottom = mix(L.bottom, {10, 12, 30}, 0.5f); L.skin = mix(L.skin, {20, 20, 45}, 0.3f); }
    if (m.sky == 3) { L.top = mix(L.top, {60, 30, 50}, 0.25f); L.skin = mix(L.skin, {200, 90, 60}, 0.15f); }
    return L;
}

void limb(const Mat3& M, Vec2 a, Vec2 b, float w0, float w1, Color c) {
    Vec2 d = {b.x - a.x, b.y - a.y}; float L = std::hypot(d.x, d.y); if (L < 0.001f) return;
    Vec2 n = {-d.y / L, d.x / L};
    fill(M, {{a.x + n.x * w0, a.y + n.y * w0}, {b.x + n.x * w1, b.y + n.y * w1}, {b.x - n.x * w1, b.y - n.y * w1}, {a.x - n.x * w0, a.y - n.y * w0}}, c);
}
void label(const Mat3& M, const char* text, float x, float y, float widthWorld, Color c, float alpha = 1.0f) {
    float z = zoomOf(M); Vec2 sp = M * Vec2{x, y};
    font::draw(text, sp.x, sp.y, font::fit(text, widthWorld * z, 6.0f * z), c, alpha, true, false);
}

void figure(const Mat3& M, float x, float y, float s, const Look& L, float ph, float walk, ArmMode arm, float t, float bow = 0.0f, bool shadowOn = true) {
    float bob = std::fabs(std::sin(ph)) * 0.06f * s * walk;
    float yb = y - bob, hipY = yb - 1.45f * s, shY = yb - 2.42f * s;
    float sw = std::sin(ph) * 0.5f * walk * s;
    Color dkBot = mix(L.bottom, {0, 0, 0}, 0.35f);
    if (shadowOn) shade(M, ellipse(x, y + 0.03f * s, 0.85f * s, 0.16f * s, 12), {0, 0, 0}, 0.30f);
    for (int k = 0; k < 2; k++) {                                                      // legs
        float side = k ? 1.0f : -1.0f, swing = k ? -sw : sw;
        float lift = std::max(0.0f, k ? -std::cos(ph) : std::cos(ph)) * 0.16f * s * walk;
        Vec2 hip = {x + side * 0.16f * s, hipY}, foot = {x + side * 0.18f * s + swing, y - lift};
        Vec2 knee = {(hip.x + foot.x) / 2 + (swing > 0 ? 0.05f * s : 0.0f), (hip.y + foot.y) / 2 - lift * 0.3f};
        if (L.dress) limb(M, {hip.x, hip.y + 0.55f * s}, foot, 0.1f * s, 0.07f * s, L.skin);
        else { Color lc = k ? L.bottom : dkBot; limb(M, hip, knee, 0.15f * s, 0.12f * s, lc); limb(M, knee, foot, 0.12f * s, 0.09f * s, lc); }
        fill(M, {{foot.x - 0.12f * s, foot.y - 0.07f * s}, {foot.x + 0.24f * s, foot.y - 0.05f * s}, {foot.x + 0.26f * s, foot.y + 0.02f * s}, {foot.x - 0.12f * s, foot.y + 0.02f * s}}, {28, 24, 22});
    }
    if (L.dress) grad(M, {{x - 0.5f * s, shY}, {x + 0.5f * s, shY}, {x + 0.68f * s, yb - 0.55f * s}, {x - 0.68f * s, yb - 0.55f * s}}, L.top, mix(L.top, {0, 0, 0}, 0.3f));
    else {
        grad(M, {{x - 0.52f * s, shY}, {x + 0.52f * s, shY}, {x + 0.42f * s, hipY + 0.12f * s}, {x - 0.42f * s, hipY + 0.12f * s}}, L.top, mix(L.top, {0, 0, 0}, 0.3f));
        fill(M, rect(x - 0.42f * s, hipY + 0.05f * s, 0.84f * s, 0.1f * s), dkBot);
    }
    for (int k = 0; k < 2; k++) {                                                      // arms
        float side = k ? 1.0f : -1.0f, armSw = (k ? sw : -sw) * 0.8f;
        Vec2 sho = {x + side * 0.52f * s, shY + 0.1f * s};
        Vec2 hand = {sho.x + side * 0.12f * s + armSw * 0.4f, sho.y + 1.0f * s - std::fabs(armSw) * 0.1f};
        if (k == 1) {
            switch (arm) {
                case ARM_FIST:    hand = {x + 0.75f * s, shY - (0.55f + 0.2f * std::sin(t * 4 + ph)) * s}; break;
                case ARM_PLACARD: hand = {x + 0.62f * s, shY - 0.45f * s}; break;
                case ARM_PHONE:   hand = {x + 0.3f * s, shY - 0.5f * s}; break;
                case ARM_CANDLE:  hand = {x + 0.5f * s, shY + 0.7f * s}; break;
                case ARM_BOTH_UP: hand = {x + 0.8f * s, shY - 0.7f * s}; break;
                default: break;
            }
        } else if (arm == ARM_BOTH_UP) hand = {x - 0.8f * s, shY - 0.7f * s};
        Vec2 elb = {(sho.x + hand.x) / 2 + side * 0.12f * s, (sho.y + hand.y) / 2 + 0.12f * s};
        limb(M, sho, elb, 0.12f * s, 0.10f * s, L.top);
        limb(M, elb, hand, 0.10f * s, 0.08f * s, L.skin);
        disc(M, hand, 0.1f * s, L.skin);
        if (k == 1 && arm == ARM_PLACARD) {
            float sway = std::sin(t * 2.0f + ph) * 0.2f * s;
            Vec2 top = {hand.x + 0.05f * s + sway, shY - 2.7f * s};
            limb(M, {hand.x, hand.y + 0.8f * s}, top, 0.04f * s, 0.04f * s, {110, 80, 50});
            Poly b = {{top.x - 1.05f * s, top.y - 1.4f * s}, {top.x + 1.05f * s, top.y - 1.45f * s}, {top.x + 1.05f * s, top.y + 0.02f * s}, {top.x - 1.05f * s, top.y}};
            fill(M, b, L.sign); render::drawPolygon(transformPoly(M, b), {40, 35, 30});
            for (int q = 0; q < 3; q++) line(M, {top.x - 0.8f * s, top.y - (1.05f - q * 0.3f) * s}, {top.x + (0.8f - 0.25f * q) * s, top.y - (1.07f - q * 0.3f) * s}, {45, 30, 30});
        } else if (k == 1 && arm == ARM_PHONE) {
            fill(M, rect(hand.x - 0.09f * s, hand.y - 0.3f * s, 0.18f * s, 0.3f * s), {18, 18, 24});
            glow(M, {hand.x, hand.y - 0.15f * s}, 0.35f * s, {150, 190, 255}, 0.25f);
        } else if (k == 1 && arm == ARM_CANDLE) {
            float fl = 0.6f + 0.4f * std::sin(t * 9 + ph * 2.3f);
            fill(M, rect(hand.x - 0.1f * s, hand.y - 0.45f * s, 0.2f * s, 0.5f * s), {240, 235, 215});
            glow(M, {hand.x, hand.y - 0.65f * s}, 2.0f * s, {255, 190, 80}, 0.16f * fl);
            disc(M, {hand.x, hand.y - 0.6f * s}, 0.16f * s * fl + 0.07f * s, {255, 215, 100});
        }
    }
    float hx = x + bow * 0.14f * s, hy = shY - 0.45f * s + bow * 0.12f * s;           // head
    limb(M, {x, shY}, {hx, hy + 0.2f * s}, 0.1f * s, 0.1f * s, L.skin);
    if (L.hairStyle == 1) disc(M, {hx, hy - 0.08f * s}, 0.43f * s, L.hair);
    if (L.hairStyle == 2) fill(M, ellipse(hx, hy - 0.02f * s, 0.42f * s, 0.46f * s, 14), mix(L.top, {0, 0, 0}, 0.2f));
    fill(M, ellipse(hx, hy, 0.29f * s, 0.36f * s, 14), L.skin);
    if (L.hairStyle == 0) fill(M, ellipse(hx, hy - 0.17f * s, 0.3f * s, 0.2f * s, 14), L.hair);
    if (L.hairStyle == 3) { fill(M, ellipse(hx, hy - 0.2f * s, 0.32f * s, 0.2f * s, 14), L.top); fill(M, rect(hx - 0.1f * s, hy - 0.12f * s, 0.42f * s, 0.06f * s), mix(L.top, {0, 0, 0}, 0.3f)); }
    if (s >= 15) { disc(M, {hx - 0.1f * s, hy - 0.02f * s}, 0.03f * s, {20, 15, 12}); disc(M, {hx + 0.1f * s, hy - 0.02f * s}, 0.03f * s, {20, 15, 12}); }
}

// ---------- people (crowds): march, wave, hold placards, film, mourn ----------
void people(const Mat3& M, const Mood& m, float t, float x0, float x1, float baseY, float size) {
    int n = std::max(0, m.crowd);
    bool protest = m.placards || m.fists;
    bool march = protest && m.energy >= 0.45f;
    float range = (x1 - x0) + 160.0f;
    for (int row = 0; row < 3; row++) {
        for (int i = row; i < n; i += 3) {
            float base = x0 + (x1 - x0) * (i + 0.5f) / std::max(1, n) + (hash(i * 7 + m.seed) - 0.5f) * size * 1.4f;
            float s = size * (1.0f + 0.14f * row) * (0.94f + 0.12f * hash(i + 3));
            float x = base, y = baseY + row * size * 0.95f;
            float spd = (14.0f + 26.0f * m.energy) * (1.0f - 0.08f * row);
            if (march) x = x0 - 80.0f + std::fmod(base - x0 + 80.0f + t * spd, range);
            Look L = makeLook(i + row * 100, m.seed, m);
            float ph = march ? t * (5.0f + 3.0f * m.energy) + i * 1.7f : t * 1.1f + i;
            float walk = march ? 1.0f : 0.0f;
            if (!march) { x += std::sin(t * 0.9f + i * 2.1f) * 0.06f * s; y -= std::fabs(std::sin(t * 3.0f + i * 1.7f)) * s * 0.2f * m.energy; }
            ArmMode arm = ARM_DOWN; float r = hash(i * 3 + m.seed + 31);
            if (m.candles) arm = ARM_CANDLE;
            else if (m.placards && r < 0.5f) arm = ARM_PLACARD;
            else if (m.fists && hash(i + 77 + m.seed) < 0.5f) arm = (hash(i + 5) < 0.3f ? ARM_BOTH_UP : ARM_FIST);
            else if (!m.mourning && hash(i + m.seed * 5 + 9) < 0.1f) arm = ARM_PHONE;
            float bow = m.mourning ? 0.55f + 0.15f * std::sin(t * 0.7f + i) : 0.0f;
            figure(M, x, y, s, L, ph, walk, arm, t, bow);
        }
    }
}
// a cloth banner carried between two marchers
void banner(const Mat3& M, const Mood& m, float t, float cx, float baseY, float size) {
    if (!m.banner) return;
    bool march = m.energy >= 0.45f;
    float w = 340.0f; cx += std::sin(t * 0.45f) * 18.0f;
    float top = baseY - 5.4f * size, h = 1.9f * size;
    for (int e = 0; e < 2; e++) {
        float px = cx + (e ? 1 : -1) * w / 2;
        limb(M, {px, top - 6}, {px, baseY - 1.2f * size}, 1.8f, 1.8f, {105, 80, 55});
        figure(M, px, baseY, size * 1.1f, makeLook(900 + e * 7, m.seed, m), t * 6.0f + e * 2.0f, march ? 1.0f : 0.0f, ARM_DOWN, t);
    }
    Poly cloth, lower;
    for (int k = 0; k <= 12; k++) cloth.push_back({cx - w / 2 + w * k / 12, top + std::sin(t * 2.0f + k * 0.6f) * 2.5f});
    for (int k = 12; k >= 0; k--) cloth.push_back({cx - w / 2 + w * k / 12, top + h + std::sin(t * 2.0f + k * 0.6f + 0.8f) * 2.5f});
    fill(M, cloth, {240, 238, 228}); render::drawPolygon(transformPoly(M, cloth), {60, 55, 50});
    label(M, m.banner, cx, top + h * 0.28f, w * 0.88f, {170, 25, 25});
}
void policeLine(const Mat3& M, float t, float x0, float x1, float baseY, float size) {
    Look L; L.skin = {105, 65, 35}; L.top = {38, 52, 86}; L.bottom = {30, 38, 62}; L.hair = {20, 20, 20}; L.sign = {0, 0, 0}; L.hairStyle = 3; L.dress = false;
    for (float x = x0; x < x1; x += size * 1.6f) {
        figure(M, x, baseY, size, L, 0, 0, ARM_DOWN, t);
        float cx = x - 0.14f * size, top = baseY - 2.30f * size;
        Poly crest = {{cx - 0.18f * size, top}, {cx + 0.18f * size, top}, {cx + 0.18f * size, top + 0.46f * size}, {cx, top + 0.62f * size}, {cx - 0.18f * size, top + 0.46f * size}};
        fill(M, crest, {238, 232, 211});
        fill(M, rect(cx - 0.14f * size, top + 0.03f * size, 0.28f * size, 0.16f * size), {12, 12, 14});
        fill(M, rect(cx - 0.14f * size, top + 0.20f * size, 0.28f * size, 0.16f * size), {185, 30, 38});
        fill(M, rect(cx - 0.14f * size, top + 0.37f * size, 0.28f * size, 0.15f * size), {24, 105, 55});
        Poly sh = {{x + 0.15f * size, baseY - 2.6f * size}, {x + 1.05f * size, baseY - 2.6f * size}, {x + 1.05f * size, baseY - 0.7f * size}, {x + 0.15f * size, baseY - 0.7f * size}};
        shade(M, sh, {190, 220, 245}, 0.5f); render::drawPolygon(transformPoly(M, sh), {210, 225, 240});
    }
}
void teargas(const Mat3& M, float t, int seed, float cx) {
    for (int k = 0; k < 24; k++) {
        float ph = std::fmod(t * 0.11f + hash(k + seed * 3), 1.0f);
        float x = cx + (hash(k * 3 + seed) - 0.5f) * 560.0f + ph * 70.0f, y = 480 - ph * 200.0f, r = 28 + ph * 80;
        shade(M, ellipse(x, y, r, r * 0.8f, 16), {225, 225, 222}, 0.17f * std::sin(ph * PI));
    }
    shade(M, rect(-40, 250, 880, 320), {205, 205, 200}, 0.07f);
}
// dark heads and shoulders at the bottom edge: "the people are watching"
void foreground(const Mat3& M, int n, float t, const Mood& m) {
    for (int i = 0; i < n; i++) {
        float x = 800.0f * (i + 0.5f) / n + (hash(i + 5) - 0.5f) * 40;
        float s = 34 + hash(i + 11) * 14;
        float sway = std::sin(t * 0.8f + i) * 2.0f;
        Color c = m.sky == 4 ? Color{4, 5, 12} : Color{14, 16, 22};
        disc(M, {x + sway, 612 - s * 1.7f}, s * 0.55f, c);
        fill(M, {{x - s * 0.9f + sway, 640}, {x - s * 0.7f + sway, 612 - s * 1.1f}, {x + s * 0.7f + sway, 612 - s * 1.1f}, {x + s * 0.9f + sway, 640}}, c);
    }
}

// ---------- buildings ----------
void classical(const Mat3& M, float cx, float baseY, float w, float h, int cols, Color lt, Color dk, bool emblem) {
    for (int s = 0; s < 3; s++)
        grad(M, rect(cx - w / 2 - 14 + s * 4, baseY - (s + 1) * 6, w + 28 - s * 8, 6), mix(lt, {255, 255, 255}, 0.1f), mix(dk, {0, 0, 0}, 0.2f));
    float by = baseY - 18;
    grad(M, rect(cx - w / 2, by - h, w, h), lt, dk);
    fill(M, rect(cx - w * 0.09f, by - h * 0.55f, w * 0.18f, h * 0.55f), mix(dk, {0, 0, 0}, 0.55f));
    for (int i = 0; i < cols; i++) {
        float x = cx - w / 2 + 10 + (w - 40) * i / std::max(1, cols - 1);
        grad(M, rect(x, by - h, 22, h), mix(lt, {255, 255, 255}, 0.4f), mix(dk, {0, 0, 0}, 0.15f), true);
    }
    grad(M, {{cx - w / 2 - 14, by - h}, {cx + w / 2 + 14, by - h}, {cx, by - h - h * 0.38f}}, mix(lt, {255, 255, 255}, 0.15f), mix(dk, {0, 0, 0}, 0.1f));
    render::drawPolygon(transformPoly(M, {{cx - w / 2 - 14, by - h}, {cx + w / 2 + 14, by - h}, {cx, by - h - h * 0.38f}}), mix(dk, {0, 0, 0}, 0.4f));
    if (emblem) disc(M, {cx, by - h - h * 0.13f}, 14, {195, 155, 60});
}

void windowsGrid(const Mat3& M, float x, float y, float w, float h, bool night, int seed) {
    int cols = std::max(1, int(w / 16)), rows = std::max(1, int(h / 22));
    for (int r = 0; r < rows; r++)
        for (int c = 0; c < cols; c++) {
            bool lit = night && hash(seed + r * 13 + c * 7) < 0.45f;
            Color wc = lit ? Color{255, 215, 120} : (night ? Color{25, 32, 55} : Color{140, 170, 195});
            fill(M, rect(x + 6 + c * (w - 12) / cols, y + 8 + r * (h - 14) / rows, (w - 12) / cols - 4, (h - 14) / rows - 6), wc);
        }
}

// ================== LOCATIONS ==================
void locCourt(const Mat3& M, float t, const Mood& m, const Pal& p) {
    drawSky(M, p, 395, t);
    classical(M, 400, 410, 440, 170, 6, {238, 232, 218}, {185, 178, 162}, true);
    flagPole(M, 150, 410, 180, m.halfMast, t);
    flagPole(M, 650, 410, 180, m.halfMast, t);
    people(M, m, t, 90, 710, 452, 15);
}
void locBomas(const Mat3& M, float t, const Mood& m, const Pal& p) {
    drawSky(M, p, 395, t);
    grad(M, rect(150, 320, 500, 80), {225, 220, 205}, {170, 165, 150});
    Poly dome;
    for (int i = 0; i <= 24; i++) { float a = PI * i / 24; dome.push_back({400 - 240 * std::cos(a), 320 - 125 * std::sin(a)}); }
    grad(M, dome, {240, 235, 222}, {150, 145, 135}, true);
    for (int i = 1; i < 6; i++) line(M, {400 - 240 * std::cos(PI * i / 6), 320 - 125 * std::sin(PI * i / 6)}, {400, 320}, {170, 165, 150});
    grad(M, rect(357, 338, 86, 62), {235, 230, 216}, {190, 185, 170});            // stone frame round the door
    fill(M, rect(365, 345, 70, 55), {50, 45, 45});                                  // door
    for (int i = 0; i < 6; i++) {                                                   // six windows each side, clear of the door
        float wx = 170 + i * 31.8f;                                                 // left block ends at x = 347
        fill(M, rect(wx, 335, 18, 50), {95, 100, 110});
        fill(M, rect(800 - 18 - wx, 335, 18, 50), {95, 100, 110});                  // mirror image on the right
    }
    flagPole(M, 110, 410, 190, m.halfMast, t);
    flagPole(M, 690, 410, 190, m.halfMast, t);
    people(M, m, t, 60, 740, 450, 14);
}
void locStreet(const Mat3& M, float t, const Mood& m, const Pal& p) {
    const float horizon = 392;
    drawSky(M, p, horizon, t);
    const Color walls[5] = {{175, 170, 160}, {150, 155, 165}, {190, 175, 150}, {135, 140, 150}, {170, 160, 155}};
    float x = -40; int i = 0;
    while (x < 840) {
        float w = 80 + hash(m.seed * 100 + i) * 60, h = 140 + hash(m.seed * 100 + i + 50) * 160;
        Color wc = walls[int(hash(m.seed * 100 + i + 20) * 5) % 5];
        if (p.night) wc = mix(wc, {10, 14, 30}, 0.7f);
        grad(M, rect(x, horizon - h, w, h + 8), wc, mix(wc, {0, 0, 0}, 0.3f));
        windowsGrid(M, x, horizon - h, w, h, p.night, m.seed * 500 + i * 31);
        x += w + 4; i++;
    }
    fill(M, rect(-40, horizon + 6, 880, 24), p.night ? Color{40, 42, 52} : Color{170, 168, 160});
    grad(M, rect(-40, horizon + 30, 880, 300), p.night ? Color{28, 30, 38} : Color{75, 78, 85}, p.night ? Color{14, 16, 22} : Color{50, 52, 58});
    for (int d = 0; d < 9; d++) fill(M, rect(d * 100.0f - 20, 520, 52, 5), {225, 225, 215});
    if (m.sky != 4) {                                                // a matatu drifting by
        float bx = std::fmod(t * 30.0f + m.seed * 200.0f, 1100.0f) - 200.0f;
        grad(M, rect(bx, 448, 140, 52), {240, 240, 240}, {190, 190, 195});
        fill(M, rect(bx, 470, 140, 10), {40, 150, 90});
        for (int w = 0; w < 4; w++) fill(M, rect(bx + 8 + w * 32, 455, 24, 14), {80, 110, 140});
        disc(M, {bx + 28, 502}, 9, {20, 20, 22}); disc(M, {bx + 112, 502}, 9, {20, 20, 22});
    }
    if (m.police) policeLine(M, t, 20, 330, 436, 9.5f);
    people(M, m, t, 40, 760, 478, 13);
    banner(M, m, t, 400, 520, 15);
    if (m.smoke) teargas(M, t, m.seed, 230);
}
void locParliament(const Mat3& M, float t, const Mood& m, const Pal& p) {
    drawSky(M, p, 395, t);
    grad(M, rect(110, 270, 580, 125), {200, 195, 182}, {135, 130, 122});
    windowsGrid(M, 110, 270, 580, 125, p.night, m.seed + 900);
    grad(M, rect(375, 120, 50, 160), {215, 210, 196}, {150, 145, 135}, true);
    grad(M, {{368, 120}, {432, 120}, {400, 78}}, {120, 118, 122}, {70, 68, 74});
    disc(M, {400, 160}, 15, {245, 242, 230});
    float a = t * 0.25f;
    line(M, {400, 160}, {400 + 10 * std::sin(a), 160 - 10 * std::cos(a)}, {30, 30, 30});
    line(M, {400, 160}, {400 + 7 * std::sin(a * 12), 160 - 7 * std::cos(a * 12)}, {30, 30, 30});
    fill(M, rect(-40, 392, 880, 8), {40, 42, 46});
    for (int i = 0; i < 44; i++) line(M, {-20.0f + i * 20.0f, 340}, {-20.0f + i * 20.0f, 395}, {25, 25, 30});
    line(M, {-40, 345}, {840, 345}, {25, 25, 30});
    flagPole(M, 90, 395, 175, m.halfMast, t);
    if (m.police) policeLine(M, t, 150, 560, 424, 10.5f);
    people(M, m, t, 30, 770, 462, 15);
    banner(M, m, t, 400, 505, 16);
    if (m.smoke) teargas(M, t, m.seed, 380);
}
void locStateHouse(const Mat3& M, float t, const Mood& m, const Pal& p) {
    drawSky(M, p, 395, t);
    grad(M, rect(60, 300, 220, 95), {245, 243, 238}, {200, 196, 188});
    grad(M, rect(520, 300, 220, 95), {245, 243, 238}, {200, 196, 188});
    windowsGrid(M, 60, 300, 220, 95, p.night, 20); windowsGrid(M, 520, 300, 220, 95, p.night, 40);
    classical(M, 400, 400, 340, 140, 5, {250, 248, 242}, {205, 200, 192}, false);
    flagPole(M, 400, 192, 108, m.halfMast, t, 62, 40);
    // Podium and speaker are drawn as one group at crowd scale (about 0.46 of the 194-unit person used elsewhere).
    // Local units: origin = centre of the podium base, y grows downward.
    const float k = 0.46f;
    Mat3 G = M * translate(400, 450) * scale(k, k);
    shade(G, ellipse(0, 4, 78, 9), {0, 0, 0}, 0.30f);                                  // shadow on the ground
    Color suit = {32, 38, 70};
    fill(G, rect(-22, -96, 44, 86), mix(suit, {0, 0, 0}, 0.35f));                      // legs
    fill(G, {{-30, -162}, {30, -162}, {25, -92}, {-25, -92}}, suit);                   // torso
    fill(G, {{-5, -162}, {5, -162}, {3, -128}, {0, -122}, {-3, -128}}, {170, 30, 35}); // tie
    fill(G, {{-30, -160}, {-42, -158}, {-46, -98}, {-33, -96}}, suit);                 // left arm, resting
    float ang = -1.1f * (0.5f + 0.5f * std::sin(t * 1.6f));                            // right arm gestures
    fill(G, transformPoly(rotateAbout(ang, {30, -158}), {{27, -162}, {41, -158}, {49, -92}, {35, -90}}), suit);
    fill(G, rect(-5, -178, 10, 18), {150, 98, 62});                                    // neck
    disc(G, {0, -192}, 16, {150, 98, 62});                                             // head
    fill(G, ellipse(0, -204, 15, 7), {25, 22, 22});                                    // hair
    grad(G, {{-48, -100}, {48, -100}, {60, 0}, {-60, 0}}, {150, 102, 62}, {96, 62, 36}, true);   // podium
    grad(G, rect(-54, -110, 108, 12), {170, 120, 74}, {120, 78, 44});
    disc(G, {0, -52}, 11, {200, 160, 62});
    line(G, {0, -110}, {0, -150}, {40, 40, 40});                                        // microphone
    disc(G, {0, -153}, 4.5f, {30, 30, 30});
}
void locStadium(const Mat3& M, float t, const Mood& m, const Pal& p) {
    drawSky(M, p, 330, t);
    grad(M, {{40, 330}, {760, 330}, {800, 470}, {0, 470}}, {120, 122, 130}, {60, 62, 70});
    for (int tier = 0; tier < 4; tier++)
        for (int k = 0; k < 44; k++) {
            float x = 28 + k * 17.5f + (tier % 2) * 8, y = 352 + tier * 28;
            static const Color festive[6] = {{200, 50, 45}, {50, 90, 170}, {60, 140, 80}, {225, 190, 60}, {235, 235, 235}, {120, 70, 150}};
            float hv = hash(k * 13 + tier * 7 + 3);
            Color c = m.mourning ? (hv < 0.7f ? Color{22, 22, 28} : Color{200, 200, 205}) : festive[int(hv * 6) % 6];   // dark for a funeral, colourful otherwise
            disc(M, {x, y - std::fabs(std::sin(t * 2 + k)) * 0.8f}, 5.5f, c);
            fill(M, rect(x - 5, y + 4, 10, 10), mix(c, {0, 0, 0}, 0.2f));
        }
    grad(M, ellipse(400, 505, 380, 62), {70, 130, 70}, {40, 90, 50});
    fill(M, rect(340, 470, 120, 10), {60, 60, 70});
    fill(M, rect(385, 478, 30, 12), {120, 90, 60});                  // a draped table at the centre of the pitch
    flagPole(M, 690, 470, 150, m.halfMast, t);
    foreground(M, 9, t, m);
}
void locHomestead(const Mat3& M, float t, const Mood& m, const Pal& p) {
    drawSky(M, p, 380, t);
    grad(M, rect(-40, 340, 880, 42), {70, 120, 165}, {110, 160, 195});                            // lake
    Poly hills = {{-40, 400}};
    for (int x = -40; x <= 840; x += 20) hills.push_back({float(x), 372 - 26 * std::sin(x * 0.011f + 1.0f) - 14 * std::sin(x * 0.027f)});
    hills.push_back({840, 400});
    grad(M, hills, mix(p.gTop, {70, 100, 60}, 0.5f), p.gTop);
    for (int i = 0; i < 4; i++) {
        float x = 70 + i * 210 + hash(i + 3) * 40, y = 415 + hash(i + 8) * 25;
        fill(M, {{x - 3, y}, {x + 3, y}, {x + 1, y - 46}, {x - 1, y - 46}}, {50, 38, 28});
        grad(M, ellipse(x, y - 52, 46, 12), {55, 100, 50}, {30, 65, 35});
    }
    grad(M, rect(470, 400, 130, 70), {200, 190, 170}, {150, 140, 120});                          // house
    grad(M, {{458, 400}, {612, 400}, {535, 352}}, {150, 90, 70}, {100, 60, 50});
    fill(M, rect(525, 425, 24, 45), {70, 50, 40}); fill(M, rect(490, 418, 22, 22), {90, 110, 130}); fill(M, rect(565, 418, 22, 22), {90, 110, 130});
    flagPole(M, 640, 470, 250, m.halfMast, t, 60, 38);
    people(M, m, t, 200, 620, 492, 14);
}
void locBallot(const Mat3& M, float t, const Mood& m, const Pal& p) {
    drawSky(M, p, 395, t);
    grad(M, rect(150, 290, 500, 105), {225, 215, 190}, {170, 160, 140});
    grad(M, {{140, 290}, {660, 290}, {600, 250}, {200, 250}}, {150, 80, 60}, {105, 55, 45});
    fill(M, rect(360, 330, 80, 65), {70, 55, 45});
    fill(M, rect(300, 305, 200, 18), {245, 245, 245});
    for (int i = 0; i < 12; i++) fill(M, rect(310 + i * 15, 309, 9, 10), i % 3 == 0 ? Color{200, 40, 40} : (i % 3 == 1 ? Color{30, 30, 30} : Color{40, 140, 70}));
    flagPole(M, 110, 400, 190, m.halfMast, t);
    people(M, m, t, 40, 760, 452, 13);
    // ballot box in the foreground
    grad(M, {{290, 600}, {510, 600}, {500, 505}, {300, 505}}, {70, 110, 170}, {40, 70, 125});
    fill(M, rect(365, 507, 70, 8), {15, 15, 20});
    float drop = std::fmod(t * 0.6f, 1.0f);
    fill(M, rect(372, 470 + drop * 42, 56, 38), {248, 248, 240});
    fill(M, rect(380, 480 + drop * 42, 40, 3), {120, 120, 130}); fill(M, rect(380, 489 + drop * 42, 30, 3), {120, 120, 130});
}

} // namespace

void drawLocation(Loc loc, float t, const Mood& m, const Mat3& M) {
    Pal p = palette(m.sky);
    switch (loc) {
        case Loc::COURT:       locCourt(M, t, m, p); break;
        case Loc::BOMAS:       locBomas(M, t, m, p); break;
        case Loc::STREET:      locStreet(M, t, m, p); break;
        case Loc::PARLIAMENT:  locParliament(M, t, m, p); break;
        case Loc::STATE_HOUSE: locStateHouse(M, t, m, p); break;
        case Loc::STADIUM:     locStadium(M, t, m, p); break;
        case Loc::HOMESTEAD:   locHomestead(M, t, m, p); break;
        case Loc::BALLOT:      locBallot(M, t, m, p); break;
    }
    if (loc != Loc::STADIUM && loc != Loc::BALLOT) foreground(M, loc == Loc::HOMESTEAD ? 0 : 7, t, m);
    if (p.tintA > 0)                                                  // mood tint over the whole frame
        shade(M, rect(-40, -300, 880, 1000), p.tint, p.tintA);
}
