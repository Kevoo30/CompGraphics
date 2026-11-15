#include "animation.h"
#include "render.h"
#include <cmath>
#include <algorithm>


// Working placeholders so the project animates on Day 1. // Person 3: flag waving, cloud movement and speaker arm animation.
// (flag wave that is fixed at the pole, cloud drift, speaker arm gesture, camera moves, timelines).

void updateAnimations(float dt) {
    (void)dt;
}

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


void drawAnimatedClouds(float time) {
    const Color cloudColor = {245, 248, 255};
    const float screenWidth = static_cast<float>(render::width());

    // Three clouds with different starting positions and speeds
    const float starts[] = {80.0f, 350.0f, 650.0f};
    const float heights[] = {85.0f, 125.0f, 75.0f};
    const float speeds[] = {25.0f, 35.0f, 20.0f};

    for (int i = 0; i < 3; i++) {
        float x = cloudPosition(
            starts[i], time, speeds[i], screenWidth
        );

        float y = heights[i];

        // Three overlapping circles form one fluffy cloud
        render::fillCircle({x, y,}, 18.0f, cloudColor);
        render::fillCircle({x + 20.0f, y - 10.0f}, 23.0f, cloudColor);
        render::fillCircle({x + 43.0f, y}, 17.0f, cloudColor);
    }
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


void drawAnimatedSpeaker(float time, const Mat3& cam) {
    // Speaker position
    Vec2 shoulder{385.0f, 470.0f};

    // Calculate the arm angle and convert degrees to radians
    float angle = speakerArmAngle(time);
    float radians = angle * 3.14159265f / 180.0f;

    // Draw the animated arm around the shoulder pivot
    std::vector<Vec2> raisedArm = {
        {379, 470},
        {391, 470},
        {397, 420},
        {385, 418}
    };

    Mat3 armTransform = cam * rotateAbout(radians, shoulder);

    render::fillPolygon(
        transformPoly(armTransform, raisedArm),
        {141, 85, 36}
    );

    // Draw the other stationary arm
    render::fillPolygon(
        transformPoly(cam, {
            {409, 465}, {421, 470},
            {435, 505}, {424, 510}
        }),
        {141, 85, 36}
    );

    // Draw torso
    render::fillPolygon(
        transformPoly(cam, {
            {380, 465}, {420, 465},
            {430, 530}, {370, 530}
        }),
        {30, 70, 130}
    );

    // Draw head
    render::fillCircle(
        cam * Vec2{400, 445},
        17.0f,
        {141, 85, 36}
    );
}
