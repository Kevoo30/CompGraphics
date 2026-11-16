#pragma once

// ---- Person 4 owns this file. ----
// Call pollInput() once per frame, after render::pollQuit().
// The original fields keep their names and meaning. New fields were APPENDED, so code that only
// uses the old ones still compiles. Fields marked "edge" are true only on the frame the key goes down.
struct Input {
    bool  pause;        // SPACE   (edge)  pause / resume the animation
    bool  gesture;      // G or right mouse click (edge)  speaker gestures, crowd cheers
    bool  screenshot;   // P       (edge)  save screenshot
    float camDx, camDy; // arrow keys: -1, 0 or 1 (held)
    float zoom;         // Q = zoom out (-1), E = zoom in (+1) (held)

    // ---- added by Person 4 ----
    bool  applause;       // A   (edge)  toggle sustained crowd applause
    bool  switchRenderer; // TAB (edge)  switch software <-> OpenGL  (Person 1 acts on this in main.cpp)
    bool  resetCamera;    // R   (edge)  back to the default view
    float mouseDx, mouseDy; // pixels the mouse moved this frame WHILE the left button is held (drag to pan)
    float wheel;          // mouse wheel notches this frame (+ = scroll up = zoom in)
    int   mouseX, mouseY; // cursor position in window pixels
    bool  click;          // left button went down this frame (edge)
};
Input pollInput();
