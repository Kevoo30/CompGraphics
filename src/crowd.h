#pragma once
#include "transform.h"

// ---- Person 4 owns this file. ----
// Draws the audience. `cam` is the camera matrix: apply it to every point (see scene_gl.cpp).
// The spectators are the SAME person model as the speaker (effects::drawPerson), seen from behind.
void drawCrowd(float time, const Mat3& cam);

// Applause control (call from the scene when input arrives; drawCrowd eases in and out by itself).
void setApplause(bool on);               // sustained applause on / off
bool isApplauding();                     // true only for sustained applause (not a short cheer)
void cheer(float time, float seconds);   // short burst of applause, e.g. when the speaker gestures
