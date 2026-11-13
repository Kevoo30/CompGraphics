#include "scene_gl.h"
#include "render.h"
#include "transform.h"
#include "animation.h"
#include "crowd.h"
#include <cmath>

// Placeholder scene proving the whole pipeline works: transform -> render -> animation -> input.
// Person 5: replace this with the real building, flag, podium and person ported from scene.cpp,
// add gradients, shadows and the Kenyan flag shield.

namespace {
    float camX = 0, camY = 0, zoomLevel = 1.0f;
    bool  paused = false;
    float sceneTime = 0;

    typedef std::vector<Vec2> Poly;
    Poly rect(float x, float y, float w, float h) { return {{x, y}, {x + w, y}, {x + w, y + h}, {x, y + h}}; }
}

void drawScene(float time, const Input& in) {
    (void)time;
    float dt = render::deltaTime();
    if (in.pause) paused = !paused;
    if (!paused) sceneTime += dt;
    camX += in.camDx * 200.0f * dt;
    camY += in.camDy * 200.0f * dt;
    zoomLevel *= 1.0f + in.zoom * 1.0f * dt;

    // Camera = move to centre, zoom, move back (all with our own Mat3 maths)
    Mat3 cam = translate(400, 300) * scale(zoomLevel, zoomLevel) * translate(-400 - camX, -300 - camY);
    auto T = [&](const Poly& p) { return transformPoly(cam, p); };

    // Sky and ground
    render::fillPolygonGradient(T(rect(-2000, -2000, 4800, 2400)), {70, 130, 200}, {200, 225, 245});
    render::fillPolygonGradient(T(rect(-2000, 400, 4800, 2000)), {90, 120, 70}, {50, 75, 45});

    // Building
    render::fillPolygonGradient(T(rect(150, 380, 500, 20)), {200, 198, 190}, {150, 148, 140});
    render::fillPolygonGradient(T(rect(170, 200, 460, 180)), {235, 230, 218}, {195, 188, 172});
    for (int i = 0; i < 6; i++)
        render::fillPolygonGradient(T(rect(190.0f + i * 72, 200, 26, 180)), {250, 247, 238}, {170, 163, 150}, true);
    render::fillPolygonGradient(T({{150, 200}, {650, 200}, {400, 120}}), {225, 220, 208}, {170, 165, 152});
    render::drawPolygon(T({{150, 200}, {650, 200}, {400, 120}}), {120, 115, 105});
    render::fillCircle(cam * Vec2{400, 170}, 18 * zoomLevel, {190, 150, 60});

    // Flagpole + waving flag (each stripe is a strip whose vertices are displaced by flagWave)
    render::drawLine(cam * Vec2{700, 150}, cam * Vec2{700, 400}, {80, 80, 80});
    const Color top[3] = {{20, 20, 20}, {200, 30, 30}, {30, 140, 60}};
    const Color bot[3] = {{0, 0, 0},    {140, 15, 15}, {15, 90, 35}};
    for (int s = 0; s < 3; s++) {
        Poly strip;
        float y0 = 155.0f + s * 20, y1 = y0 + 20;
        for (int x = 0; x <= 88; x += 8) strip.push_back(flagWave({702.0f + x, y0}, sceneTime));
        for (int x = 88; x >= 0; x -= 8)  strip.push_back(flagWave({702.0f + x, y1}, sceneTime));
        render::fillPolygonGradient(T(strip), top[s], bot[s], true);
    }

    // Shadow under the building
    render::fillPolygonAlpha(T({{160, 400}, {660, 400}, {760, 470}, {260, 470}}), {0, 0, 0}, 0.28f);

    // Audience in front
    drawCrowd(sceneTime, cam);
}