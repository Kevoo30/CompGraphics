#include "scene.h"
#include "lines.h"
#include "circle.h"
#include "polygon.h"
#include <algorithm>
#include <cmath>
#include <initializer_list>
#include <utility>
#include <vector>

namespace {

Color rgb(int r, int g, int b) { return Color{ (uint8_t)r, (uint8_t)g, (uint8_t)b }; }

// The ground line everything stands on (440 in an 800x600 window).
int groundY() { return gfx::height() * 11 / 15; }

void fillRect(int x, int y, int w, int h, Color c) {
    fillPolygonScanline({ {x, y}, {x + w, y}, {x + w, y + h}, {x, y + h} }, c);
}
void outlineRect(int x, int y, int w, int h, Color c) {
    drawPolygon({ {x, y}, {x + w, y}, {x + w, y + h}, {x, y + h} }, c);
}

// ---------------------------------------------------------------- Kenyan flag
// Bands black / red / green separated by thin white bands (6:1:6:1:6),
// with the Maasai shield and two crossed spears in the middle.
void flagCloth(int x, int y, int w) {
    const int h = w * 2 / 3;
    const float u = h / 20.0f;
    const Color black = rgb(0, 0, 0), red = rgb(187, 0, 0), green = rgb(0, 102, 0), white = rgb(255, 255, 255);

    auto band = [&](float u0, float u1, Color c) {
        int y0 = y + (int)std::lround(u0 * u), y1 = y + (int)std::lround(u1 * u);
        fillRect(x, y0, w, y1 - y0, c);
    };
    band(0, 6, black);
    band(6, 7, white);
    band(7, 13, red);
    band(13, 14, white);
    band(14, 20, green);

    int cx = x + w / 2, cy = y + h / 2;
    int sw = (int)(h * 0.26f), sh = (int)(h * 0.44f);

    // crossed spears behind the shield
    drawLineBresenham(cx - sw * 2, cy - sh, cx + sw * 2, cy + sh, white);
    drawLineBresenham(cx + sw * 2, cy - sh, cx - sw * 2, cy + sh, white);

    // shield
    std::vector<Point> shield = {
        {cx, cy - sh}, {cx + sw, cy - sh / 2}, {cx + sw, cy + sh / 4},
        {cx, cy + sh}, {cx - sw, cy + sh / 4}, {cx - sw, cy - sh / 2} };
    fillPolygonScanline(shield, red);
    std::vector<Point> inner = {
        {cx, cy - sh * 3 / 4}, {cx + sw * 3 / 5, cy - sh / 3}, {cx + sw * 3 / 5, cy + sh / 5},
        {cx, cy + sh * 3 / 4}, {cx - sw * 3 / 5, cy + sh / 5}, {cx - sw * 3 / 5, cy - sh / 3} };
    fillPolygonScanline(inner, white);
    fillRect(cx - sw / 6, cy - sh * 3 / 4, sw / 3, sh * 3 / 2 - sh / 6, black);
    drawPolygon(shield, black);
}

// ----------------------------------------------------------- person (polygons)
struct Xf { int ox, oy; float s; int f; };

// Local coordinates: x right, y UP from the feet. Converted to screen space.
Point P(const Xf& t, float lx, float ly) {
    return { t.ox + (int)std::lround(lx * t.s * t.f), t.oy - (int)std::lround(ly * t.s) };
}

void poly(const Xf& t, std::initializer_list<std::pair<float, float>> pts, Color fill, Color edge) {
    std::vector<Point> v;
    for (const auto& p : pts) v.push_back(P(t, p.first, p.second));
    fillPolygonScanline(v, fill);
    drawPolygon(v, edge);
}

} // namespace

// ================================================================ public API

void drawBuilding() {
    const int W = gfx::width(), H = gfx::height();
    const int gY = groundY();

    // ground + pavement strip
    fillRect(0, gY, W, H - gY, rgb(95, 130, 85));
    fillRect(0, gY, W, 26, rgb(150, 150, 150));

    // building, centred
    const int w = W * 9 / 20, h = H * 13 / 25;
    const int x = (W - w) / 2;

    const Color stone = rgb(225, 220, 205), shade = rgb(190, 184, 168), line = rgb(60, 55, 45);
    const int stepH = h / 14, nSteps = 3;
    const int baseY = gY - nSteps * stepH;

    // steps
    for (int i = 0; i < nSteps; ++i) {
        int inset = i * (w / 40);
        int sy = gY - (i + 1) * stepH;
        fillRect(x + inset, sy, w - 2 * inset, stepH, i % 2 ? stone : shade);
        drawLineBresenham(x + inset, sy, x + w - inset, sy, line);
    }

    // main wall
    const int bodyTop = gY - h + h / 5;
    const int bodyH = baseY - bodyTop;
    fillRect(x + w / 20, bodyTop, w - w / 10, bodyH, stone);
    outlineRect(x + w / 20, bodyTop, w - w / 10, bodyH, line);

    // entablature + pediment
    fillRect(x, bodyTop - h / 28, w, h / 28, shade);
    std::vector<Point> ped = { {x, bodyTop - h / 28}, {x + w, bodyTop - h / 28}, {x + w / 2, gY - h} };
    fillPolygonScanline(ped, stone);
    drawPolygon(ped, line);

    // clock / emblem in the pediment (circle algorithm)
    int ecx = x + w / 2, ecy = bodyTop - h / 28 - h / 11, er = h / 20;
    fillCircleMidpoint(ecx, ecy, er, rgb(250, 250, 245));
    drawCircleMidpoint(ecx, ecy, er, line);
    drawLineBresenham(ecx, ecy, ecx, ecy - er * 2 / 3, line);   // minute hand
    drawLineBresenham(ecx, ecy, ecx + er / 2, ecy, line);       // hour hand

    // door
    int dw = w / 9, dh = bodyH * 2 / 3;
    fillRect(x + w / 2 - dw / 2, baseY - dh, dw, dh, rgb(70, 45, 30));
    outlineRect(x + w / 2 - dw / 2, baseY - dh, dw, dh, line);

    // columns (drawn after the door so they stay in front of it)
    const int nCols = 6, colW = w / 28, span = w - w / 5;
    for (int i = 0; i < nCols; ++i) {
        int cx = x + w / 10 + (span - colW) * i / (nCols - 1);
        fillRect(cx, bodyTop, colW, bodyH, rgb(245, 242, 232));
        fillRect(cx - colW / 4, bodyTop, colW + colW / 2, colW / 2, shade);
        fillRect(cx - colW / 4, baseY - colW / 2, colW + colW / 2, colW / 2, shade);
        drawLineBresenham(cx, bodyTop, cx, baseY, line);
        drawLineBresenham(cx + colW, bodyTop, cx + colW, baseY, line);
    }
}

void drawFlag(int x, int y) {
    const int gY = groundY();
    // pole, from just above the flag down to the ground
    fillRect(x - 2, y - 10, 4, gY - (y - 10), rgb(190, 190, 190));
    fillCircleMidpoint(x, y - 13, 5, rgb(212, 175, 55));
    // cloth 120 px wide (80 tall), attached to the pole
    flagCloth(x + 2, y, 120);
}

void drawPodium(int x, int y) {
    const int w = 120, h = 90;
    const Color wood = rgb(120, 78, 45), dark = rgb(80, 50, 28), line = rgb(30, 20, 12);

    std::vector<Point> body = { {x - w / 2, y - h}, {x + w / 2, y - h},
                                {x + w * 2 / 5, y}, {x - w * 2 / 5, y} };
    fillPolygonScanline(body, wood);
    drawPolygon(body, line);

    std::vector<Point> top = { {x - w / 2 - 6, y - h}, {x + w / 2 + 6, y - h},
                               {x + w / 2, y - h - h / 8}, {x - w / 2, y - h - h / 8} };
    fillPolygonScanline(top, dark);
    drawPolygon(top, line);

    fillRect(x - w / 3, y - h * 3 / 4, w * 2 / 3, h / 2, dark);

    // microphone
    drawLineBresenham(x + w / 6, y - h - h / 8, x + w / 6 - 8, y - h - h / 3, rgb(40, 40, 40));
    fillCircleMidpoint(x + w / 6 - 8, y - h - h / 3 - 4, 5, rgb(30, 30, 30));
}

void drawPerson(int x, int y) {
    PersonStyle presenter{ rgb(141, 85, 36), rgb(40, 40, 90), rgb(255, 255, 255), rgb(190, 0, 0), rgb(30, 30, 60) };
    drawPerson(x, y, 1.0f, presenter, 1);
}

void drawPerson(int x, int y, float scale, const PersonStyle& st, int facing) {
    Xf t{ x, y, scale, facing >= 0 ? 1 : -1 };
    const Color edge = rgb(20, 20, 20), shoe = rgb(25, 25, 25);

    // legs
    poly(t, { {-16,0},{-16,6},{-3,6},{-3,80},{-18,80},{-22,6},{-22,0} }, st.trousers, edge);
    poly(t, { {16,0},{16,6},{3,6},{3,80},{18,80},{22,6},{22,0} },       st.trousers, edge);
    // shoes
    poly(t, { {-24,0},{-24,7},{-6,7},{-6,0} }, shoe, edge);
    poly(t, { {24,0},{24,7},{6,7},{6,0} },     shoe, edge);
    // arms + hands
    poly(t, { {-30,150},{-40,95},{-33,93},{-22,145} }, st.suit, edge);
    poly(t, { {30,150},{40,95},{33,93},{22,145} },     st.suit, edge);
    poly(t, { {-41,95},{-33,93},{-34,84},{-42,86} }, st.skin, edge);
    poly(t, { {41,95},{33,93},{34,84},{42,86} },     st.skin, edge);
    // torso, shirt V, tie
    poly(t, { {-26,150},{26,150},{22,78},{-22,78} }, st.suit, edge);
    poly(t, { {-7,150},{7,150},{0,118} }, st.shirt, edge);
    poly(t, { {-3,138},{3,138},{5,100},{0,92},{-5,100} }, st.tie, edge);
    // neck
    poly(t, { {-6,150},{6,150},{6,160},{-6,160} }, st.skin, edge);

    // head (circle algorithm) + eyes + mouth
    Point h = P(t, 0, 176);
    int r = (int)std::lround(17 * scale);
    fillCircleMidpoint(h.x, h.y, r, st.skin);
    drawCircleMidpoint(h.x, h.y, r, edge);
    Point e1 = P(t, -6, 179), e2 = P(t, 6, 179);
    int er = std::max(1, (int)std::lround(2 * scale));
    fillCircleMidpoint(e1.x, e1.y, er, edge);
    fillCircleMidpoint(e2.x, e2.y, er, edge);
    Point m1 = P(t, -5, 169), m2 = P(t, 5, 169);
    drawLineBresenham(m1.x, m1.y, m2.x, m2.y, edge);
}
