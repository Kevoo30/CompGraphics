#pragma once

#include "locations.h"
#include "simulation.h"

// Perspective, lit scene pass used by the documentary timeline.
namespace film3d {
    void drawLocation(Loc loc, float time, const Mood& mood, const simulation::Simulation* simulation = nullptr);
}
