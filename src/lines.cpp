#include "lines.h"
#include <cmath>
#include <algorithm>

// TODO (Person 2): DDA must handle all slopes; Bresenham must handle all 8 octants.
// Temporary stub so the project builds: DDA below is a working basic version.
void drawLineDDA(int x0, int y0, int x1, int y1, Color c) {
    int dx = x1 - x0, dy = y1 - y0;
    int steps = std::max(std::abs(dx), std::abs(dy));
    if (steps == 0) { gfx::putPixel(x0, y0, c); return; }
    float xi = dx / (float)steps, yi = dy / (float)steps, x = x0, y = y0;
    for (int i = 0; i <= steps; i++) {
        gfx::putPixel((int)std::lround(x), (int)std::lround(y), c);
        x += xi; y += yi;
    }
}

void drawLineBresenham(int x0, int y0, int x1, int y1, Color c) {
    // TODO (Person 2): replace with your own integer-only Bresenham
    int dx = std::abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
    int dy = -std::abs(y1 - y0), sy = y0 < y1 ? 1 : -1;
    int err = dx + dy;
    while (true) {
        gfx::putPixel(x0, y0, c);
        if (x0 == x1 && y0 == y1) break;
        int e2 = 2 * err;
        if (e2 >= dy) { err += dy; x0 += sx; }
        if (e2 <= dx) { err += dx; y0 += sy; }
    }
}
