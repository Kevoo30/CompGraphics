#pragma once
#include "transform.h"
#include "scene.h"       // PersonStyle

// ---- Person 5 owns this file. ----
// OpenGL versions of the scene assets (building, flag, podium, person) with gradients,
// soft shading and cast shadows. Everything is drawn through render:: and the camera matrix,
// so panning / zooming works. All coordinates are world pixels (y grows downward, 800x600 window).
//
// `cam` is the same camera matrix scene_gl.cpp builds and passes to drawCrowd().

namespace effects {

    // Where the lawn meets the building steps (440 in an 800x600 window, same as the CP2 scene).
    float groundY();

    // Pose for a person. Angles are in radians, 0 = arm hanging down, positive = raised outward.
    struct Pose {
        float rightArm = 0;        // the arm on the figure's own right side (screen right when facing = +1)
        float leftArm  = 0;
        bool  fromBehind = false;  // true: no face / tie / shirt, hair covers the head (audience seen from the back)
    };

    // Backdrop
    void drawSky(const Mat3& cam, float time);      // gradient sky + sun with glow
    void drawGround(const Mat3& cam);               // lawn gradient + pavement strip

    // Scene objects
    void drawBuilding(const Mat3& cam, float time); // steps, columns, pediment, clock (hands move with time) + cast shadow
    void drawFlag(const Mat3& cam, float time);     // pole at x = 700, Kenyan flag waving via flagWave(), cloth shaded by folds
    void drawPodium(const Mat3& cam, Vec2 base);    // base = bottom centre of the podium

    // Person: feet = point between the feet. scale 1.0 is about 195 px tall. facing +1 / -1 mirrors the figure.
    // Person 4: call this for every spectator (change scale, style, pose.rightArm/leftArm for applause).
    void drawPerson(const Mat3& cam, Vec2 feet, float scale, const PersonStyle& style,
                    int facing, const Pose& pose = Pose{}, bool shadow = true);
}
