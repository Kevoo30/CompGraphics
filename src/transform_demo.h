#pragma once
#include "transform.h"

// ---- Person 2 owns this file. ----
// Standalone transformations demo (run: people_are_watching.exe --transform).
//   Top band : a star that moves, rotates and scales  (M = T * R * S), with a motion trail and its
//              local axes. Hover the mouse over it: picking uses inverse(M) to go screen -> local.
//   Panel 1  : shear            (shearAbout, grid lines shear with the square)
//   Panel 2  : reflectX / reflectY / reflectOrigin of an "F" about the panel centre
//   Panel 3  : reflectAcross a rotating mirror line
//   Panel 4  : rotateAbout + scaleAbout on a two-part arm (shoulder -> elbow), composed hierarchically
void drawTransformDemo(float time, Vec2 mouse);
