#pragma once
#include "graphics.h"    // Color
#include "transform.h"   // Vec2
#include <vector>

// ---- Person 1 (Lead) owns this file. Signatures are agreed: do not change them. ----
// OpenGL renderer. Coordinates are in pixels, origin top-left, y grows downward
// (same as the CP2 software renderer), so scene code ports straight over.

namespace render {
    void  init(int w, int h, const char* title);
    void  shutdown();

    bool  pollQuit();                  // true when window closed or ESC pressed; also pumps input events
    void  beginFrame(Color clear);     // clears the screen, updates deltaTime
    void  endFrame();                  // swaps buffers
    float deltaTime();                 // seconds since previous frame
    float time();                      // seconds since init
    int   width();
    int   height();

    // Filled shapes. Concave polygons are fine (triangulated internally).
    void fillPolygon(const std::vector<Vec2>& pts, Color c);
    // horizontal = false: c1 at the top, c2 at the bottom. horizontal = true: c1 at the left, c2 at the right.
    void fillPolygonGradient(const std::vector<Vec2>& pts, Color c1, Color c2, bool horizontal = false);
    // Semi-transparent fill. alpha 0..1 (shadows: black, 0.3).
    void fillPolygonAlpha(const std::vector<Vec2>& pts, Color c, float alpha);
    void fillCircle(Vec2 center, float r, Color c);

    // Outlines
    void drawPolygon(const std::vector<Vec2>& pts, Color c);
    void drawLine(Vec2 a, Vec2 b, Color c);
    void drawCircle(Vec2 center, float r, Color c);

    void saveScreenshot(const char* path);   // BMP of the current frame
}