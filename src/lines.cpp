
#include "lines.h"
#include <algorithm>
#include <cmath>
#include <cstdlib>

// =====================================================
// DDA LINE DRAWING ALGORITHM
// Supports all slopes and all directions.
// =====================================================

void drawLineDDA(int x0, int y0, int x1, int y1, Color c)
{
    // Calculate horizontal and vertical differences
    double dx = static_cast<double>(x1) - x0;
    double dy = static_cast<double>(y1) - y0;

    // Determine the number of steps
    double steps = std::max(std::abs(dx), std::abs(dy));

    // Handle a line consisting of only one point
    if (steps == 0)
    {
        gfx::putPixel(x0, y0, c);
        return;
    }

    // Calculate increments for each step
    double xIncrement = dx / steps;
    double yIncrement = dy / steps;

    // Start at the first point
    double x = x0;
    double y = y0;

    // Plot each pixel along the line
    for (int i = 0; i <= static_cast<int>(steps); i++)
    {
        gfx::putPixel(
            static_cast<int>(std::lround(x)),
            static_cast<int>(std::lround(y)),
            c
        );

        x += xIncrement;
        y += yIncrement;
    }
}


// =====================================================
// BRESENHAM LINE DRAWING ALGORITHM
// Uses integer arithmetic and handles all 8 octants.
// =====================================================

void drawLineBresenham(int x0, int y0, int x1, int y1, Color c)
{
    // Calculate absolute differences
    long long dx = std::abs(static_cast<long long>(x1) - x0);
    long long dy = std::abs(static_cast<long long>(y1) - y0);

    // Determine the direction of movement
    int sx = (x0 < x1) ? 1 : -1;
    int sy = (y0 < y1) ? 1 : -1;

    // Initial error value
    long long error = dx - dy;

    while (true)
    {
        // Plot the current pixel
        gfx::putPixel(x0, y0, c);

        // Stop when the endpoint is reached
        if (x0 == x1 && y0 == y1)
        {
            break;
        }

        // Calculate twice the error
        long long e2 = 2 * error;

        // Decide whether to move horizontally
        if (e2 > -dy)
        {
            error -= dy;
            x0 += sx;
        }

        // Decide whether to move vertically
        if (e2 < dx)
        {
            error += dx;
            y0 += sy;
        }
    }
}
