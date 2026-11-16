#include "transform.h"
#include <cmath>

// 2D transformations (Person 2). Pure maths: no OpenGL, no SDL, so it is shared by every renderer.
// Every matrix is written out element by element so the maths is visible.

// ---------------------------------------------------------------- basic matrices

Mat3 identity() {
    return {{1, 0, 0,
             0, 1, 0,
             0, 0, 1}};
}

Mat3 translate(float tx, float ty) {
    return {{1, 0, tx,
             0, 1, ty,
             0, 0, 1}};
}

// x' = x cos r - y sin r,  y' = x sin r + y cos r
Mat3 rotate(float r) {
    float c = std::cos(r), s = std::sin(r);
    return {{c, -s, 0,
             s,  c, 0,
             0,  0, 1}};
}

Mat3 scale(float sx, float sy) {
    return {{sx, 0,  0,
             0,  sy, 0,
             0,  0,  1}};
}

// x' = x + shx * y   (horizontal shear: rows slide sideways in proportion to their height)
// y' = y + shy * x   (vertical shear: columns slide up/down in proportion to their x)
Mat3 shear(float shx, float shy) {
    return {{1,   shx, 0,
             shy, 1,   0,
             0,   0,   1}};
}

Mat3 reflectX() {
    return {{1,  0, 0,
             0, -1, 0,
             0,  0, 1}};
}

Mat3 reflectY() {
    return {{-1, 0, 0,
              0, 1, 0,
              0, 0, 1}};
}

Mat3 reflectOrigin() {
    return {{-1,  0, 0,
              0, -1, 0,
              0,  0, 1}};
}

// ---------------------------------------------------------------- about a pivot / line
// Pattern: move the pivot to the origin, do the basic transform, move back.
//   M = T(p) * X * T(-p)      (read right to left)

Mat3 rotateAbout(float r, Vec2 p) {
    return translate(p.x, p.y) * rotate(r) * translate(-p.x, -p.y);
}

Mat3 scaleAbout(float sx, float sy, Vec2 p) {
    return translate(p.x, p.y) * scale(sx, sy) * translate(-p.x, -p.y);
}

Mat3 shearAbout(float shx, float shy, Vec2 p) {
    return translate(p.x, p.y) * shear(shx, shy) * translate(-p.x, -p.y);
}

// Mirror across an arbitrary line through a and b:
//   1. translate a to the origin       2. rotate the line onto the x-axis
//   3. reflect across the x-axis       4. undo step 2      5. undo step 1
Mat3 reflectAcross(Vec2 a, Vec2 b) {
    float dx = b.x - a.x, dy = b.y - a.y;
    if (dx == 0.0f && dy == 0.0f) return identity();
    float angle = std::atan2(dy, dx);
    return translate(a.x, a.y) * rotate(angle) * reflectX() * rotate(-angle) * translate(-a.x, -a.y);
}

// ---------------------------------------------------------------- matrix operations

Mat3 operator*(const Mat3& a, const Mat3& b) {
    Mat3 r{};
    for (int i = 0; i < 3; i++)
        for (int j = 0; j < 3; j++) {
            float sum = 0;
            for (int k = 0; k < 3; k++) sum += a.m[i * 3 + k] * b.m[k * 3 + j];
            r.m[i * 3 + j] = sum;
        }
    return r;
}

// (x, y, 1) -> (x', y', 1). The bottom row is always (0 0 1) for our affine matrices.
Vec2 operator*(const Mat3& m, Vec2 v) {
    return { m.m[0] * v.x + m.m[1] * v.y + m.m[2],
             m.m[3] * v.x + m.m[4] * v.y + m.m[5] };
}

std::vector<Vec2> transformPoly(const Mat3& m, const std::vector<Vec2>& pts) {
    std::vector<Vec2> out;
    out.reserve(pts.size());
    for (const Vec2& p : pts) out.push_back(m * p);
    return out;
}

// Cofactor expansion along the first row.
float determinant(const Mat3& a) {
    const float* m = a.m;
    return m[0] * (m[4] * m[8] - m[5] * m[7])
         - m[1] * (m[3] * m[8] - m[5] * m[6])
         + m[2] * (m[3] * m[7] - m[4] * m[6]);
}

// inverse = adjugate / determinant (adjugate = transpose of the cofactor matrix).
Mat3 inverse(const Mat3& a) {
    float d = determinant(a);
    if (std::fabs(d) < 1e-8f) return identity();
    const float* m = a.m;
    float k = 1.0f / d;
    return {{ (m[4] * m[8] - m[5] * m[7]) * k, (m[2] * m[7] - m[1] * m[8]) * k, (m[1] * m[5] - m[2] * m[4]) * k,
              (m[5] * m[6] - m[3] * m[8]) * k, (m[0] * m[8] - m[2] * m[6]) * k, (m[2] * m[3] - m[0] * m[5]) * k,
              (m[3] * m[7] - m[4] * m[6]) * k, (m[1] * m[6] - m[0] * m[7]) * k, (m[0] * m[4] - m[1] * m[3]) * k }};
}
