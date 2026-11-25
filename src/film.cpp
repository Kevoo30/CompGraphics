#include "film.h"
#include "render.h"
#include "animation.h"
#include "locations.h"
#include "film3d.h"
#include "font.h"
#include "audio.h"
#include "scenario.h"
#include "simulation.h"
#include <SDL.h>
#include <SDL_opengl.h>
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <vector>

// ============================================================================
//  THE TIMELINE.  Edit this table to change the film: one row per shot.
//  Dates and facts come from news reports (see the narration script); verify before presenting.
// ============================================================================
namespace {

struct Shot {
    Loc         loc;
    Mood        mood;          // sky, crowd, placards, fists, candles, mourning, halfMast, energy, seed
    float       dur;           // minimum seconds; the shot grows to fit its narration clip (see init)
    const char* place;         // lower-third line 1
    const char* date;          // lower-third line 2
    const char* headline;      // lower-third line 3
    int         yearCard;      // big year numerals at the start of the shot (0 = none)
    bool        applause;      // play the generated applause at the start
    int         kind;          // 0 normal, 1 intro title, 2 outro title
    int         trans;         // how this shot is entered: 0 cinematic cut, 1 dip through black (time jump / change of tone)
};

//                  sky crowd plac  fist  cand  mourn half  energy seed
#define MOOD(sky,crowd,pl,fi,ca,mo,hm,en,sd) Mood{sky,crowd,pl,fi,ca,mo,hm,en,sd}

const Shot SHOTS[] = {
 {Loc::COURT,       MOOD(5,18,0,0,0,0,0,0.2f,1),  10, "", "", "", 0, false, 1, 0},
 {Loc::BALLOT,      MOOD(0,34,0,0,0,0,0,0.3f,17), 15, "POLLING STATIONS, KENYA",                 "9 AUGUST 2022",          "KENYANS VOTE IN THE GENERAL ELECTION",       2022, false, 0, 1},
 {Loc::BOMAS,       MOOD(0,22,0,0,0,0,0,0.3f,2),  15, "BOMAS OF KENYA, NAIROBI",                 "15 AUGUST 2022",         "PRESIDENTIAL RESULT DECLARED",              0,    false, 0, 0},
 {Loc::COURT,       MOOD(1,26,0,0,0,0,0,0.5f,3),  14, "SUPREME COURT, NAIROBI",                  "5 SEPTEMBER 2022",       "COURT UPHOLDS THE ELECTION RESULT",         0,    true,  0, 0},
 {Loc::STADIUM,     MOOD(1,90,0,0,0,0,0,0.4f,13), 15, "MOI INTERNATIONAL SPORTS CENTRE, KASARANI","13 SEPTEMBER 2022",     "SWEARING-IN OF THE NEW PRESIDENT",          0,    false, 0, 0},
 {Loc::STREET,      MOOD(2,34,0,0,0,0,0,0.3f,14), 14, "NAIROBI",                                 "2023",                   "FUEL AND FOOD TAXES RAISE THE COST OF LIVING", 2023, false, 0, 1},
 {Loc::STREET,      MOOD(2,40,1,0,0,0,0,0.6f,1),  15, "NAIROBI",                                 "7 JULY 2023",            "SABA SABA PROTESTS OVER THE FINANCE ACT",   0,    false, 0, 0},
 {Loc::COURT,       MOOD(2,14,0,0,0,0,0,0.1f,4),  14, "COURTS, NAIROBI",                         "NOVEMBER 2023",          "HIGH COURT RULING ON THE HOUSING LEVY",     0,    false, 0, 1},
 {Loc::PARLIAMENT,  MOOD(1,20,0,0,0,0,0,0.2f,18), 14, "PARLIAMENT, NAIROBI",                     "MAY 2024",               "FINANCE BILL 2024 INTRODUCED IN PARLIAMENT", 2024, false, 0, 1},
 {Loc::STREET,      MOOD(4,62,1,1,0,0,0,0.9f,15), 15, "NAIROBI",                                 "20 JUNE 2024",           "PEOPLE PROTEST PROPOSED TAX RISES",         0,    false, 0, 0},
 {Loc::PARLIAMENT,  MOOD(3,60,1,1,0,0,0,1.0f,5),  18, "PARLIAMENT, NAIROBI",                     "25 JUNE 2024",           "FINANCE BILL PASSES, PROTESTS REACH PARLIAMENT", 0, false, 0, 0},
 {Loc::STATE_HOUSE, MOOD(1,0,0,0,0,0,0,0.0f,6),   14, "STATE HOUSE, NAIROBI",                    "26 JUNE 2024",           "PRESIDENT DECLINES TO SIGN THE BILL",       0,    false, 0, 0},
 {Loc::STATE_HOUSE, MOOD(0,0,0,0,0,0,0,0.0f,7),   14, "STATE HOUSE, NAIROBI",                    "11 JULY 2024",           "CABINET DISMISSED",                         0,    false, 0, 0},
 {Loc::PARLIAMENT,  MOOD(4,16,0,0,0,0,0,0.1f,8),  15, "PARLIAMENT, NAIROBI",                     "17 OCTOBER 2024",        "DEPUTY PRESIDENT REMOVED AFTER SENATE VOTE", 0,   false, 0, 1},
 {Loc::STREET,      MOOD(4,30,0,0,1,1,0,0.1f,2),  16, "NAIROBI",                                 "JUNE 2025",              "DEATH IN POLICE CUSTODY SPARKS PROTESTS",   2025, false, 0, 1},
 {Loc::STREET,      MOOD(3,55,1,1,0,0,0,0.9f,3),  15, "NAIROBI",                                 "25 JUNE AND 7 JULY 2025","ANNIVERSARY AND SABA SABA PROTESTS",        0,    false, 0, 0},
 {Loc::STADIUM,     MOOD(3,90,0,0,0,1,1,0.0f,9),  16, "NYAYO NATIONAL STADIUM, NAIROBI",         "17 OCTOBER 2025",        "STATE FUNERAL OF RAILA ODINGA",             0,    false, 0, 1},
 {Loc::HOMESTEAD,   MOOD(0,24,0,0,0,1,1,0.0f,10), 14, "BONDO, SIAYA COUNTY",                     "19 OCTOBER 2025",        "LAID TO REST WITH MILITARY HONOURS",        0,    false, 0, 1},
 {Loc::PARLIAMENT,  MOOD(3,52,1,0,0,0,0,0.6f,16), 14, "PARLIAMENT, NAIROBI",                     "18 JUNE 2026",           "FINANCE BILL 2026 PASSES AFTER AMENDMENTS",  2026, false, 0, 1},
 {Loc::STREET,      MOOD(1,45,1,0,0,0,0,0.5f,4),  15, "NAIROBI",                                 "25 JUNE 2026",           "TWO YEARS ON, MARCHES MARK THE ANNIVERSARY", 0,   false, 0, 0},
 {Loc::BALLOT,      MOOD(1,30,0,0,0,0,0,0.2f,11), 13, "KENYA",                                   "10 AUGUST 2027",         "THE NEXT GENERAL ELECTION",                 0,    false, 0, 1},
 {Loc::PARLIAMENT,  MOOD(5,70,0,0,0,0,0,0.1f,12), 10, "", "", "", 0, false, 2, 1},
};
const int N = (int)(sizeof(SHOTS) / sizeof(SHOTS[0]));
const float DIP        = 0.8f;                    // each half of a dip-to-black (out at the end of a shot, in at the start of the next)

std::vector<float> starts;
std::vector<float> shotDur;                       // real length of each shot: max(table value, narration clip + tail)
const float NARRATION_TAIL = 1.8f;                // quiet time after the voice finishes
float total = 0, T = 0;
bool  paused = false, done = false;
int   lastShot = -1;
std::vector<int> narration;                       // audio ids, -1 when the file is missing
std::vector<float> narrationDuration;
int   musicId = -1;
scenario::Data scenarioData;
simulation::Simulation sceneSimulation;
int simulationShot = -1;
float simulationTime = 0.0f;

const scenario::Event* scenarioEvent(int index) {
    return index >= 0 && index < static_cast<int>(scenarioData.events.size()) ? &scenarioData.events[index] : nullptr;
}

simulation::Config fallbackConfig(int index) {
    simulation::Config c;
    c.mode = SHOTS[index].loc == Loc::BALLOT ? simulation::Mode::Voting : simulation::Mode::Protest;
    c.agents = SHOTS[index].mood.crowd;
    c.turnout = .65f;
    c.seed = static_cast<unsigned>(SHOTS[index].mood.seed);
    c.stations = 3;
    return c;
}

void simulateTo(int index, float local) {
    if (simulationShot != index || local < simulationTime) {
        const scenario::Event* event = scenarioEvent(index);
        sceneSimulation.reset(event ? scenario::simulationConfig(*event) : fallbackConfig(index));
        simulationShot = index;
        simulationTime = 0.0f;
    }
    constexpr float STEP = 1.0f / 30.0f;
    while (simulationTime + 1e-6f < local) {
        float dt = std::min(STEP, local - simulationTime);
        sceneSimulation.step(dt);
        simulationTime += dt;
    }
}

Loc locationForShot(int index) {
    const scenario::Event* event = scenarioEvent(index);
    if (!event || event->location.empty()) return SHOTS[index].loc;
    if (event->location == "court") return Loc::COURT;
    if (event->location == "bomas") return Loc::BOMAS;
    if (event->location == "street") return Loc::STREET;
    if (event->location == "parliament") return Loc::PARLIAMENT;
    if (event->location == "state_house") return Loc::STATE_HOUSE;
    if (event->location == "stadium") return Loc::STADIUM;
    if (event->location == "homestead") return Loc::HOMESTEAD;
    if (event->location == "ballot") return Loc::BALLOT;
    return SHOTS[index].loc;
}

float ease(float t) { t = std::max(0.0f, std::min(1.0f, t)); return t * t * (3 - 2 * t); }
float easeOut(float t) { t = std::max(0.0f, std::min(1.0f, t)); return 1.0f - (1 - t) * (1 - t) * (1 - t); }
float fade(float t, float in0, float in1, float out0, float out1) {      // 0 -> 1 -> 0 envelope
    float a = ease((t - in0) / std::max(0.001f, in1 - in0));
    float b = 1.0f - ease((t - out0) / std::max(0.001f, out1 - out0));
    return std::min(a, b);
}
int shotAt(float t) {
    for (int i = N - 1; i >= 0; i--) if (t >= starts[i]) return i;
    return 0;
}
typedef std::vector<Vec2> Poly;
Poly rect(float x, float y, float w, float h) { return {{x, y}, {x + w, y}, {x + w, y + h}, {x, y + h}}; }

void drawShot(int i, float local) {
    const Shot& s = SHOTS[i];
    // Each event is now a perspective low-poly set; the timeline supplies its action time.
    simulateTo(i, local);
    film3d::drawLocation(locationForShot(i), local, s.mood, &sceneSimulation);
}

void votingOverlay(int index) {
    if (sceneSimulation.mode() != simulation::Mode::Voting) return;
    const auto& tallies = sceneSimulation.tallies();
    if (tallies.empty()) return;
    render::fillPolygonAlpha(rect(552, 24, 232, 226), {8, 13, 20}, .78f);
    font::draw("POLLING DISTRICTS", 566, 36, 2.0f, {240, 190, 70}, .95f);
    font::draw("ILLUSTRATIVE SIMULATION", 566, 54, 1.25f, {190, 195, 205}, .90f);
    // Schematic district map: district markers fill as the simulated queue casts ballots.
    Poly map = {{602,78},{621,70},{637,74},{652,70},{672,78},{687,95},{682,112},{673,122},{668,145},{653,159},{645,181},{632,174},{623,155},{607,145},{596,124},{588,107},{595,94}};
    render::fillPolygon(map, {48, 62, 55});
    render::drawPolygon(map, {125, 146, 130});
    int estimatedCast = 0, estimatedExpected = 0;
    for (std::size_t i = 0; i < tallies.size(); ++i) {
        const auto& d = tallies[i];
        float progress = d.represented > 0 ? std::min(1.0f, float(d.cast) / d.represented) : 0.0f;
        estimatedCast += d.cast;
        estimatedExpected += static_cast<int>(d.expected * d.turnout);
        float radius = 3.0f + progress * 7.0f;
        Color fill = progress > .66f ? Color{80, 175, 95} : (progress > .25f ? Color{175, 160, 65} : Color{110, 120, 115});
        render::fillCircle({588.0f + d.mapX * 100.0f, 70.0f + d.mapY * 120.0f}, radius, fill);
        char line[96];
        std::snprintf(line, sizeof(line), "%s  %d/%d", d.name.c_str(), d.cast, d.represented);
        font::draw(line, 566, 190 + static_cast<float>(i) * 15.0f, 1.6f, {235, 238, 230}, .95f);
    }
    char tally[80];
    std::snprintf(tally, sizeof(tally), "CAST %d   MODEL %d", estimatedCast, estimatedExpected);
    font::draw(tally, 566, 173, 1.6f, {255, 255, 255}, .95f);
    (void)index;
}

void documentaryOverlay(int index, float local) {
    if (sceneSimulation.mode() == simulation::Mode::Voting) return;
    int active = 0;
    for (const auto& agent : sceneSimulation.agents())
        if (agent.state != simulation::State::Finished && agent.state != simulation::State::GaveUp) ++active;
    render::fillPolygonAlpha(rect(0, 18, 250, 48), {8, 13, 20}, .64f);
    font::draw("SIMULATED RECONSTRUCTION", 14, 25, 1.5f, {240, 190, 70}, .95f);
    char line[64];
    std::snprintf(line, sizeof(line), "EVENT %02d   T+%02ds   PEOPLE %d", index + 1, static_cast<int>(local), active);
    font::draw(line, 14, 44, 1.5f, {235, 238, 230}, .95f);
}

void titleCard(float local, float dur) {
    float a = fade(local, 0.8f, 2.0f, dur - 1.6f, dur - 0.4f);
    render::fillPolygonAlpha(rect(0, 190, 800, 150), {0, 0, 0}, 0.42f * a);
    font::draw("THE PEOPLE ARE WATCHING", 400, 212, font::fit("THE PEOPLE ARE WATCHING", 740, 5.0f), {255, 255, 255}, a, true);
    font::draw("KENYA 2022 - 2026", 400, 280, 4.0f, {240, 190, 70}, a, true);
}

void yearCard(int year, float local) {
    float a = fade(local, 1.0f, 1.8f, 2.8f, 3.8f);
    if (a <= 0.01f) return;
    float k = 1.0f + 0.18f * (1.0f - easeOut((local - 1.0f) / 1.2f));    // numerals settle in from slightly larger
    float px = 14.0f * k;
    char buf[8]; std::snprintf(buf, sizeof(buf), "%d", year);
    float lineW = 200.0f * easeOut((local - 1.1f) / 0.9f);
    render::fillPolygonAlpha(rect(0, 70, 800, 150), {0, 0, 0}, 0.35f * a);
    font::draw(buf, 400, 100 + (14.0f - px) * 3.5f, px, {255, 255, 255}, a, true);
    render::fillPolygonAlpha(rect(400 - lineW / 2, 206, lineW, 4), {240, 190, 70}, a);
}

void lowerThird(const Shot& s, const scenario::Event* event, float local, float length) {
    float in  = easeOut((local - 1.0f) / 0.7f);
    float a   = fade(local, 1.0f, 1.6f, length - 1.5f, length - 0.9f);
    if (a <= 0.01f) return;
    float dx  = -50.0f * (1.0f - in);                                    // text slides in from the left
    float bar = 104.0f * in;                                             // gold bar grows downward
    render::fillPolygonAlpha(rect(0, 496, 800, 104), {0, 0, 0}, 0.62f * a);
    render::fillPolygonAlpha(rect(0, 496, 8, bar), {240, 190, 70}, a);
    const char* place = event && !event->place.empty() ? event->place.c_str() : s.place;
    const char* date = event && !event->date.empty() ? event->date.c_str() : s.date;
    const char* headline = event && !event->headline.empty() ? event->headline.c_str() : s.headline;
    font::draw(place,    30 + dx, 504, 2.0f, {190, 195, 205}, a);
    font::draw(date,     30 + dx * 1.2f, 524, font::fit(date, 740, 3.0f), {240, 190, 70}, a);
    font::draw(headline, 30 + dx * 1.4f, 554, font::fit(headline, 740, 3.0f), {255, 255, 255}, a);
}

void vignette() {                                                        // soft dark edges, cinematic framing
    const int RINGS = 26; const float STEP = 4.5f;
    for (int k = 0; k < RINGS; k++) {
        float d = k * STEP, f = 1.0f - (float)k / RINGS, a = 0.075f * f * f, w = STEP;
        render::fillPolygonAlpha(rect(0, d, 800, w), {0, 0, 0}, a);
        render::fillPolygonAlpha(rect(0, 600 - d - w, 800, w), {0, 0, 0}, a);
        render::fillPolygonAlpha(rect(d, d + w, w, 600 - 2 * (d + w)), {0, 0, 0}, a);
        render::fillPolygonAlpha(rect(800 - d - w, d + w, w, 600 - 2 * (d + w)), {0, 0, 0}, a);
    }
}

void filmGrain(float time) {
    unsigned state = static_cast<unsigned>(time * 30.0f) + 0x9e3779b9u;
    const float logicalWidth = 600.0f * float(render::width()) / std::max(1, render::height());
    const float offset = (logicalWidth - 800.0f) * .5f;
    glPointSize(1.0f);
    glBegin(GL_POINTS);
    for (int i = 0; i < 900; ++i) {
        state = state * 1664525u + 1013904223u;
        float x = float(state >> 8) / 16777216.0f * logicalWidth - offset;
        state = state * 1664525u + 1013904223u;
        float y = float(state >> 8) / 16777216.0f * 600.0f;
        float v = (state & 1u) ? .045f : .018f;
        glColor4f(v, v, v, .24f);
        glVertex2f(x, y);
    }
    glEnd();
}

void startShot(int i) {
    if (i < 0 || i >= N) return;
    audio::stopAll(true);                                                // keep the crowd murmur, stop old narration
    if (narration[i] >= 0) audio::play(narration[i], 1.0f);
    if (SHOTS[i].applause) audio::play(audio::applause(), 0.7f);
}

} // namespace

namespace film {

void init() {
    std::string scenarioError;
    if (!scenario::load("assets/scenario.json", scenarioData, scenarioError)) {
        std::printf("Scenario unavailable (%s); using built-in event settings.\n", scenarioError.c_str());
        scenarioData.events.clear();
    }
    simulationShot = -1; simulationTime = 0.0f;
    narration.assign(N, -1);
    narrationDuration.assign(N, 0.0f);
    for (int i = 0; i < N; i++) {
        char path[64]; std::snprintf(path, sizeof(path), "assets/narration/%02d.wav", i + 1);
        narration[i] = audio::load(path);                                // -1 if the file is not there
        narrationDuration[i] = narration[i] >= 0 ? audio::length(narration[i]) : audio::wavLength(path);
    }
    // Auto-fit: a shot lasts at least its table value, and long enough for its narration plus a short tail.
    starts.clear(); shotDur.assign(N, 0.0f); total = 0;
    for (int i = 0; i < N; i++) {
        const scenario::Event* event = scenarioEvent(i);
        shotDur[i] = event ? event->duration : SHOTS[i].dur;
        if (narrationDuration[i] > 0.0f) shotDur[i] = std::max(shotDur[i], narrationDuration[i] + NARRATION_TAIL);
        starts.push_back(total); total += shotDur[i];
    }
    musicId = audio::load("assets/music.wav");
    if (musicId >= 0) audio::play(musicId, 0.25f, true);
    T = 0; paused = false; done = false; lastShot = -1;
}

float totalTime() { return total; }
bool  finished()  { return done; }
void  seek(float t) { T = std::max(0.0f, std::min(total - 0.01f, t)); lastShot = -1; }

std::vector<AudioCue> audioCues() {
    std::vector<AudioCue> cues;
    for (int i = 0; i < N; ++i) {
        if (narrationDuration[i] <= 0.0f) continue;
        char path[64]; std::snprintf(path, sizeof(path), "assets/narration/%02d.wav", i + 1);
        cues.push_back({path, starts[i], 1.0f, false});
    }
    if (audio::wavLength("assets/music.wav") > 0.0f) cues.push_back({"assets/music.wav", 0.0f, .25f, true});
    return cues;
}

void update(float dt, const Input& in) {
    if (in.pause) paused = !paused;
    const Uint8* k = SDL_GetKeyboardState(nullptr);
    static bool pl = false, pr = false;
    bool l = k[SDL_SCANCODE_LEFT] != 0, r = k[SDL_SCANCODE_RIGHT] != 0;
    int cur = shotAt(T);
    if (r && !pr && cur + 1 < N) { T = starts[cur + 1]; lastShot = -1; }
    if (l && !pl) { T = (T - starts[cur] > 2.0f) ? starts[cur] : starts[std::max(0, cur - 1)]; lastShot = -1; }
    pl = l; pr = r;

    if (!paused) T += dt;
    if (T >= total) { T = total; done = true; }
    int s = shotAt(std::min(T, total - 0.001f));
    if (s != lastShot) { startShot(s); lastShot = s; }
}

void draw() {
    float t = std::min(T, total - 0.001f);
    int i = shotAt(t);
    float local = t - starts[i];
    // Match the reference film's clear cinematic cuts; time jumps still dip through black.
    drawShot(i, local);

    // The scene pass uses perspective and depth testing. Re-establish the pixel-space
    // overlay camera explicitly so titles and captions cannot inherit 3D state.
    glPushAttrib(GL_ALL_ATTRIB_BITS);
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    const float logicalWidth = 600.0f * float(render::width()) / std::max(1, render::height());
    glOrtho(0, logicalWidth, 600, 0, -1, 1);
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();
    glTranslatef((logicalWidth - 800.0f) * .5f, 0.0f, 0.0f);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_LIGHTING);
    glDisable(GL_CULL_FACE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    vignette();
    filmGrain(t);
    documentaryOverlay(i, local);
    votingOverlay(i);
    const Shot& s = SHOTS[i];
    const scenario::Event* event = scenarioEvent(i);
    if (s.kind == 1 || s.kind == 2) titleCard(local, shotDur[i]);
    else { lowerThird(s, event, local, shotDur[i]); int year = event && event->yearCard ? event->yearCard : s.yearCard; if (year) yearCard(year, local); }

    // dip through black: fade in at the start of a "time jump" shot, fade out at the end of a shot that is followed by one
    float black = 0.0f;
    if (s.trans == 1 && i > 0) black = std::max(black, 1.0f - ease(local / DIP));
    if (i + 1 < N && SHOTS[i + 1].trans == 1) black = std::max(black, ease((local - (shotDur[i] - DIP)) / DIP));
    black = std::max(black, std::max(1.0f - ease(t / 1.2f), ease((t - (total - 2.2f)) / 2.2f)));   // film fade in / out
    if (black > 0.001f) render::fillPolygonAlpha(rect(-((logicalWidth - 800.0f) * .5f), 0, logicalWidth, 600), {0, 0, 0}, black);

    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    glPopAttrib();
}

}
