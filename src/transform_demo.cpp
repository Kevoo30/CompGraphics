#include "transform_demo.h"
#include "render.h"
#include "animation.h"   // easeInOut() from Person 3
#include <cmath>
#include <vector>

// Transformations demo (Person 2). Every shape is defined once in local coordinates and placed on
// screen only through Mat3 matrices from transform.cpp.

namespace {
    typedef std::vector<Vec2> Poly;
    const float kPi = 3.14159265f;

    const Color kInk     = {40, 44, 60};
    const Color kGhost   = {170, 175, 190};
    const Color kAxis    = {205, 208, 220};
    const Color kPanel   = {252, 252, 255};
    const Color kBlue    = {52, 110, 220};
    const Color kRed     = {220, 70, 60};
    const Color kGreen   = {40, 160, 90};
    const Color kOrange  = {240, 150, 30};
    const Color kPurple  = {130, 80, 200};

    // ---------------------------------------------------------------- shapes (local coordinates)

    Poly star(float rOuter, float rInner, int points = 5) {
        Poly p;
        for (int i = 0; i < points * 2; i++) {
            float r = (i % 2 == 0) ? rOuter : rInner;
            float a = -kPi / 2 + i * kPi / points;
            p.push_back({r * std::cos(a), r * std::sin(a)});
        }
        return p;
    }

    Poly rect(float x, float y, float w, float h) { return {{x, y}, {x + w, y}, {x + w, y + h}, {x, y + h}}; }

    // Letter "F" in the upper-right quadrant (screen y grows downward). It has no symmetry,
    // so every reflection is easy to tell apart.
    Poly letterF() {
        return {{10, -70}, {55, -70}, {55, -58}, {22, -58}, {22, -46}, {45, -46},
                {45, -36}, {22, -36}, {22, -10}, {10, -10}};
    }

    // ---------------------------------------------------------------- helpers

    float pingPong(float t) { float f = std::fmod(t, 2.0f); if (f < 0) f += 2.0f; return f < 1.0f ? f : 2.0f - f; }

    bool pointInPoly(Vec2 p, const Poly& poly) {            // even-odd ray casting
        bool inside = false;
        for (size_t i = 0, j = poly.size() - 1; i < poly.size(); j = i++) {
            const Vec2 &a = poly[i], &b = poly[j];
            if ((a.y > p.y) != (b.y > p.y) && p.x < (b.x - a.x) * (p.y - a.y) / (b.y - a.y) + a.x)
                inside = !inside;
        }
        return inside;
    }

    void panel(float x, float y, float w, float h) {
        Poly r = rect(x, y, w, h);
        render::fillPolygon(r, kPanel);
        render::drawPolygon(r, kGhost);
    }

    void axes(Vec2 c, float hw, float hh) {
        render::drawLine({c.x - hw, c.y}, {c.x + hw, c.y}, kAxis);
        render::drawLine({c.x, c.y - hh}, {c.x, c.y + hh}, kAxis);
    }

    void shape(const Mat3& m, const Poly& local, Color c) {
        Poly p = transformPoly(m, local);
        render::fillPolygon(p, c);
        render::drawPolygon(p, kInk);
    }

    // ---------------------------------------------------------------- top band: T * R * S

    Mat3 starPose(float t) {
        float u = easeInOut(pingPong(t / 4.0f));                // eased back-and-forth, 4 s each way
        float x = 110.0f + 580.0f * u;
        float y = 150.0f + 45.0f * std::sin(t * 2.0f);
        float angle = t * 1.5f;
        float s = 1.0f + 0.35f * std::sin(t * 3.0f);
        return translate(x, y) * rotate(angle) * scale(s, s);  // scale, then rotate, then move
    }

    void compositeDemo(float t, Vec2 mouse) {
        panel(20, 20, 760, 260);
        const Poly s = star(55, 24);

        // ghost of the untransformed shape at its rest position (identity * translate only)
        render::drawPolygon(transformPoly(translate(110, 150), s), kGhost);

        // motion trail: the same matrix evaluated at earlier times
        for (int i = 6; i >= 1; i--)
            render::fillPolygonAlpha(transformPoly(starPose(t - i * 0.1f), s), kBlue, 0.05f * (7 - i));

        Mat3 M = starPose(t);
        bool hover = pointInPoly(inverse(M) * mouse, s);        // screen -> local with the inverse
        Poly p = transformPoly(M, s);
        if (hover) render::fillPolygonAlpha(transformPoly(M * scale(1.25f, 1.25f), s), kOrange, 0.35f);
        render::fillPolygonGradient(p, hover ? Color{255, 210, 80} : Color{110, 160, 255},
                                       hover ? kOrange : kBlue);
        render::drawPolygon(p, kInk);

        // the shape's own local x (red) and y (green) axes, carried along by M
        Vec2 o = M * Vec2{0, 0};
        render::drawLine(o, M * Vec2{75, 0}, kRed);
        render::drawLine(o, M * Vec2{0, 75}, kGreen);
        render::fillCircle(o, 4, kInk);
    }

    // ---------------------------------------------------------------- panel 1: shear

    void shearDemo(float t, Vec2 c) {
        float shx = 0.6f * std::sin(t * 1.4f);
        float shy = 0.3f * std::sin(t * 0.9f);
        Mat3 place = translate(c.x, c.y);
        Mat3 M = shearAbout(shx, shy, c) * place;

        const Poly sq = rect(-45, -45, 90, 90);
        render::drawPolygon(transformPoly(place, sq), kGhost);  // original square
        shape(M, sq, kPurple);
        for (int k = -1; k <= 1; k++) {                         // grid lines shear with the square
            float v = k * 30.0f;
            render::drawLine(M * Vec2{v, -45}, M * Vec2{v, 45}, kPanel);
            render::drawLine(M * Vec2{-45, v}, M * Vec2{45, v}, kPanel);
        }
    }

    // ---------------------------------------------------------------- panel 2: reflect about the axes

    void reflectDemo(float t, Vec2 c) {
        axes(c, 85, 130);
        Mat3 wobble = rotateAbout(0.25f * std::sin(t * 1.3f), {32, -40});   // original moves; mirrors follow
        Mat3 toPanel = translate(c.x, c.y);
        const Poly F = letterF();
        shape(toPanel * reflectOrigin() * wobble, F, kGhost);
        shape(toPanel * reflectX()      * wobble, F, kOrange);  // below the x-axis
        shape(toPanel * reflectY()      * wobble, F, kGreen);   // left of the y-axis
        shape(toPanel                   * wobble, F, kBlue);    // original
    }

    // ---------------------------------------------------------------- panel 3: mirror across any line

    void mirrorDemo(float t, Vec2 c) {
        float a = t * 0.6f;
        Vec2 dir = {std::cos(a), std::sin(a)};
        Vec2 p0 = {c.x - 85 * dir.x, c.y - 85 * dir.y}, p1 = {c.x + 85 * dir.x, c.y + 85 * dir.y};

        Mat3 original = translate(c.x, c.y) * scale(0.9f, 0.9f);
        Mat3 mirror   = reflectAcross(p0, p1);
        const Poly F = letterF();

        // construction lines: each vertex joined to its image (they cross the mirror at right angles)
        for (size_t i = 0; i < F.size(); i += 3) {
            Vec2 v = original * F[i];
            render::drawLine(v, mirror * v, kAxis);
        }
        render::drawLine(p0, p1, kRed);
        shape(original, F, kBlue);
        shape(mirror * original, F, kOrange);
    }

    // ---------------------------------------------------------------- panel 4: pivots and hierarchy

    void pivotDemo(float t, Vec2 c) {
        Vec2 shoulder = {c.x - 50, c.y - 40};
        Vec2 elbow    = {shoulder.x + 55, shoulder.y};
        const Poly upper = rect(shoulder.x, shoulder.y - 8, 55, 16);
        const Poly fore  = rect(elbow.x, elbow.y - 6, 45, 12);

        // rest pose
        render::drawPolygon(upper, kGhost);
        render::drawPolygon(fore, kGhost);

        float s  = 1.0f + 0.15f * std::sin(t * 2.5f);
        float a1 = 0.4f + 0.9f * std::sin(t * 1.2f);            // shoulder swing
        float a2 = 0.5f + 0.8f * std::sin(t * 2.1f);            // elbow bend, relative to the upper arm
        Mat3 upperM = rotateAbout(a1, shoulder) * scaleAbout(s, s, shoulder);
        Mat3 foreM  = upperM * rotateAbout(a2, elbow);          // child inherits the parent's transform

        shape(foreM, fore, kGreen);
        shape(upperM, upper, kBlue);
        render::fillCircle(foreM * Vec2{elbow.x + 45, elbow.y}, 9, kOrange);   // hand
        render::fillCircle(upperM * elbow, 4, kInk);
        render::fillCircle(shoulder, 5, kRed);                  // the fixed pivot
    }
}

void drawTransformDemo(float time, Vec2 mouse) {
    render::fillPolygonGradient(rect(0, 0, (float)render::width(), (float)render::height()),
                                {232, 236, 246}, {210, 216, 232});
    compositeDemo(time, mouse);

    void (*demos[4])(float, Vec2) = {shearDemo, reflectDemo, mirrorDemo, pivotDemo};
    for (int i = 0; i < 4; i++) {
        float x = 20.0f + i * 192.0f, y = 300.0f, w = 184.0f, h = 280.0f;
        panel(x, y, w, h);
        demos[i](time, {x + w / 2, y + h / 2});
    }
}
