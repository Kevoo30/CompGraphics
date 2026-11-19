#include "scene_gl.h"
#include "render.h"
#include "transform.h"
#include "animation.h"
#include "crowd.h"
#include "effects.h"
#include <algorithm>
#include <cmath>

// OpenGL scene for "The People Are Watching" (Person 5).
// Pipeline: input -> camera (Mat3 from transform.*) -> effects:: assets -> animation.* + crowd.*

namespace {
    float camX = 0, camY = 0, zoomLevel = 1.0f;
    bool  paused = false;
    float sceneTime = 0;
    float gestureStart = -100.0f;        // sceneTime at which G was last pressed

    const float kDeg = 3.14159265f / 180.0f;

    // Speaker's raised-arm angle (radians, 0 = down, positive = raised).
    // Idle: Person 3's speakerArmAngle() cycle. G key: a bigger, eased "wave to the crowd" on top of it.
    float speakerRaise(float t) {
        float idle = -speakerArmAngle(t) * kDeg;                 // speakerArmAngle() returns -10..-65 degrees
        float k = (t - gestureStart) / 2.5f;                     // gesture lasts 2.5 s
        if (k > 0.0f && k < 1.0f) {
            float bump = easeInOut(1.0f - std::fabs(2.0f * k - 1.0f));   // up then down
            return idle + (125.0f * kDeg - idle) * bump;
        }
        return idle;
    }
}

void drawScene(float time, const Input& in) {
    (void)time;
    float dt = render::deltaTime();
    if (in.pause) paused = !paused;
    if (!paused) sceneTime += dt;
    if (in.gesture) { gestureStart = sceneTime; cheer(sceneTime, 3.5f); }   // speaker waves, crowd cheers
    if (in.applause) setApplause(!isApplauding());
    if (in.resetCamera) { camX = camY = 0; zoomLevel = 1.0f; }

    camX += in.camDx * 200.0f * dt / zoomLevel;
    camY += in.camDy * 200.0f * dt / zoomLevel;
    camX -= in.mouseDx / zoomLevel;                          // dragging moves the world with the cursor
    camY -= in.mouseDy / zoomLevel;
    zoomLevel *= (1.0f + in.zoom * 1.0f * dt) * std::pow(1.1f, in.wheel);
    zoomLevel = std::max(0.5f, std::min(3.0f, zoomLevel));

    // Camera = move to centre, zoom, move back (all with our own Mat3 maths)
    Mat3 cam = translate(400, 300) * scale(zoomLevel, zoomLevel) * translate(-400 - camX, -300 - camY);

    // Back to front
    effects::drawSky(cam, sceneTime);
    effects::drawGround(cam);
    drawAnimatedClouds(sceneTime);                     // Person 3 (drawn in screen space, so clouds ignore the camera)
    effects::drawBuilding(cam, sceneTime);
    effects::drawFlag(cam, sceneTime);

    // Speaker: stands on the top step behind the podium (same positions as the CP2 scene)
    PersonStyle presenter{ {141, 85, 36}, {40, 40, 90}, {255, 255, 255}, {190, 0, 0}, {30, 30, 60} };
    effects::Pose pose;
    pose.rightArm = speakerRaise(sceneTime);
    const float kSpeakerScale = 0.47f;   // crowd rows are 0.38 / 0.47 / 0.56
    effects::drawPerson(cam, {400, 380}, kSpeakerScale, presenter, 1, pose);
    effects::drawPodium(cam, {400, 450});

    // Audience in front (Person 4)
    drawCrowd(sceneTime, cam);
}