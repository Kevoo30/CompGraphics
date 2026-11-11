#include "polygon.h"
#include "graphics.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <vector>

// Draw a line using Bresenham's algorithm.
// This is temporary until Person 2's line functions are integrated.
static void drawPolygonLine(int x1, int y1, int x2, int y2, Color c) {
    int dx = std::abs(x2 - x1);
    int dy = std::abs(y2 - y1);

    int sx = (x1 < x2) ? 1 : -1;
    int sy = (y1 < y2) ? 1 : -1;

    int error = dx - dy;

    while (true) {
        gfx::putPixel(x1, y1, c);

        if (x1 == x2 && y1 == y2)
            break;

        int e2 = 2 * error;

        if (e2 > -dy) {
            error -= dy;
            x1 += sx;
        }

        if (e2 < dx) {
            error += dx;
            y1 += sy;
        }
    }
}

// Draw polygon outline
void drawPolygon(const std::vector<Point>& vertices, Color c) {
    int n = vertices.size();

    if (n < 2)
        return;

    for (int i = 0; i < n; i++) {
        int next = (i + 1) % n;

        drawPolygonLine(
            vertices[i].x,
            vertices[i].y,
            vertices[next].x,
            vertices[next].y,
            c
        );
    }
}

// Fill polygon using Scanline Filling Algorithm
void fillPolygonScanline(const std::vector<Point>& vertices, Color c) {
    int n = vertices.size();

    if (n < 3)
        return;

    int minY = vertices[0].y;
    int maxY = vertices[0].y;

    for (const auto& point : vertices) {
        minY = std::min(minY, point.y);
        maxY = std::max(maxY, point.y);
    }

    // Process each horizontal scanline
    for (int y = minY; y < maxY; y++) {
        std::vector<double> intersections;

        // Find intersections between scanline and polygon edges
        for (int i = 0; i < n; i++) {
            Point p1 = vertices[i];
            Point p2 = vertices[(i + 1) % n];

            // Ignore horizontal edges and use half-open intervals
            if (p1.y == p2.y)
                continue;

            if (y < std::min(p1.y, p2.y) ||
                y >= std::max(p1.y, p2.y))
                continue;

            double x = p1.x +
                (static_cast<double>(y - p1.y) *
                 (p2.x - p1.x)) /
                (p2.y - p1.y);

            intersections.push_back(x);
        }

        // Sort intersections from left to right
        std::sort(intersections.begin(), intersections.end());

        // Fill between pairs of intersections
        for (size_t i = 0; i + 1 < intersections.size(); i += 2) {
            int startX = static_cast<int>(
                std::ceil(intersections[i])
            );

            int endX = static_cast<int>(
                std::floor(intersections[i + 1])
            );

            for (int x = startX; x <= endX; x++) {
                gfx::putPixel(x, y, c);
            }
        }
    }
}