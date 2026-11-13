#include "transform.h"
#include <cmath>

// Working basics so the project builds and animates on Day 1.
// Person 2: review, extend (shear, reflect, inverse...), and add your standalone demo.

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

Mat3 shear(float, float)  { return identity(); }   // TODO Person 2
Mat3 reflectX()           { return identity(); }   // TODO Person 2
Mat3 reflectY()           { return identity(); }   // TODO Person 2

Mat3 rotateAbout(float r, Vec2 p) {
    return translate(p.x, p.y) * rotate(r) * translate(-p.x, -p.y);
}

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