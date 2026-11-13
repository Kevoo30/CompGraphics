#include "animation.h"
#include <cmath>

// Working placeholders so the project animates on Day 1. Person 3: replace with your own system
// (flag wave that is fixed at the pole, cloud drift, speaker arm gesture, camera moves, timelines).

void updateAnimations(float dt) { (void)dt; }

Vec2 flagWave(Vec2 base, float time) {
    return { base.x, base.y + 3.0f * std::sin(0.10f * base.x + 4.0f * time) };
}

float easeInOut(float t) {
    if (t < 0) t = 0;
    if (t > 1) t = 1;
    return t * t * (3.0f - 2.0f * t);
}