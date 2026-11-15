#include "effects.h"
#include "render.h"
#include "animation.h"      // flagWave()
#include <algorithm>
#include <cmath>
#include <initializer_list>
#include <utility>

// Person 5: OpenGL port of the CP2 scene (scene.cpp) with gradients, shading and shadows.
//
// Lighting rule used everywhere: the light comes from the UPPER LEFT.
//   - vertical gradients are lighter at the top
//   - horizontal gradients are lighter on the left
//   - cast shadows fall to the RIGHT and slightly toward the viewer (down the screen)

namespace {

typedef std::vector<Vec2> Poly;

const float kPi = 3.14159265f;
const float kShadowX = 0.35f;     // shadow shift to the right per pixel of height
const float kShadowY = 0.12f;     // shadow shift down the screen per pixel of height
const float kPoleX   = 700.0f;    // flag pole x. flagWave() in animation.cpp measures from x = 702 (pole + 2)

// ------------------------------------------------------------------ colour helpers
uint8_t clamp8(float v) { return (uint8_t)std::max(0.0f, std::min(255.0f, v)); }
Color rgb(int r, int g, int b) { return Color{ clamp8((float)r), clamp8((float)g), clamp8((float)b) }; }
Color mul(Color c, float f) { return Color{ clamp8(c.r * f), clamp8(c.g * f), clamp8(c.b * f) }; }
Color mix(Color a, Color b, float t) {
    return Color{ clamp8(a.r + (b.r - a.r) * t), clamp8(a.g + (b.g - a.g) * t), clamp8(a.b + (b.b - a.b) * t) };
}
// f < 1 darkens, f > 1 pushes toward white (so black cloth can still catch light on a fold)
Color lit(Color c, float f) { return f <= 1.0f ? mul(c, f) : mix(c, rgb(255, 255, 255), (f - 1.0f) * 0.5f); }

// ------------------------------------------------------------------ geometry helpers
Poly rectP(float x, float y, float w, float h) { return { {x, y}, {x + w, y}, {x + w, y + h}, {x, y + h} }; }

Poly circleP(Vec2 c, float r, int n = 32) {
    Poly p;
    for (int i = 0; i < n; i++) {
        float a = 2.0f * kPi * i / n;
        p.push_back({ c.x + std::cos(a) * r, c.y + std::sin(a) * r });
    }
    return p;
}

float zoomOf(const Mat3& cam) { return std::hypot(cam.m[0], cam.m[3]); }

// Project a silhouette onto the ground. lineY is the y of the ground line the object stands on.
// A point h pixels above the line lands h*kShadowX to the right and h*kShadowY below it.
Poly castShadow(const Poly& p, float lineY) {
    Poly out;
    for (const Vec2& v : p) {
        float h = lineY - v.y;
        out.push_back({ v.x + h * kShadowX, lineY + h * kShadowY });
    }
    return out;
}

// ------------------------------------------------------------------ camera-aware drawing
void fill(const Mat3& cam, const Poly& p, Color c)                  { render::fillPolygon(transformPoly(cam, p), c); }
void grad(const Mat3& cam, const Poly& p, Color c1, Color c2, bool horizontal = false) {
    render::fillPolygonGradient(transformPoly(cam, p), c1, c2, horizontal);
}
void shade(const Mat3& cam, const Poly& p, float alpha)             { render::fillPolygonAlpha(transformPoly(cam, p), rgb(0, 0, 0), alpha); }
void glow(const Mat3& cam, const Poly& p, Color c, float alpha)     { render::fillPolygonAlpha(transformPoly(cam, p), c, alpha); }
void outline(const Mat3& cam, const Poly& p, Color c)               { render::drawPolygon(transformPoly(cam, p), c); }
void disc(const Mat3& cam, Vec2 c, float r, Color col)              { render::fillCircle(cam * c, r * zoomOf(cam), col); }
void ring(const Mat3& cam, Vec2 c, float r, Color col)              { render::drawCircle(cam * c, r * zoomOf(cam), col); }
void seg(const Mat3& cam, Vec2 a, Vec2 b, Color c)                  { render::drawLine(cam * a, cam * b, c); }

// A line with real thickness (render::drawLine is always ~1.5 px)
void thickLine(const Mat3& cam, Vec2 a, Vec2 b, float w, Color c) {
    float dx = b.x - a.x, dy = b.y - a.y, len = std::hypot(dx, dy);
    if (len < 1e-4f) return;
    float nx = -dy / len * w * 0.5f, ny = dx / len * w * 0.5f;
    fill(cam, { {a.x + nx, a.y + ny}, {b.x + nx, b.y + ny}, {b.x - nx, b.y - ny}, {a.x - nx, a.y - ny} }, c);
}

// Vertical gradient box, light at the top
void vbox(const Mat3& cam, float x, float y, float w, float h, Color base, float top = 1.06f, float bottom = 0.88f) {
    grad(cam, rectP(x, y, w, h), mul(base, top), mul(base, bottom));
}

const Color kEdge = {20, 20, 20};

} // namespace

// =============================================================== public API
namespace effects {

float groundY() { return render::height() * 11.0f / 15.0f; }

// ------------------------------------------------------------------ sky and ground
void drawSky(const Mat3& cam, float time) {
    const float gY = groundY();
    const Color top = rgb(62, 120, 195), horizon = rgb(205, 228, 246);
    // gradient over the visible sky, flat colour above it so zooming / panning never shows a gap
    grad(cam, rectP(-2000, 0, 4800, gY), top, horizon);
    fill(cam, rectP(-2000, -2000, 4800, 2000), top);

    // sun: bright core and soft concentric halos (semi-transparent discs)
    const Vec2 sun = { 120.0f, 95.0f };
    const Color warm = rgb(255, 238, 180);
    for (int i = 14; i >= 1; i--)                                   // many faint rings so the halo looks smooth
        glow(cam, circleP(sun, 24.0f + i * 6.5f, 48), warm, 0.035f);
    float pulse = 1.0f + 0.03f * std::sin(time * 1.5f);            // tiny breathing so the sun is not dead still
    disc(cam, sun, 24.0f * pulse, rgb(255, 247, 215));
}

void drawGround(const Mat3& cam) {
    const float W = (float)render::width(), H = (float)render::height();
    const float gY = groundY();
    const Color lawnTop = rgb(104, 142, 92), lawnBottom = rgb(58, 92, 52);

    grad(cam, rectP(-2000, gY, 4800, H - gY), lawnTop, lawnBottom);
    fill(cam, rectP(-2000, H, 4800, 2000), lawnBottom);              // flat extension below the window

    // pavement strip along the building, lit edge on top, dark kerb below
    grad(cam, rectP(0, gY, W, 26), rgb(176, 174, 168), rgb(138, 136, 130));
    fill(cam, rectP(0, gY, W, 2), rgb(200, 198, 192));
    fill(cam, rectP(0, gY + 26, W, 2), rgb(90, 88, 84));
    shade(cam, rectP(0, gY + 28, W, 6), 0.12f);                      // soft contact shadow under the kerb
}

// ------------------------------------------------------------------ building
void drawBuilding(const Mat3& cam, float time) {
    const float W = (float)render::width(), H = (float)render::height();
    const float gY = groundY();

    const float w = W * 9.0f / 20.0f, h = H * 13.0f / 25.0f;
    const float x = (W - w) / 2.0f;

    const Color stone = rgb(225, 220, 205), stoneShade = rgb(190, 184, 168), line = rgb(60, 55, 45);
    const float stepH = h / 14.0f;
    const int   nSteps = 3;
    const float baseY = gY - nSteps * stepH;

    const float bodyTop = gY - h + h / 5.0f;
    const float bodyH   = baseY - bodyTop;
    const float entH    = h / 28.0f;
    const float pedBase = bodyTop - entH;
    const float apexY   = gY - h;

    // 1. cast shadow of the whole building on the lawn (drawn first so everything covers its root)
    shade(cam, castShadow({ {x, gY}, {x, pedBase}, {x + w / 2, apexY}, {x + w, pedBase}, {x + w, gY} }, gY), 0.26f);

    // 2. steps: each one lighter at the lip, darker toward the riser
    for (int i = 0; i < nSteps; i++) {
        float inset = i * (w / 40.0f);
        float sy = gY - (i + 1) * stepH;
        vbox(cam, x + inset, sy, w - 2 * inset, stepH, i % 2 ? stone : stoneShade, 1.07f, 0.86f);
        seg(cam, {x + inset, sy}, {x + w - inset, sy}, line);
        fill(cam, rectP(x + inset, sy, w - 2 * inset, 1.5f), rgb(250, 247, 238));   // highlight on the edge
    }

    // 3. main wall
    const float wallX = x + w / 20.0f, wallW = w - w / 10.0f;
    grad(cam, rectP(wallX, bodyTop, wallW, bodyH), rgb(236, 231, 218), rgb(196, 189, 173));
    outline(cam, rectP(wallX, bodyTop, wallW, bodyH), line);
    shade(cam, rectP(wallX, bodyTop, wallW, 7.0f), 0.20f);          // shadow the entablature throws on the wall
    shade(cam, rectP(wallX, bodyTop + 7.0f, wallW, 6.0f), 0.09f);

    // 4. entablature and pediment
    grad(cam, rectP(x, pedBase, w, entH), mul(stoneShade, 1.08f), mul(stoneShade, 0.85f));
    outline(cam, rectP(x, pedBase, w, entH), line);

    Poly ped = { {x, pedBase}, {x + w, pedBase}, {x + w / 2, apexY} };
    grad(cam, ped, rgb(238, 233, 221), rgb(190, 184, 168));
    outline(cam, ped, line);
    // recessed triangle inside the pediment, darker, gives depth
    float innerApex = apexY + (pedBase - apexY) * 0.28f;
    Poly tymp = { {x + w * 0.10f, pedBase - 4}, {x + w * 0.90f, pedBase - 4}, {x + w / 2, innerApex} };
    grad(cam, tymp, rgb(190, 184, 168), rgb(214, 208, 194));
    outline(cam, tymp, rgb(150, 144, 130));

    // clock in the pediment: moving hands are rotated about the centre with our own rotateAbout()
    Vec2 c = { x + w / 2, pedBase - h / 11.0f };
    float er = h / 20.0f;
    disc(cam, c, er + 2.5f, rgb(212, 175, 55));                      // gold rim
    disc(cam, c, er, rgb(252, 250, 244));
    ring(cam, c, er, line);
    for (int i = 0; i < 12; i++) {                                   // hour ticks
        float a = i * kPi / 6.0f;
        float r0 = er * (i % 3 == 0 ? 0.72f : 0.83f);
        seg(cam, {c.x + std::sin(a) * r0, c.y - std::cos(a) * r0},
                 {c.x + std::sin(a) * er * 0.93f, c.y - std::cos(a) * er * 0.93f}, line);
    }
    float minuteAngle = time * 0.5f;                                 // 1 rev every ~12.6 s so the motion is visible
    Mat3 minuteRot = rotateAbout(minuteAngle, c);
    Mat3 hourRot   = rotateAbout(minuteAngle / 12.0f, c);
    seg(cam, c, minuteRot * Vec2{c.x, c.y - er * 0.75f}, line);
    seg(cam, c, hourRot   * Vec2{c.x, c.y - er * 0.5f},  line);
    disc(cam, c, 1.6f, line);

    // 5. door, set back in the shade
    float dw = w / 9.0f, dh = bodyH * 2.0f / 3.0f;
    float dx = x + w / 2 - dw / 2, dy = baseY - dh;
    grad(cam, rectP(dx, dy, dw, dh), rgb(92, 60, 40), rgb(56, 34, 22));
    outline(cam, rectP(dx, dy, dw, dh), line);
    seg(cam, {dx + dw / 2, dy}, {dx + dw / 2, baseY}, rgb(35, 22, 14));                       // double door seam
    outline(cam, rectP(dx + dw * 0.12f, dy + dh * 0.10f, dw * 0.30f, dh * 0.35f), rgb(40, 26, 16));
    outline(cam, rectP(dx + dw * 0.58f, dy + dh * 0.10f, dw * 0.30f, dh * 0.35f), rgb(40, 26, 16));
    disc(cam, {dx + dw * 0.44f, dy + dh * 0.58f}, 1.8f, rgb(212, 175, 55));                    // handles
    disc(cam, {dx + dw * 0.56f, dy + dh * 0.58f}, 1.8f, rgb(212, 175, 55));

    // 6. columns, drawn after the door so they stay in front of it. Each is a cylinder:
    //    left half lit -> highlight, right half highlight -> shade. Each throws a soft shadow on the wall.
    const int   nCols = 6;
    const float colW = w / 28.0f, span = w - w / 5.0f;
    for (int i = 0; i < nCols; i++) {
        float cx = x + w / 10.0f + (span - colW) * i / (nCols - 1);
        shade(cam, rectP(cx + colW, bodyTop + 2, colW * 0.9f, bodyH - 2), 0.15f);               // shadow cast to the right
        grad(cam, rectP(cx, bodyTop, colW / 2, bodyH), rgb(232, 228, 216), rgb(255, 254, 250), true);
        grad(cam, rectP(cx + colW / 2, bodyTop, colW / 2, bodyH), rgb(255, 254, 250), rgb(168, 161, 146), true);
        seg(cam, {cx, bodyTop}, {cx, baseY}, line);
        seg(cam, {cx + colW, bodyTop}, {cx + colW, baseY}, line);
        grad(cam, rectP(cx - colW / 4, bodyTop, colW * 1.5f, colW / 2), mul(stoneShade, 1.1f), mul(stoneShade, 0.85f));          // capital
        grad(cam, rectP(cx - colW / 4, baseY - colW / 2, colW * 1.5f, colW / 2), mul(stoneShade, 1.1f), mul(stoneShade, 0.85f)); // base
    }
}

// ------------------------------------------------------------------ flag
void drawFlag(const Mat3& cam, float time) {
    const float gY = groundY();
    const float clothTop = 128.0f, clothW = 88.0f, clothH = clothW * 2.0f / 3.0f;   // 3:2 flag
    const float clothX = kPoleX + 2.0f;                                              // flagWave() expects this
    const float poleTop = clothTop - 12.0f;

    // pole shadow on the lawn
    shade(cam, castShadow(rectP(kPoleX - 2, poleTop, 4, gY - poleTop), gY), 0.22f);

    // pole: metal cylinder (horizontal gradient) + gold ball with highlight
    grad(cam, rectP(kPoleX - 2, poleTop, 2, gY - poleTop), rgb(235, 235, 235), rgb(190, 190, 190), true);
    grad(cam, rectP(kPoleX, poleTop, 2, gY - poleTop), rgb(190, 190, 190), rgb(110, 110, 110), true);
    disc(cam, {kPoleX, poleTop - 3}, 5.0f, rgb(190, 150, 40));
    disc(cam, {kPoleX - 1, poleTop - 4}, 3.6f, rgb(228, 190, 70));
    glow(cam, circleP({kPoleX - 1.8f, poleTop - 5.2f}, 1.6f, 12), rgb(255, 255, 255), 0.7f);

    // cloth bands: black / white / red / white / green  (6:1:6:1:6, same as the CP2 flag)
    const Color black = rgb(0, 0, 0), red = rgb(187, 0, 0), green = rgb(0, 102, 0), white = rgb(255, 255, 255);
    struct Band { float u0, u1; Color c; };
    const Band bands[5] = { {0, 6, black}, {6, 7, white}, {7, 13, red}, {13, 14, white}, {14, 20, green} };
    const float u = clothH / 20.0f;
    const int   nSeg = 11;
    const float segW = clothW / nSeg;

    auto warp = [&](float px, float py) { return flagWave({px, py}, time); };

    // Each band is cut into narrow quads. Brightness of a quad depends on how steeply the wave
    // changes across it, so the cloth looks folded (light on the folds that face the sun).
    for (const Band& b : bands) {
        float y0 = clothTop + b.u0 * u, y1 = clothTop + b.u1 * u;
        for (int s = 0; s < nSeg; s++) {
            float xa = clothX + s * segW, xb = xa + segW;
            Vec2 a0 = warp(xa, y0), b0 = warp(xb, y0), b1 = warp(xb, y1), a1 = warp(xa, y1);
            float slope = b0.y - a0.y;                         // vertical change across this quad
            float f = std::max(0.70f, std::min(1.30f, 1.0f - slope * 0.05f));
            // the cloth also darkens a little toward the free end (further from the light's rim)
            float fade = 1.0f - 0.10f * (float)s / nSeg;
            fill(cam, { a0, b0, b1, a1 }, lit(b.c, f * fade));
        }
    }

    // Maasai shield and crossed spears: every vertex goes through flagWave so they ripple with the cloth
    float cx = clothX + clothW / 2, cy = clothTop + clothH / 2;
    float sw = clothH * 0.26f, sh = clothH * 0.44f;
    float fShield = std::max(0.75f, std::min(1.25f, 1.0f - (warp(cx + 4, cy).y - warp(cx - 4, cy).y) * 0.05f));

    auto warpPoly = [&](const Poly& p) { Poly o; for (const Vec2& v : p) o.push_back(warp(v.x, v.y)); return o; };
    auto spear = [&](Vec2 a, Vec2 b) {                          // subdivided so the spear bends with the wave
        Vec2 prev = warp(a.x, a.y);
        for (int i = 1; i <= 8; i++) {
            float t = i / 8.0f;
            Vec2 cur = warp(a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t);
            seg(cam, prev, cur, white);
            prev = cur;
        }
    };
    spear({cx - sw * 1.6f, cy - sh * 0.95f}, {cx + sw * 1.6f, cy + sh * 0.95f});
    spear({cx + sw * 1.6f, cy - sh * 0.95f}, {cx - sw * 1.6f, cy + sh * 0.95f});

    Poly shield = { {cx, cy - sh}, {cx + sw, cy - sh / 2}, {cx + sw, cy + sh / 4},
                    {cx, cy + sh}, {cx - sw, cy + sh / 4}, {cx - sw, cy - sh / 2} };
    Poly inner  = { {cx, cy - sh * 0.75f}, {cx + sw * 0.6f, cy - sh / 3}, {cx + sw * 0.6f, cy + sh / 5},
                    {cx, cy + sh * 0.75f}, {cx - sw * 0.6f, cy + sh / 5}, {cx - sw * 0.6f, cy - sh / 3} };
    Poly stripe = rectP(cx - sw / 6, cy - sh * 0.75f, sw / 3, sh * 1.5f - sh / 6);

    Poly wShield = warpPoly(shield);
    fill(cam, wShield, lit(red, fShield));
    fill(cam, warpPoly(inner), lit(white, fShield));
    fill(cam, warpPoly(stripe), lit(black, fShield));
    outline(cam, wShield, black);

    // edge of the cloth at the pole: a thin dark seam
    seg(cam, warp(clothX, clothTop), warp(clothX, clothTop + clothH), rgb(30, 30, 30));
}

// ------------------------------------------------------------------ podium
void drawPodium(const Mat3& cam, Vec2 base) {
    const float w = 120.0f, h = 90.0f;
    const float x = base.x, y = base.y;
    const Color dark = rgb(82, 52, 29), edge = rgb(30, 20, 12);

    // cast shadow of the podium
    shade(cam, castShadow({ {x - w / 2, y - h}, {x + w / 2, y - h}, {x + w * 0.4f, y}, {x - w * 0.4f, y} }, y), 0.28f);
    shade(cam, castShadow(circleP({x + w / 6 - 8, y - h - h / 3 - 4}, 5.0f, 12), y), 0.10f);   // mic head

    // body: vertical gradient, then a darker right side (away from the light)
    Poly body = { {x - w / 2, y - h}, {x + w / 2, y - h}, {x + w * 0.4f, y}, {x - w * 0.4f, y} };
    grad(cam, body, rgb(146, 96, 58), rgb(88, 56, 31));
    shade(cam, { {x + w * 0.28f, y - h}, {x + w / 2, y - h}, {x + w * 0.4f, y}, {x + w * 0.18f, y} }, 0.16f);
    outline(cam, body, edge);

    // front panel with a lighter frame
    Poly panel = rectP(x - w / 3, y - h * 0.75f, w * 2.0f / 3.0f, h / 2.0f);
    grad(cam, panel, mul(dark, 1.05f), mul(dark, 0.78f));
    outline(cam, panel, rgb(150, 105, 66));
    // gold emblem on the panel
    Vec2 em = { x, y - h * 0.5f };
    disc(cam, em, 11.0f, rgb(190, 150, 40));
    disc(cam, em, 8.5f, rgb(222, 186, 70));
    ring(cam, em, 5.0f, rgb(150, 112, 28));

    // top slab, lit from the left
    Poly top = { {x - w / 2 - 6, y - h}, {x + w / 2 + 6, y - h}, {x + w / 2, y - h - h / 8}, {x - w / 2, y - h - h / 8} };
    grad(cam, top, rgb(120, 78, 44), rgb(70, 44, 24), true);
    outline(cam, top, edge);
    fill(cam, rectP(x - w / 2, y - h - h / 8, w, 1.5f), rgb(170, 125, 80));      // highlight on the top edge

    // microphone: stand (thick), head, highlight
    Vec2 foot = { x + w / 6, y - h - h / 8 }, headC = { x + w / 6 - 8, y - h - h / 3 - 4 };
    thickLine(cam, foot, {headC.x, headC.y + 4}, 2.5f, rgb(175, 178, 185));   // silver stand, visible against dark trousers
    disc(cam, headC, 5.5f, rgb(60, 62, 70));
    ring(cam, headC, 5.5f, rgb(200, 203, 210));
    glow(cam, circleP({headC.x - 1.6f, headC.y - 1.8f}, 1.8f, 12), rgb(255, 255, 255), 0.55f);
}

// ------------------------------------------------------------------ person
namespace {
    // Local figure space: x right, y UP from the feet (as in CP2). World space: y down.
    struct Xf { Vec2 o; float s; int f; };

    Vec2 P(const Xf& t, float lx, float ly) { return { t.o.x + lx * t.s * t.f, t.o.y - ly * t.s }; }

    Poly local(const Xf& t, std::initializer_list<std::pair<float, float>> pts) {
        Poly v;
        for (const auto& p : pts) v.push_back(P(t, p.first, p.second));
        return v;
    }

    // Body part: horizontal gradient (left lit, right in shade) + outline. Optional extra transform
    // (the raised arm uses it to rotate about the shoulder before the camera is applied).
    void part(const Mat3& cam, const Poly& p, Color c, const Mat3& extra = identity()) {
        Poly q = transformPoly(extra, p);
        grad(cam, q, mul(c, 1.12f), mul(c, 0.80f), true);
        outline(cam, q, kEdge);
    }
}

void drawPerson(const Mat3& cam, Vec2 feet, float scale, const PersonStyle& st, int facing, const Pose& pose, bool shadow) {
    const Xf t{ feet, scale, facing >= 0 ? 1 : -1 };
    const Color shoe = rgb(25, 25, 28);
    const bool back = pose.fromBehind;

    // cast shadow of a simplified silhouette, projected from the feet line
    if (shadow) {
        Poly sil = local(t, { {-24,0},{24,0},{23,80},{42,86},{32,148},{9,158},{17,176},{12,191},{0,194},
                              {-12,191},{-17,176},{-9,158},{-32,148},{-42,86},{-23,80} });
        shade(cam, castShadow(sil, feet.y), 0.26f);
    }

    // legs, with a crease line, then shoes
    part(cam, local(t, { {-16,0},{-16,6},{-3,6},{-3,80},{-18,80},{-22,6},{-22,0} }), st.trousers);
    part(cam, local(t, { {16,0},{16,6},{3,6},{3,80},{18,80},{22,6},{22,0} }),       st.trousers);
    part(cam, local(t, { {-24,0},{-24,7},{-6,7},{-6,0} }), shoe);
    part(cam, local(t, { {24,0},{24,7},{6,7},{6,0} }),     shoe);

    // arms rotate about the shoulders. Positive raise = outward and up, whichever way the figure faces.
    // (rotate() turns clockwise on screen for positive angles, so the screen-right arm needs a negative angle)
    float angRight = -pose.rightArm * t.f;
    float angLeft  = +pose.leftArm  * t.f;
    Mat3 rotR = rotateAbout(angRight, P(t,  26, 147));
    Mat3 rotL = rotateAbout(angLeft,  P(t, -26, 147));
    part(cam, local(t, { {-30,150},{-40,95},{-33,93},{-22,145} }), st.suit, rotL);
    part(cam, local(t, { {30,150},{40,95},{33,93},{22,145} }),     st.suit, rotR);
    part(cam, local(t, { {-41,95},{-33,93},{-34,84},{-42,86} }), st.skin, rotL);   // hands
    part(cam, local(t, { {41,95},{33,93},{34,84},{42,86} }),     st.skin, rotR);

    // torso
    part(cam, local(t, { {-26,150},{26,150},{22,78},{-22,78} }), st.suit);
    if (!back) {
        Poly shirt = local(t, { {-7,150},{7,150},{0,118} });
        grad(cam, shirt, mul(st.shirt, 1.0f), mul(st.shirt, 0.85f));
        outline(cam, shirt, kEdge);
        // lapels
        part(cam, local(t, { {-26,150},{-7,150},{0,118},{-9,108},{-22,140} }), mul(st.suit, 0.85f));
        part(cam, local(t, { {26,150},{7,150},{0,118},{9,108},{22,140} }),     mul(st.suit, 0.85f));
        Poly tie = local(t, { {-3,138},{3,138},{5,100},{0,92},{-5,100} });
        grad(cam, tie, mul(st.tie, 1.1f), mul(st.tie, 0.75f), true);
        outline(cam, tie, kEdge);
    }
    // soft shadow under the chin on the collar
    shade(cam, local(t, { {-10,150},{10,150},{7,143},{-7,143} }), 0.18f);

    // neck
    part(cam, local(t, { {-6,150},{6,150},{6,160},{-6,160} }), mul(st.skin, 0.9f));

    // head: dark disc, lit disc nudged up-left (soft shading), faint highlight
    Vec2 hc = P(t, 0, 176);
    float r = 17.0f * scale;
    Vec2 litC = { hc.x - 1.7f * scale, hc.y - 1.7f * scale };
    disc(cam, hc, r, mul(st.skin, 0.82f));
    disc(cam, litC, r * 0.86f, back ? st.hair : st.skin);
    outline(cam, circleP(hc, r, 32), kEdge);

    // hair: a cap over the top of the head (the whole head when seen from behind)
    if (back) {
        disc(cam, hc, r, st.hair);
        disc(cam, litC, r * 0.86f, mul(st.hair, 1.25f));
        outline(cam, circleP(hc, r, 32), kEdge);
    } else {
        Poly hair;
        for (int i = 0; i <= 8; i++) {                         // outer arc, 12 deg -> 168 deg (local y is up)
            float a = (12.0f + i * 19.5f) * kPi / 180.0f;
            hair.push_back(P(t, 17.6f * std::cos(a), 176.0f + 17.6f * std::sin(a)));
        }
        for (int i = 8; i >= 0; i--) {                         // hairline, flatter, shifted down a little
            float a = (12.0f + i * 19.5f) * kPi / 180.0f;
            hair.push_back(P(t, 17.6f * std::cos(a), 176.0f + 17.6f * 0.55f * std::sin(a) + 1.0f));
        }
        fill(cam, hair, st.hair);

        // eyes with a tiny catch-light, and a smile curved with a parabola
        float er = std::max(1.0f, 2.0f * scale);
        for (float ex : {-6.0f, 6.0f}) {
            disc(cam, P(t, ex, 177), er, kEdge);
            glow(cam, circleP({P(t, ex, 177).x - 0.5f * scale, P(t, ex, 177).y - 0.6f * scale}, 0.6f * scale + 0.3f, 8), rgb(255, 255, 255), 0.8f);
        }
        Vec2 prev = P(t, -5, 170.0f);
        for (int i = 1; i <= 6; i++) {
            float lx = -5.0f + i * (10.0f / 6.0f);
            Vec2 cur = P(t, lx, 168.0f + 2.0f * (lx / 5.0f) * (lx / 5.0f));
            seg(cam, prev, cur, kEdge);
            prev = cur;
        }
    }
}

} // namespace effects
