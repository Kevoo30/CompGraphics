#include "animation.h"
#include <cmath>
#include <algorithm>

// Working placeholders so the project animates on Day 1. Person 3: replace with your own system
// (flag wave that is fixed at the pole, cloud drift, speaker arm gesture, camera moves, timelines).

void updateAnimations(float dt) { (void)dt; }


Vec2 flagWave(Vec2 base, float time) {
    // Distance from the flagpole
    float distance = std::max(0.0f, base.x - 702.0f);

    // The wave becomes larger farther from the pole
    float amplitude = distance * 0.12f;

    // Controls wave speed and wavelength
    float phase = distance * 0.08f - time * 4.0f;

    // Calculate the new vertical position
    return {
        base.x,
        base.y + amplitude * std::sin(phase)
    };
}


float cloudPosition(float startX, float time, float speed, float screenWidth) {
    float distance = time * speed;
    float position = std::fmod(startX + distance, screenWidth + 100.0f);

    // Keep the cloud's position within the wrapping range
    if (position < 0.0f) {
        position += screenWidth + 100.0f;
    }

    return position - 50.0f;
}


float speakerArmAngle(float time) {
    // A complete raise-and-lower cycle takes 4 seconds
    float cycle = std::fmod(time, 4.0f) / 2.0f;

    // Reverse the second half to lower the arm
    if (cycle > 1.0f) {
        cycle = 2.0f - cycle;
    }

    // Smooth the movement
    float movement = easeInOut(cycle);

    // Move between -10 and -65 degrees
    return -10.0f - 55.0f * movement;
}




float easeInOut(float t) {
    if (t < 0) t = 0;
    if (t > 1) t = 1;
    return t * t * (3.0f - 2.0f * t);
}