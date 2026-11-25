#pragma once

#include "simulation.h"
#include <string>
#include <vector>

namespace scenario {

struct Event {
    std::string id;
    std::string location;
    std::string place;
    std::string date;
    std::string headline;
    simulation::Mode mode = simulation::Mode::Protest;
    int crowdSize = 40;
    float duration = 15.0f;
    unsigned seed = 1;
    int stations = 3;
    float turnout = 0.65f;
    int yearCard = 0;
    std::vector<simulation::District> districts;
    std::vector<simulation::Barrier> barriers;
};

struct Data {
    std::string label = "Illustrative simulation values";
    std::vector<Event> events;
};

bool load(const std::string& path, Data& out, std::string& error);
simulation::Config simulationConfig(const Event& event);

} // namespace scenario
