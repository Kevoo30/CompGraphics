#pragma once
#include <vector>

// ---- Person 2 owns this file. Signatures are agreed: do not change them. ----
// All 2D transformations are written by hand as 3x3 matrices in homogeneous coordinates.
// A point (x, y) is treated as the column vector (x, y, 1), so  p' = M * p.
// Combine with *, right to left:  M = T * R * S  means "scale, then rotate, then translate".

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
Mat3 shear(float shx, float shy);              // x' = x + shx*y,  y' = y + shy*x
Mat3 reflectX();                               // flip across the x-axis:  (x, y) -> ( x, -y)
Mat3 reflectY();                               // flip across the y-axis:  (x, y) -> (-x,  y)
Mat3 rotateAbout(float radians, Vec2 pivot);   // rotate around any point (e.g. an arm around the shoulder)

Mat3 operator*(const Mat3& a, const Mat3& b);  // a * b: b is applied first, then a
Vec2 operator*(const Mat3& m, Vec2 v);
std::vector<Vec2> transformPoly(const Mat3& m, const std::vector<Vec2>& pts);

// ---- Extras added by Person 2 (new functions only; the agreed ones above are unchanged) ----
Mat3  reflectOrigin();                         // (x, y) -> (-x, -y)  (same as rotating by 180 degrees)
Mat3  reflectAcross(Vec2 a, Vec2 b);           // mirror across the line through a and b (identity if a == b)
Mat3  scaleAbout(float sx, float sy, Vec2 pivot);
Mat3  shearAbout(float shx, float shy, Vec2 pivot);
float determinant(const Mat3& m);              // < 0 means the transform flips orientation (a reflection)
Mat3  inverse(const Mat3& m);                  // undoes m (identity if m is not invertible); e.g. screen -> local for mouse picking
