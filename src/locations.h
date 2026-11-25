#pragma once
#include "transform.h"

// The places the film travels through. Each is drawn procedurally with our own polygons, gradients and circles.
enum class Loc { COURT, BOMAS, STREET, PARLIAMENT, STATE_HOUSE, STADIUM, HOMESTEAD, BALLOT };

struct Mood {
    int   sky;          // 0 dawn, 1 day, 2 harsh midday, 3 dusk, 4 night, 5 pre-dawn
    int   crowd;        // how many people are in view
    bool  placards;     // signs held up
    bool  fists;        // raised arms
    bool  candles;      // vigil
    bool  mourning;     // dark clothing
    bool  halfMast;     // flag at half-mast
    float energy;       // 0 calm ... 1 surging
    int   seed;         // changes buildings / people so repeated places don't look identical
    const char* banner = nullptr;   // text on a cloth banner carried by marchers
    bool  smoke = false;            // tear-gas haze drifting over the crowd
    bool  police = false;           // a line of police with shields in the distance
};

// t = seconds since this shot started (drives all the motion). M = camera / slide transform for this shot.
void drawLocation(Loc loc, float t, const Mood& mood, const Mat3& M);