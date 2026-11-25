#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace simulation {

struct Vec2 {
    float x = 0.0f;
    float y = 0.0f;
};

enum class AgeGroup { Youth, Adult, Senior };
enum class Mode { Voting, Protest };
enum class State { Approaching, Queuing, Waiting, Casting, Leaving, Gathering, Rallying, Dispersing, Finished, GaveUp };

struct Barrier { float x = 0.0f; float y = 0.0f; float radius = 1.0f; };

struct District {
    std::string name;
    float turnout = 0.65f;
    int expectedVoters = 1000;
    float mapX = 0.5f;
    float mapY = 0.5f;
};

struct Config {
    Mode mode = Mode::Protest;
    int agents = 40;
    float turnout = 0.65f;
    unsigned seed = 1;
    int stations = 3;
    std::vector<District> districts;
    std::vector<Barrier> barriers;
};

struct Agent {
    std::uint32_t id = 0;
    Vec2 position;
    Vec2 velocity;
    AgeGroup age = AgeGroup::Adult;
    float opinion = 0.5f;
    float motivation = 0.5f;
    State state = State::Approaching;
    int district = 0;
    int station = 0;
    int queueRank = 0;
    float stateTime = 0.0f;
    float patience = 10.0f;
    float arrivalTime = 0.0f;
};

struct DistrictTally {
    std::string name;
    int expected = 0;
    int cast = 0;
    int represented = 0;
    float turnout = 0.0f;
    float mapX = 0.5f;
    float mapY = 0.5f;
};

class Simulation {
public:
    explicit Simulation(Config config = {});
    void reset(Config config);
    void step(float dt);

    const std::vector<Agent>& agents() const { return agents_; }
    const std::vector<DistrictTally>& tallies() const { return tallies_; }
    int ballotsCast() const { return ballotsCast_; }
    int gaveUpCount() const { return gaveUpCount_; }
    float elapsed() const { return elapsed_; }
    Mode mode() const { return config_.mode; }
    const std::vector<Barrier>& barriers() const { return config_.barriers; }

private:
    void stepVoting(float dt);
    void stepProtest(float dt);
    Config config_;
    std::vector<Agent> agents_;
    std::vector<DistrictTally> tallies_;
    int ballotsCast_ = 0;
    int gaveUpCount_ = 0;
    float elapsed_ = 0.0f;
};

} // namespace simulation
