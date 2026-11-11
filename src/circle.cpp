#include "circle.h"

// ---------------------------------------------------------------------------
// Midpoint Circle Algorithm
//
// Uses 8-way symmetry: for each step in the first octant (x >= y >= 0),
// all eight reflected points are plotted.
//
// Decision parameter initialisation:
//   p0 = 1 - r   (derived from the circle equation F(x,y) = x²+y²-r²)
//
// Each iteration:
//   y is incremented by 1; if p < 0  -> midpoint is inside the circle,
//     move to (x, y+1), update p += 2y+1
//   if p >= 0 -> midpoint is outside, move to (x-1, y+1),
//     update p += 2y - 2x + 1
// ---------------------------------------------------------------------------

static void plotCirclePoints(int cx, int cy, int x, int y, Color c) {
    // 8 symmetric points
    gfx::putPixel(cx + x, cy + y, c);
    gfx::putPixel(cx - x, cy + y, c);
    gfx::putPixel(cx + x, cy - y, c);
    gfx::putPixel(cx - x, cy - y, c);
    gfx::putPixel(cx + y, cy + x, c);
    gfx::putPixel(cx - y, cy + x, c);
    gfx::putPixel(cx + y, cy - x, c);
    gfx::putPixel(cx - y, cy - x, c);
}

void drawCircleMidpoint(int cx, int cy, int r, Color c) {
    if (r < 0) return;
    if (r == 0) { gfx::putPixel(cx, cy, c); return; }

    int x = r, y = 0;
    int p = 1 - r;   // initial decision parameter

    plotCirclePoints(cx, cy, x, y, c);

    while (x > y) {
        ++y;
        if (p < 0) {
            p += 2 * y + 1;
        } else {
            --x;
            p += 2 * y - 2 * x + 1;
        }
        plotCirclePoints(cx, cy, x, y, c);
    }
}

// ---------------------------------------------------------------------------
// Filled circle
//
// Same midpoint loop, but instead of plotting 8 isolated points we draw
// horizontal spans between the symmetric x boundaries at every y level.
// Two spans are covered per (x,y) pair in the first octant:
//   • horizontal span at  cy+y  from cx-x to cx+x
//   • horizontal span at  cy-y  from cx-x to cx+x
//   • horizontal span at  cy+x  from cx-y to cx+y
//   • horizontal span at  cy-x  from cx-y to cx+y
// ---------------------------------------------------------------------------

static void hline(int x0, int x1, int y, Color c) {
    if (x0 > x1) { int t = x0; x0 = x1; x1 = t; }
    for (int x = x0; x <= x1; ++x) gfx::putPixel(x, y, c);
}

void fillCircleMidpoint(int cx, int cy, int r, Color c) {
    if (r < 0) return;
    if (r == 0) { gfx::putPixel(cx, cy, c); return; }

    int x = r, y = 0;
    int p = 1 - r;

    // Initial spans at y=0: horizontal diameter
    hline(cx - x, cx + x, cy + y, c);
    hline(cx - y, cx + y, cy + x, c);
    hline(cx - y, cx + y, cy - x, c);

    while (x > y) {
        ++y;
        if (p < 0) {
            p += 2 * y + 1;
        } else {
            --x;
            p += 2 * y - 2 * x + 1;
        }
        hline(cx - x, cx + x, cy + y, c);
        hline(cx - x, cx + x, cy - y, c);
        hline(cx - y, cx + y, cy + x, c);
        hline(cx - y, cx + y, cy - x, c);
    }
}
