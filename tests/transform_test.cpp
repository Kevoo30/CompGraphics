// Unit tests for src/transform.cpp (Person 2). No SDL/OpenGL needed.
// Build and run from the repo root:
//   g++ -std=c++17 -Isrc tests/transform_test.cpp src/transform.cpp -o transform_test && ./transform_test
#include "transform.h"
#include <cmath>
#include <cstdio>

static int failures = 0, checks = 0;
static const float kPi = 3.14159265f;

static bool near(float a, float b) { return std::fabs(a - b) < 1e-4f; }

static void check(const char* name, Vec2 got, Vec2 want) {
    checks++;
    if (near(got.x, want.x) && near(got.y, want.y)) return;
    failures++;
    std::printf("FAIL %-40s got (%g, %g) want (%g, %g)\n", name, got.x, got.y, want.x, want.y);
}

static void check(const char* name, float got, float want) {
    checks++;
    if (near(got, want)) return;
    failures++;
    std::printf("FAIL %-40s got %g want %g\n", name, got, want);
}

static void checkIdentity(const char* name, const Mat3& m) {
    Mat3 I = identity();
    for (int i = 0; i < 9; i++) {
        checks++;
        if (!near(m.m[i], I.m[i])) {
            failures++;
            std::printf("FAIL %-40s element %d = %g\n", name, i, m.m[i]);
            return;
        }
    }
}

int main() {
    Vec2 p = {3, 4};

    check("identity",                  identity() * p, p);
    check("translate",                 translate(10, -2) * p, {13, 2});
    check("rotate 90",                 rotate(kPi / 2) * Vec2{1, 0}, {0, 1});
    check("rotate 180",                rotate(kPi) * p, {-3, -4});
    check("scale",                     scale(2, 0.5f) * p, {6, 2});
    check("shear x",                   shear(2, 0) * p, {11, 4});
    check("shear y",                   shear(0, 3) * p, {3, 13});
    check("shear both",                shear(1, 1) * p, {7, 7});
    check("reflectX",                  reflectX() * p, {3, -4});
    check("reflectY",                  reflectY() * p, {-3, 4});
    check("reflectOrigin",             reflectOrigin() * p, {-3, -4});

    Vec2 pivot = {5, 5};
    check("rotateAbout keeps pivot",   rotateAbout(1.234f, pivot) * pivot, pivot);
    check("rotateAbout 90",            rotateAbout(kPi / 2, pivot) * Vec2{6, 5}, {5, 6});
    check("scaleAbout keeps pivot",    scaleAbout(3, 2, pivot) * pivot, pivot);
    check("scaleAbout",                scaleAbout(2, 2, pivot) * Vec2{6, 7}, {7, 9});
    check("shearAbout keeps pivot",    shearAbout(0.7f, 0.2f, pivot) * pivot, pivot);

    check("reflectAcross y=x",         reflectAcross({0, 0}, {1, 1}) * p, {4, 3});
    check("reflectAcross x-axis",      reflectAcross({-1, 0}, {1, 0}) * p, {3, -4});
    check("reflectAcross y=10",        reflectAcross({0, 10}, {5, 10}) * p, {3, 16});
    check("reflectAcross x=1",         reflectAcross({1, 0}, {1, 7}) * p, {-1, 4});
    check("reflectAcross point on line", reflectAcross({0, 2}, {4, 6}) * Vec2{2, 4}, {2, 4});
    check("reflectAcross degenerate",  reflectAcross(p, p) * Vec2{1, 2}, {1, 2});
    checkIdentity("reflectAcross twice",
                  reflectAcross({1, 2}, {4, -3}) * reflectAcross({1, 2}, {4, -3}));

    // order of composition: a * b applies b first
    check("T*R (rotate then move)",    (translate(10, 0) * rotate(kPi / 2)) * Vec2{1, 0}, {10, 1});
    check("R*T (move then rotate)",    (rotate(kPi / 2) * translate(10, 0)) * Vec2{1, 0}, {0, 11});

    check("det rotate",                determinant(rotate(0.8f)), 1);
    check("det scale",                 determinant(scale(2, 3)), 6);
    check("det shear",                 determinant(shear(0.5f, 0)), 1);
    check("det reflect",               determinant(reflectX()), -1);
    check("det reflectAcross",         determinant(reflectAcross({0, 0}, {2, 1})), -1);

    Mat3 M = translate(40, -7) * rotate(0.6f) * shear(0.3f, -0.2f) * scale(2, 0.5f);
    checkIdentity("inverse(M) * M",    inverse(M) * M);
    checkIdentity("M * inverse(M)",    M * inverse(M));
    check("inverse round trip",        inverse(M) * (M * p), p);
    checkIdentity("inverse singular",  inverse(scale(0, 1)));

    std::vector<Vec2> poly = transformPoly(translate(1, 1), {{0, 0}, {2, 0}, {2, 3}});
    checks++;
    if (poly.size() != 3) { failures++; std::printf("FAIL transformPoly size\n"); }
    else check("transformPoly", poly[2], {3, 4});

    std::printf("%d/%d checks passed\n", checks - failures, checks);
    return failures == 0 ? 0 : 1;
}
