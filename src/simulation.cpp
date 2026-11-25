#include "simulation.h"

#include <algorithm>
#include <cmath>
#include <utility>

namespace simulation {
namespace {

float length(Vec2 v) { return std::sqrt(v.x * v.x + v.y * v.y); }
Vec2 add(Vec2 a, Vec2 b) { return {a.x + b.x, a.y + b.y}; }
Vec2 sub(Vec2 a, Vec2 b) { return {a.x - b.x, a.y - b.y}; }
Vec2 mul(Vec2 a, float k) { return {a.x * k, a.y * k}; }
Vec2 limit(Vec2 v, float max) {
    float n = length(v);
    return n > max && n > 0.0f ? mul(v, max / n) : v;
}
float hash01(unsigned n) {
    n ^= n >> 16; n *= 0x7feb352du; n ^= n >> 15; n *= 0x846ca68bu; n ^= n >> 16;
    return float(n & 0x00ffffffu) / float(0x01000000u);
}
Vec2 normalized(Vec2 v) { float n = length(v); return n > 1e-5f ? mul(v, 1.0f / n) : Vec2{}; }

} // namespace

Simulation::Simulation(Config config) { reset(std::move(config)); }

void Simulation::reset(Config config) {
    config_ = std::move(config);
    config_.agents = std::max(0, config_.agents);
    config_.stations = std::max(1, config_.stations);
    config_.turnout = std::max(0.0f, std::min(1.0f, config_.turnout));
    agents_.clear(); tallies_.clear(); ballotsCast_ = 0; gaveUpCount_ = 0; elapsed_ = 0.0f;

    if (config_.districts.empty()) config_.districts.push_back({"Central", config_.turnout, std::max(1, config_.agents)});
    for (const District& district : config_.districts)
        tallies_.push_back({district.name, std::max(0, district.expectedVoters), 0, 0, district.turnout, district.mapX, district.mapY});

    std::vector<int> stationCounts(config_.stations, 0);
    agents_.reserve(static_cast<std::size_t>(config_.agents));
    for (int i = 0; i < config_.agents; ++i) {
        Agent a;
        a.id = static_cast<std::uint32_t>(i);
        const unsigned key = config_.seed + static_cast<unsigned>(i * 97);
        a.age = hash01(key + 1) < .18f ? AgeGroup::Youth : (hash01(key + 2) > .84f ? AgeGroup::Senior : AgeGroup::Adult);
        a.opinion = hash01(key + 3);
        a.motivation = .25f + .75f * hash01(key + 4);
        a.patience = 5.0f + 16.0f * hash01(key + 5);
        a.district = i % static_cast<int>(config_.districts.size());
        if (a.age == AgeGroup::Youth) a.patience *= .90f;
        if (a.age == AgeGroup::Senior) a.patience *= 1.15f;
        a.station = i % config_.stations;
        a.queueRank = stationCounts[a.station]++;

        if (config_.mode == Mode::Voting) {
            // Queue lanes sit in front of a polling station at the origin.
            const float lane = (a.station - (config_.stations - 1) * .5f) * 1.35f;
            a.position = {-8.0f + hash01(key + 6) * 16.0f, 10.0f + hash01(key + 7) * 5.0f};
            a.velocity = {};
            a.state = State::Approaching;
            // Use the turnout attribute as an individual's decision to participate.
            if (hash01(key + 8) > config_.districts[a.district].turnout) a.state = State::Finished;
            else ++tallies_[a.district].represented;
            (void)lane;
        } else {
            const float angle = hash01(key + 6) * 6.2831853f;
            const float radius = 7.0f + hash01(key + 7) * 6.0f;
            a.position = {std::cos(angle) * radius, std::sin(angle) * radius};
            a.state = State::Gathering;
            a.arrivalTime = hash01(key + 9) * 3.0f;
            if (a.arrivalTime > 0.0f) a.state = State::Finished;
        }
        agents_.push_back(a);
    }
}

void Simulation::step(float dt) {
    if (!(dt > 0.0f) || !std::isfinite(dt)) return;
    dt = std::min(dt, .1f);
    elapsed_ += dt;
    if (config_.mode == Mode::Voting) stepVoting(dt); else stepProtest(dt);
}

void Simulation::stepVoting(float dt) {
    for (Agent& a : agents_) {
        a.stateTime += dt;
        const float lane = (a.station - (config_.stations - 1) * .5f) * 1.35f;
        int rankAhead = 0;
        for (const Agent& other : agents_)
            if (other.station == a.station && other.queueRank < a.queueRank &&
                other.state != State::Finished && other.state != State::GaveUp && other.state != State::Leaving)
                ++rankAhead;
        const Vec2 queueSpot{lane, 5.2f - rankAhead * .72f};
        const Vec2 exitSpot{lane + (a.station % 2 ? 9.0f : -9.0f), -2.0f};
        if (a.state == State::Approaching || a.state == State::Queuing) {
            const Vec2 delta = sub(queueSpot, a.position);
            if (length(delta) < .18f) {
                a.position = queueSpot; a.velocity = {}; a.state = State::Waiting; a.stateTime = 0.0f;
            } else {
                a.state = a.state == State::Waiting ? State::Queuing : a.state;
                a.velocity = limit(add(mul(a.velocity, .88f), mul(normalized(delta), 1.8f)), 2.2f);
                a.position = add(a.position, mul(a.velocity, dt));
            }
        } else if (a.state == State::Waiting) {
            // Long queues discourage some visitors. A deterministic personal patience threshold
            // keeps results reproducible while avoiding synchronized state changes.
            const float waitBeforeService = 1.2f + rankAhead * .65f;
            if (waitBeforeService > a.patience) {
                a.state = State::GaveUp; ++gaveUpCount_;
            } else if (a.stateTime >= waitBeforeService) {
                a.state = State::Casting; a.stateTime = 0.0f;
            }
        } else if (a.state == State::Casting && a.stateTime >= .8f) {
            a.state = State::Leaving; a.stateTime = 0.0f;
            ++ballotsCast_;
            if (a.district >= 0 && a.district < static_cast<int>(tallies_.size())) ++tallies_[a.district].cast;
        } else if (a.state == State::Leaving) {
            Vec2 delta = sub(exitSpot, a.position);
            a.velocity = limit(add(mul(a.velocity, .88f), mul(normalized(delta), 2.0f)), 2.2f);
            a.position = add(a.position, mul(a.velocity, dt));
            if (length(delta) < .25f) a.state = State::Finished;
        }
    }
}

void Simulation::stepProtest(float dt) {
    const std::size_t count = agents_.size();
    std::vector<Vec2> nextVelocity(count);
    for (std::size_t i = 0; i < count; ++i) {
        Agent& a = agents_[i];
        if (a.arrivalTime > elapsed_) { nextVelocity[i] = {}; continue; }
        if (a.state == State::Finished) { a.state = State::Gathering; a.stateTime = 0.0f; }
        a.stateTime += dt;
        Vec2 separation{}, cohesion{}, alignment{};
        int neighbors = 0;
        for (std::size_t j = 0; j < count; ++j) {
            if (i == j || agents_[j].state == State::Finished) continue;
            Vec2 offset = sub(a.position, agents_[j].position);
            float d = length(offset);
            if (d < 2.2f && d > 1e-4f) {
                separation = add(separation, mul(normalized(offset), 1.0f / d));
                cohesion = add(cohesion, agents_[j].position);
                alignment = add(alignment, agents_[j].velocity);
                ++neighbors;
            }
        }
        Vec2 steer{};
        if (neighbors) {
            cohesion = sub(mul(cohesion, 1.0f / neighbors), a.position);
            alignment = sub(mul(alignment, 1.0f / neighbors), a.velocity);
            steer = add(mul(separation, 1.8f), add(mul(normalized(cohesion), .24f), mul(alignment, .35f)));
        }

        const float settle = std::max(0.0f, 1.0f - a.stateTime / 10.0f);
        const float radialNoise = (hash01(config_.seed + a.id * 17u) - .5f) * 1.2f;
        const float radius = (1.3f + radialNoise) + 4.0f * settle;
        const float angle = hash01(config_.seed + a.id * 37u + 11u) * 6.2831853f;
        const Vec2 rallyGoal{std::cos(angle) * radius, std::sin(angle) * radius};
        const float motivation = std::max(.15f, a.motivation * (.72f + .28f * a.opinion));
        steer = add(steer, mul(sub(rallyGoal, a.position), a.state == State::Gathering ? .45f : .09f));
        if (a.stateTime < 2.0f) a.state = State::Gathering;
        else if (elapsed_ < 10.0f) a.state = State::Rallying;
        else a.state = State::Dispersing;
        if (a.state == State::Dispersing) steer = add(steer, mul(normalized(a.position), .8f));
        for (const Barrier& barrier : config_.barriers) {
            Vec2 away = sub(a.position, {barrier.x, barrier.y});
            float d = length(away);
            if (d < barrier.radius + 1.0f && d > 1e-4f)
                steer = add(steer, mul(normalized(away), (barrier.radius + 1.0f - d) * 2.0f));
        }
        nextVelocity[i] = limit(add(mul(a.velocity, .92f), mul(steer, dt * motivation)), 1.2f + motivation);
    }
    for (std::size_t i = 0; i < count; ++i) {
        Agent& a = agents_[i];
        a.velocity = nextVelocity[i];
        a.position = add(a.position, mul(a.velocity, dt));
        if (a.state == State::Dispersing && length(a.position) > 14.0f) a.state = State::Finished;
    }
}

} // namespace simulation
