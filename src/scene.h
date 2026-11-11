#pragma once
#include "graphics.h"
// Person 5 - scene assets for "The People Are Watching"

// Colours for one character.
struct PersonStyle { Color skin, suit, shirt, tie, trousers; };

// ---- Called from main.cpp (same names and arguments as the original stubs) ----
void drawBuilding();            // ground + government building, sized from the window
void drawFlag(int x, int y);    // flagpole stands at x; Kenyan flag hangs to its right, top edge at y
void drawPodium(int x, int y);  // x = centre, y = bottom of the podium (ground level)
void drawPerson(int x, int y);  // x = centre, y = feet; default presenter at scale 1.0 (~190 px tall)

// ---- Extra, for later checkpoints (animation, several characters) ----
// scale: 1.0 = normal size. facing: +1 faces right, -1 mirrors the figure to face left.
void drawPerson(int x, int y, float scale, const PersonStyle& style, int facing);
