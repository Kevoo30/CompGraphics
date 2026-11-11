
#ifndef POLYGON_H
#define POLYGON_H

#include <vector>
#include "graphics.h"

// Represents a 2D point
struct PolygonPoint {
    int x;
    int y;
};

// Draw the outline of a polygon using Bresenham's line algorithm
void drawPolygon(const std::vector<PolygonPoint>& vertices);

// Fill a polygon using the Scanline Filling Algorithm
void fillPolygonScanline(const std::vector<PolygonPoint>& vertices);

#endif
