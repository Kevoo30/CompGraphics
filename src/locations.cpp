#include "locations.h"
#include "render.h"
#include "animation.h"
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

// ---------- sky / ground palettes ----------
struct Pal { Color top, bot, gTop, gBot, tint; float tintA; bool night; Color sun; float sx, sy, sr; };
Pal palette(int sky) {
    switch (sky) {
        case 0:  return {{60, 90, 150},  {250, 190, 130}, {95, 105, 75},  {55, 70, 50},  {255, 170, 90}, 0.06f, false, {255, 225, 160}, 560, 372, 26};
        case 1:  return {{70, 130, 200}, {200, 225, 245}, {100, 130, 80}, {55, 85, 50},  {0, 0, 0},      0.00f, false, {255, 250, 220}, 150, 110, 22};
        case 2:  return {{35, 105, 200}, {225, 238, 250}, {135, 140, 85}, {95, 105, 60}, {255, 255, 235}, 0.05f, false, {255, 255, 240}, 400, 70, 26};
        case 3:  return {{45, 45, 105},  {245, 130, 80},  {70, 75, 70},   {35, 40, 45},  {255, 120, 60}, 0.10f, false, {255, 170, 90}, 640, 378, 34};
        case 4:  return {{8, 10, 30},    {38, 42, 78},    {30, 38, 45},   {12, 16, 22},  {20, 30, 90},   0.18f, true,  {235, 235, 245}, 620, 90, 18};
        default: return {{20, 30, 70},   {125, 110, 145}, {45, 55, 60},   {20, 28, 32},  {90, 100, 180}, 0.10f, false, {200, 190, 220}, 420, 385, 20};
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
    glow(M, {p.sx, p.sy}, p.sr * 3.2f, p.sun, 0.10f);
    glow(M, {p.sx, p.sy}, p.sr * 1.8f, p.sun, 0.15f);
    disc(M, {p.sx, p.sy}, p.sr, p.sun);
    grad(M, rect(-40, horizon, 880, 700 - horizon), p.gTop, p.gBot);
    fill(M, rect(-40, 700, 880, 1300), p.gBot);
}

// ---------- Kenyan flag (waves using animation.cpp) ----------
void flag(const Mat3& M, float x, float topY, float w, float h, float t) {
    struct S { float h; Color a, b; };
    const S s[5] = {{0.27f, {25, 25, 25}, {0, 0, 0}}, {0.04f, {245, 245, 245}, {215, 215, 215}},
                    {0.38f, {200, 30, 30}, {140, 15, 15}}, {0.04f, {245, 245, 245}, {215, 215, 215}},
                    {0.27f, {30, 140, 65}, {12, 90, 40}}};
    float y = topY;
    for (int k = 0; k < 5; k++) {
        float hh = h * s[k].h;
        Poly strip;
        for (float xx = 0; xx <= w; xx += 6) strip.push_back(flagWave({x + xx, y}, t));
        for (float xx = w; xx >= 0; xx -= 6)  strip.push_back(flagWave({x + xx, y + hh}, t));
        grad(M, strip, s[k].a, s[k].b, true);
        y += hh;
    }
    Vec2 c = flagWave({x + w / 2, topY + h / 2}, t);                 // shield and spears
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

// ---------- people ----------
void people(const Mat3& M, const Mood& m, float t, float x0, float x1, float baseY, float size) {
    const Color skin[4]  = {{141, 85, 36}, {198, 134, 66}, {224, 172, 105}, {94, 56, 30}};
    const Color shirt[8] = {{40, 60, 110}, {150, 40, 40}, {50, 100, 65}, {90, 90, 100}, {170, 130, 45}, {70, 45, 100}, {200, 200, 205}, {35, 35, 40}};
    const Color sign[4]  = {{235, 235, 235}, {240, 200, 60}, {220, 70, 60}, {80, 170, 100}};
    int n = std::max(0, m.crowd);
    for (int row = 0; row < 3; row++) {
        for (int i = row; i < n; i += 3) {
            float x = x0 + (x1 - x0) * (i + 0.5f) / std::max(1, n) + (hash(i * 7 + m.seed) - 0.5f) * size * 1.4f;
            float s = size * (1.0f + 0.14f * row);
            float bob = std::fabs(std::sin(t * 3.0f + i * 1.7f)) * s * 0.30f * m.energy;
            float y = baseY + row * size * 0.95f - bob;
            Color c = m.mourning ? shirt[(i % 2) ? 7 : 3] : shirt[(i * 5 + m.seed) % 8];
            if (m.sky == 4) c = mix(c, {10, 12, 30}, 0.45f);
            if (m.sky == 3) c = mix(c, {40, 20, 40}, 0.25f);
            Color sk = skin[(i * 3 + m.seed) % 4];
            fill(M, {{x - 0.55f * s, y - 2.2f * s}, {x + 0.55f * s, y - 2.2f * s}, {x + 0.70f * s, y}, {x - 0.70f * s, y}}, c);
            disc(M, {x, y - 2.78f * s}, 0.52f * s, sk);
            if (m.placards && hash(i + 31 + m.seed) < 0.55f) {
                float sway = std::sin(t * 2.0f + i) * 0.35f * s;
                Vec2 top = {x + 0.9f * s + sway, y - 4.6f * s};
                line(M, {x + 0.7f * s, y - 1.8f * s}, top, {110, 80, 50});
                fill(M, {{top.x - 1.2f * s, top.y - 1.6f * s}, {top.x + 1.2f * s, top.y - 1.6f * s}, {top.x + 1.2f * s, top.y}, {top.x - 1.2f * s, top.y}}, sign[(i + m.seed) % 4]);
            } else if (m.fists && hash(i + 77) < 0.5f) {
                float up = 0.5f + 0.5f * std::sin(t * 4 + i);
                fill(M, {{x + 0.45f * s, y - 2.1f * s}, {x + 0.75f * s, y - 2.0f * s}, {x + 1.05f * s, y - 3.4f * s - up * s * 0.5f}, {x + 0.75f * s, y - 3.5f * s - up * s * 0.5f}}, c);
            }
            if (m.candles) {
                float fl = 0.6f + 0.4f * std::sin(t * 9 + i * 2.3f);
                Vec2 hand = {x + 0.3f * s, y - 1.5f * s};
                fill(M, rect(hand.x - 0.12f * s, hand.y, 0.24f * s, 0.55f * s), {240, 235, 215});
                glow(M, {hand.x, hand.y - 0.3f * s}, 2.2f * s, {255, 190, 80}, 0.16f * fl);
                disc(M, {hand.x, hand.y - 0.28f * s}, 0.2f * s * fl + 0.1f * s, {255, 215, 100});
            }
        }
    }
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
    fill(M, rect(365, 345, 70, 55), {50, 45, 45});
    for (int i = 0; i < 6; i++) fill(M, rect(185 + i * 38, 335, 18, 50), {95, 100, 110});
    for (int i = 0; i < 6; i++) fill(M, rect(425 + 40 + i * 38 - 38, 335, 18, 50), {95, 100, 110});
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
    people(M, m, t, 40, 760, 478, 13);
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
    people(M, m, t, 30, 770, 462, 15);
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
            Color c = hash(k * 13 + tier * 7 + 3) < 0.7f ? Color{22, 22, 28} : Color{200, 200, 205};
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