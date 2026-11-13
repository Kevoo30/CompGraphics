#pragma once

// ---- Person 4 owns this file. Signatures are agreed: do not change them. ----
// Call pollInput() once per frame, after render::pollQuit().
struct Input {
    bool  pause;        // SPACE   (true only on the frame it is pressed)
    bool  gesture;      // G       (speaker gesture)
    bool  screenshot;   // P       (save screenshot)
    float camDx, camDy; // arrow keys: -1, 0 or 1 (held)
    float zoom;         // Q = zoom out (-1), E = zoom in (+1) (held)
};
Input pollInput();