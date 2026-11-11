#pragma once
#include "graphics.h"   // Point, Color, gfx::putPixel
#include <vector>

// Agreed interface (Person 4). Do not change these signatures:
// scene.cpp and main.cpp call them exactly like this.
void drawPolygon(const std::vector<Point>& vertices, Color c);
void fillPolygonScanline(const std::vector<Point>& vertices, Color c);