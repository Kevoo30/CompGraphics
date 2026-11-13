#include "input.h"
#include <SDL.h>

// Working placeholder. Person 4: add mouse controls, more keys, applause trigger, etc.
Input pollInput() {
    static bool prevPause = false, prevGesture = false, prevShot = false;
    const Uint8* k = SDL_GetKeyboardState(nullptr);

    bool p = k[SDL_SCANCODE_SPACE] != 0, g = k[SDL_SCANCODE_G] != 0, s = k[SDL_SCANCODE_P] != 0;
    Input in{};
    in.pause      = p && !prevPause;
    in.gesture    = g && !prevGesture;
    in.screenshot = s && !prevShot;
    prevPause = p; prevGesture = g; prevShot = s;

    in.camDx = float(k[SDL_SCANCODE_RIGHT]) - float(k[SDL_SCANCODE_LEFT]);
    in.camDy = float(k[SDL_SCANCODE_DOWN])  - float(k[SDL_SCANCODE_UP]);
    in.zoom  = float(k[SDL_SCANCODE_E])     - float(k[SDL_SCANCODE_Q]);
    return in;
}