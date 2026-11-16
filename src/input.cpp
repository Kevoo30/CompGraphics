#include "input.h"
#include <SDL.h>
#include <cstdio>

// Keyboard and mouse controls:
//   SPACE pause | arrows move camera | Q/E zoom | R reset camera | G (or right click) speaker gesture
//   A toggle applause | P screenshot | TAB switch renderer | left-drag pan | wheel zoom | ESC quit

namespace {
    float wheelAccum = 0;

    // The render loop pumps SDL events itself (render::pollQuit), so the wheel event is gone before we
    // could read it. An event watch sees every event as it is queued, whoever polls it later.
    int SDLCALL wheelWatch(void*, SDL_Event* e) {
        if (e->type == SDL_MOUSEWHEEL) wheelAccum += (e->wheel.direction == SDL_MOUSEWHEEL_FLIPPED ? -1.0f : 1.0f) * e->wheel.y;
        return 1;
    }

    // Edge detector: true only on the frame the key changes from up to down.
    struct Edge {
        bool prev = false;
        bool operator()(bool now) { bool r = now && !prev; prev = now; return r; }
    };
}

Input pollInput() {
    static bool first = true;
    static Edge pause, gesture, shot, applause, sw, reset, click, rclick;
    static int lastX = 0, lastY = 0;
    static bool wasDown = false;

    if (first) {
        first = false;
        SDL_AddEventWatch(wheelWatch, nullptr);
        std::printf("Controls: SPACE pause | arrows pan | Q/E or wheel zoom | left-drag pan | R reset\n"
                    "          G / right-click gesture | A applause | P screenshot | TAB switch renderer | ESC quit\n");
    }

    const Uint8* k = SDL_GetKeyboardState(nullptr);
    Input in{};

    in.pause          = pause(k[SDL_SCANCODE_SPACE] != 0);
    in.screenshot     = shot(k[SDL_SCANCODE_P] != 0);
    in.applause       = applause(k[SDL_SCANCODE_A] != 0);
    in.switchRenderer = sw(k[SDL_SCANCODE_TAB] != 0);
    in.resetCamera    = reset(k[SDL_SCANCODE_R] != 0);

    in.camDx = float(k[SDL_SCANCODE_RIGHT]) - float(k[SDL_SCANCODE_LEFT]);
    in.camDy = float(k[SDL_SCANCODE_DOWN])  - float(k[SDL_SCANCODE_UP]);
    in.zoom  = float(k[SDL_SCANCODE_E])     - float(k[SDL_SCANCODE_Q]);

    // mouse
    int mx = 0, my = 0;
    Uint32 buttons = SDL_GetMouseState(&mx, &my);
    bool left  = (buttons & SDL_BUTTON(SDL_BUTTON_LEFT))  != 0;
    bool right = (buttons & SDL_BUTTON(SDL_BUTTON_RIGHT)) != 0;
    in.mouseX = mx; in.mouseY = my;
    in.click  = click(left);
    if (left && wasDown) { in.mouseDx = float(mx - lastX); in.mouseDy = float(my - lastY); }   // drag only
    wasDown = left; lastX = mx; lastY = my;

    in.wheel = wheelAccum; wheelAccum = 0;

    in.gesture = gesture(k[SDL_SCANCODE_G] != 0) | rclick(right);
    return in;
}
