#pragma once
#include "transform.h"

// ---- Person 3 owns this file. Signatures are agreed: do not change them. ----
void  updateAnimations(float dt);          // called once per frame by main
Vec2  flagWave(Vec2 base, float time);     // returns the waved position of a flag vertex
float easeInOut(float t);                  // t in 0..1 -> smooth 0..1