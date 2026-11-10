#pragma once
#include <cstdint>
#include <vector>

struct Color { uint8_t r, g, b; };
struct Point { int x, y; };

namespace gfx {
    void init(int w, int h, const char* title);
    void putPixel(int x, int y, Color c);   // out-of-bounds pixels are ignored
    void clear(Color c);
    void present();
    bool pollQuit();                        // true when window closed or ESC pressed
    bool keyPressed(int sdlKeycode);        // optional: for switching demo screens
    void saveBMP(const char* path);         // pure C++ BMP writer (screenshots)
    void shutdown();
    int  width();
    int  height();
}
