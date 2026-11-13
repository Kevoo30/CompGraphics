#pragma once
#include "transform.h"

// ---- Person 4 owns this file. Signatures are agreed: do not change them. ----
// Draws the audience. `cam` is the camera matrix: apply it to every point (see scene_gl.cpp).
void drawCrowd(float time, const Mat3& cam);