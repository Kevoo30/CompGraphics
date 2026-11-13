#pragma once
#include <vector>

// ---- Person 2 owns this file. Signatures are agreed: do not change them. ----

struct Vec2 { float x, y; };

// 3x3 matrix for 2D homogeneous coordinates, ROW-major:
//   | m[0] m[1] m[2] |   | a  b  tx |
//   | m[3] m[4] m[5] | = | c  d  ty |
//   | m[6] m[7] m[8] |   | 0  0  1  |
struct Mat3 { float m[9]; };

Mat3 identity();
Mat3 translate(float tx, float ty);
Mat3 rotate(float radians);                    // counter-clockwise on a y-up plane; clockwise on screen (y-down)
Mat3 scale(float sx, float sy);
Mat3 shear(float shx, float shy);              // TODO Person 2
Mat3 reflectX();                               // TODO Person 2 (flip across the x-axis)
Mat3 reflectY();                               // TODO Person 2 (flip across the y-axis)
Mat3 rotateAbout(float radians, Vec2 pivot);   // rotate around any point (e.g. an arm around the shoulder)

Mat3 operator*(const Mat3& a, const Mat3& b);  // a * b: b is applied first, then a
Vec2 operator*(const Mat3& m, Vec2 v);
std::vector<Vec2> transformPoly(const Mat3& m, const std::vector<Vec2>& pts);