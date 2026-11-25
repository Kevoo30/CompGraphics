#include "scenario.h"
#include "simulation.h"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstdio>
#include <filesystem>
#include <fstream>

namespace {
void advance(simulation::Simulation& simulation, float seconds) {
    constexpr float dt = 1.0f / 30.0f;
    while (seconds > 0.0f) {
        const float step = std::min(dt, seconds);
        simulation.step(step);
        seconds -= step;
    }
}
}

int main() {
    simulation::Config voting;
    voting.mode = simulation::Mode::Voting;
    voting.agents = 36;
    voting.turnout = 1.0f;
    voting.stations = 3;
    voting.seed = 42;
    voting.districts = {{"North", 1.0f, 1000}, {"South", 1.0f, 800}};
    simulation::Simulation queue(voting);
    assert(queue.agents().size() == 36);
    advance(queue, 50.0f);
    assert(queue.ballotsCast() > 0);
    assert(queue.ballotsCast() + queue.gaveUpCount() <= 36);
    assert(queue.tallies()[0].cast + queue.tallies()[1].cast == queue.ballotsCast());

    voting.agents = 50;
    voting.stations = 1;
    simulation::Simulation longQueue(voting);
    advance(longQueue, 40.0f);
    assert(longQueue.gaveUpCount() > 0);

    simulation::Config protest;
    protest.mode = simulation::Mode::Protest;
    protest.agents = 40;
    protest.seed = 8;
    simulation::Simulation rally(protest);
    advance(rally, 4.0f);
    bool hasRallyingAgent = false;
    for (const auto& agent : rally.agents()) hasRallyingAgent |= agent.state == simulation::State::Rallying;
    assert(hasRallyingAgent);

    simulation::Config audience;
    audience.mode = simulation::Mode::Audience;
    audience.agents = 18;
    simulation::Simulation listeners(audience);
    const auto openingPositions = listeners.agents();
    advance(listeners, 8.0f);
    for (std::size_t i = 0; i < listeners.agents().size(); ++i) {
        const auto& person = listeners.agents()[i];
        assert(person.state == simulation::State::Listening);
        assert(std::fabs(person.position.x - openingPositions[i].position.x) < .001f);
        assert(std::fabs(person.position.y - openingPositions[i].position.y) < .001f);
        assert(person.heading == 180.0f);
    }

    const auto fixture = std::filesystem::temp_directory_path() / "compgraphics_scenario_test.json";
    {
        std::ofstream out(fixture);
        out << R"({"label":"test","events":[{"id":"vote","type":"voting","location":"ballot","place":"POLLING STATION","date":"TEST DATE","headline":"TEST EVENT","year_card":2024,"crowd_size":12,"duration":9,"stations":2,"barriers":[{"x":1,"y":2,"radius":0.5}],"districts":[{"name":"A","turnout":0.7,"expected_voters":100,"map_x":0.2,"map_y":0.8}]},{"id":"hearing","type":"audience","location":"court","crowd_size":8,"duration":9}]})";
    }
    scenario::Data loaded;
    std::string error;
    assert(scenario::load(fixture.string(), loaded, error));
    assert(loaded.events.size() == 2);
    assert(loaded.events[0].mode == simulation::Mode::Voting);
    assert(loaded.events[0].location == "ballot");
    assert(loaded.events[0].headline == "TEST EVENT");
    assert(loaded.events[0].yearCard == 2024);
    assert(loaded.events[0].barriers.size() == 1);
    assert(loaded.events[0].districts[0].name == "A");
    assert(std::fabs(loaded.events[0].districts[0].turnout - .7f) < .001f);
    assert(loaded.events[1].mode == simulation::Mode::Audience);
    std::filesystem::remove(fixture);

    std::puts("simulation and scenario checks passed");
    return 0;
}
