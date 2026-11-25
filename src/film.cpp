#include "film.h"
#include "render.h"
#include "transform.h"
#include "animation.h"
#include "locations.h"
#include "font.h"
#include "audio.h"
#include "interior3d.h"
#include <SDL.h>
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
    int         trans;         // how this shot is entered: 0 whip-pan (continuous action), 1 dip through black (time jump / change of tone)
};

//                  sky crowd plac  fist  cand  mourn half  energy seed
#define MOOD(sky,crowd,pl,fi,ca,mo,hm,en,sd) Mood{sky,crowd,pl,fi,ca,mo,hm,en,sd}
#define MOODX(sky,crowd,pl,fi,ca,mo,hm,en,sd,bn,sm,po) Mood{sky,crowd,pl,fi,ca,mo,hm,en,sd,bn,sm,po}

const Shot SHOTS[] = {
 {Loc::COURT,       MOOD(5,18,0,0,0,0,0,0.2f,1),  10, "", "", "", 0, false, 1, 0},
 {Loc::BALLOT,      MOOD(0,34,0,0,0,0,0,0.3f,17), 15, "POLLING STATIONS, KENYA",                 "9 AUGUST 2022",          "KENYANS VOTE IN THE GENERAL ELECTION",       2022, false, 0, 1},
 {Loc::BOMAS,       MOOD(0,22,0,0,0,0,0,0.3f,2),  15, "BOMAS OF KENYA, NAIROBI",                 "15 AUGUST 2022",         "PRESIDENTIAL RESULT DECLARED",              0,    false, 0, 0},
 {Loc::COURT,       MOOD(1,26,0,0,0,0,0,0.5f,3),  14, "SUPREME COURT, NAIROBI",                  "5 SEPTEMBER 2022",       "COURT UPHOLDS THE ELECTION RESULT",         0,    true,  0, 0},
 {Loc::STADIUM,     MOOD(1,90,0,0,0,0,0,0.4f,13), 15, "MOI INTERNATIONAL SPORTS CENTRE, KASARANI","13 SEPTEMBER 2022",     "SWEARING-IN OF THE NEW PRESIDENT",          0,    false, 0, 0},
 {Loc::STREET,      MOOD(2,34,0,0,0,0,0,0.3f,14), 14, "NAIROBI",                                 "2023",                   "FUEL AND FOOD TAXES RAISE THE COST OF LIVING", 2023, false, 0, 1},
 {Loc::STREET,      MOODX(2,54,1,0,0,0,0,0.6f,1,"SABA SABA",0,0),  15, "NAIROBI",                                 "7 JULY 2023",            "SABA SABA PROTESTS OVER THE FINANCE ACT",   0,    false, 0, 0},
 {Loc::COURT,       MOOD(2,14,0,0,0,0,0,0.1f,4),  14, "COURTS, NAIROBI",                         "NOVEMBER 2023",          "HIGH COURT RULING ON THE HOUSING LEVY",     0,    false, 0, 1},
 {Loc::PARLIAMENT,  MOOD(1,20,0,0,0,0,0,0.2f,18), 14, "PARLIAMENT, NAIROBI",                     "MAY 2024",               "FINANCE BILL 2024 INTRODUCED IN PARLIAMENT", 2024, false, 0, 1},
 {Loc::STREET,      MOODX(4,90,1,1,0,0,0,0.9f,15,"REJECT FINANCE BILL",0,1), 15, "NAIROBI",                                 "20 JUNE 2024",           "PEOPLE PROTEST PROPOSED TAX RISES",         0,    false, 0, 0},
 {Loc::PARLIAMENT,  MOODX(3,90,1,1,0,0,0,1.0f,5,"REJECT FINANCE BILL",1,1),  18, "PARLIAMENT, NAIROBI",                     "25 JUNE 2024",           "FINANCE BILL PASSES, PROTESTS REACH PARLIAMENT", 0, false, 0, 0},
 {Loc::STATE_HOUSE, MOOD(1,0,0,0,0,0,0,0.0f,6),   14, "STATE HOUSE, NAIROBI",                    "26 JUNE 2024",           "PRESIDENT DECLINES TO SIGN THE BILL",       0,    false, 0, 0},
 {Loc::STATE_HOUSE, MOOD(0,0,0,0,0,0,0,0.0f,7),   14, "STATE HOUSE, NAIROBI",                    "11 JULY 2024",           "CABINET DISMISSED",                         0,    false, 0, 0},
 {Loc::PARLIAMENT,  MOOD(4,16,0,0,0,0,0,0.1f,8),  15, "PARLIAMENT, NAIROBI",                     "17 OCTOBER 2024",        "DEPUTY PRESIDENT REMOVED AFTER SENATE VOTE", 0,   false, 0, 1},
 {Loc::STREET,      MOODX(4,30,0,0,1,1,0,0.1f,2,"JUSTICE",0,0),  16, "NAIROBI",                                 "JUNE 2025",              "DEATH IN POLICE CUSTODY SPARKS PROTESTS",   2025, false, 0, 1},
 {Loc::STREET,      MOODX(3,72,1,1,0,0,0,0.9f,3,"WE REMEMBER",1,1),  15, "NAIROBI",                                 "25 JUNE AND 7 JULY 2025","ANNIVERSARY AND SABA SABA PROTESTS",        0,    false, 0, 0},
 {Loc::STADIUM,     MOOD(3,90,0,0,0,1,1,0.0f,9),  16, "NYAYO NATIONAL STADIUM, NAIROBI",         "17 OCTOBER 2025",        "STATE FUNERAL OF RAILA ODINGA",             0,    false, 0, 1},
 {Loc::HOMESTEAD,   MOOD(0,24,0,0,0,1,1,0.0f,10), 14, "BONDO, SIAYA COUNTY",                     "19 OCTOBER 2025",        "LAID TO REST WITH MILITARY HONOURS",        0,    false, 0, 1},
 {Loc::PARLIAMENT,  MOOD(3,52,1,0,0,0,0,0.6f,16), 14, "PARLIAMENT, NAIROBI",                     "18 JUNE 2026",           "FINANCE BILL 2026 PASSES AFTER AMENDMENTS",  2026, false, 0, 1},
 {Loc::STREET,      MOODX(1,72,1,0,0,0,0,0.5f,4,"TWO YEARS ON",0,0),  15, "NAIROBI",                                 "25 JUNE 2026",           "TWO YEARS ON, MARCHES MARK THE ANNIVERSARY", 0,   false, 0, 0},
 {Loc::BALLOT,      MOOD(1,30,0,0,0,0,0,0.2f,11), 13, "KENYA",                                   "10 AUGUST 2027",         "THE NEXT GENERAL ELECTION",                 0,    false, 0, 1},
 {Loc::PARLIAMENT,  MOOD(5,70,0,0,0,0,0,0.1f,12), 10, "", "", "", 0, false, 2, 1},
};
const int N = (int)(sizeof(SHOTS) / sizeof(SHOTS[0]));
const float TRANSITION = 1.0f;                    // whip-pan length in seconds
const float DIP        = 0.8f;                    // each half of a dip-to-black (out at the end of a shot, in at the start of the next)

std::vector<float> starts;
std::vector<float> shotDur;                       // real length of each shot: max(table value, narration clip + tail)
const float NARRATION_TAIL = 1.8f;                // quiet time after the voice finishes
float total = 0, T = 0;
bool  paused = false, done = false;
int   lastShot = -1;
std::vector<int> narration;                       // audio ids, -1 when the file is missing
int   musicId = -1;

float ease(float t) { t = std::max(0.0f, std::min(1.0f, t)); return t * t * (3 - 2 * t); }
float easeOut(float t) { t = std::max(0.0f, std::min(1.0f, t)); return 1.0f - (1 - t) * (1 - t) * (1 - t); }
float easeIO3(float t) { t = std::max(0.0f, std::min(1.0f, t)); return t < 0.5f ? 4 * t * t * t : 1 - std::pow(-2 * t + 2, 3) / 2; }
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

// ---- 3D interiors: each shot opens on its 2D exterior, then cuts inside (court, chamber, polling station, stadium, street) ----
struct Seg { world3d::Scene sc; float from, to; };          // portion of the shot (0..1) shown in 3D
struct Ins { Seg a, b; world3d::Params p; };
std::vector<Ins> INS;
bool hasInside(int i) { return i >= 0 && i < (int)INS.size() && INS[i].a.sc != world3d::NONE; }
void buildInside() {
    using namespace world3d;
    INS.assign(N, Ins{{NONE, 0, 0}, {NONE, 0, 0}, Params{}});
    auto P = [](int crowd, float energy, int seed) { Params p; p.crowd = crowd; p.energy = energy; p.seed = seed; return p; };
    // crowd sizes, stations and barricades follow assets/events.json (illustrative values)
    INS[1]  = {{POLLING, 0.28f, 1}, {NONE, 0, 0}, P(52, 0.3f, 17)};   INS[1].p.stations = 4;
    INS[2]  = {{STATE_HOUSE, 0.30f, 1}, {NONE, 0, 0}, P(22, 0.3f, 2)}; INS[2].p.variant = 1;
    INS[3]  = {{COURT, 0.30f, 1}, {NONE, 0, 0}, P(26, 0.2f, 3)};
    INS[4]  = {{INAUGURATION, 0.25f, 1}, {NONE, 0, 0}, P(90, 0.4f, 13)};
    INS[6]  = {{PROTEST_STREET, 0.30f, 1}, {NONE, 0, 0}, P(54, 0.6f, 1)};
    INS[7]  = {{COURT, 0.30f, 1}, {NONE, 0, 0}, P(14, 0.1f, 4)};
    INS[8]  = {{CHAMBER, 0.30f, 1}, {NONE, 0, 0}, P(60, 0.2f, 18)};
    INS[9]  = {{PROTEST_STREET, 0.30f, 1}, {NONE, 0, 0}, P(90, 0.9f, 15)}; INS[9].p.barriers = {{-2.0f, 0.5f, 1.2f}, {3.0f, -1.0f, 1.0f}};
    INS[10] = {{CHAMBER, 0.12f, 0.50f}, {PROTEST_PARLIAMENT, 0.50f, 1}, P(90, 1.0f, 5)};
    INS[10].p.vote = true; INS[10].p.smoke = true; INS[10].p.police = true; INS[10].p.barriers = {{-2.5f, 0.0f, 1.4f}, {2.2f, 0.8f, 1.4f}};
    INS[11] = {{STATE_HOUSE, 0.25f, 1}, {NONE, 0, 0}, P(0, 0.0f, 6)};
    INS[12] = {{STATE_HOUSE, 0.25f, 1}, {NONE, 0, 0}, P(0, 0.0f, 7)};
    INS[13] = {{CHAMBER, 0.30f, 1}, {NONE, 0, 0}, P(48, 0.1f, 8)}; INS[13].p.vote = true;
    INS[14] = {{PROTEST_STREET, 0.30f, 1}, {NONE, 0, 0}, P(30, 0.1f, 2)}; INS[14].p.night = true; INS[14].p.candles = true; INS[14].p.mourning = true;
    INS[15] = {{PROTEST_STREET, 0.30f, 1}, {NONE, 0, 0}, P(72, 0.9f, 3)}; INS[15].p.smoke = true; INS[15].p.police = true; INS[15].p.barriers = {{-2.0f, -0.5f, 1.1f}};
    INS[16] = {{INAUGURATION, 0.30f, 1}, {NONE, 0, 0}, P(90, 0.0f, 9)}; INS[16].p.mourning = true;
    INS[18] = {{CHAMBER, 0.30f, 1}, {NONE, 0, 0}, P(90, 0.6f, 16)}; INS[18].p.vote = true;
    INS[19] = {{PROTEST_STREET, 0.30f, 1}, {NONE, 0, 0}, P(72, 0.5f, 4)};
    INS[20] = {{POLLING, 0.28f, 1}, {NONE, 0, 0}, P(48, 0.2f, 11)};   INS[20].p.stations = 4;
    // the 3D inside of every shot gets the SAME time of day as its 2D exterior (0 dawn, 1 day, 2 midday, 3 dusk, 4 night, 5 pre-dawn)
    for (int i = 0; i < N; i++) { INS[i].p.sky = SHOTS[i].mood.sky; if (SHOTS[i].mood.sky == 4) INS[i].p.night = true; }
}
bool draw3D(int i, float local) {                          // returns true when this frame is a 3D one
    if (!hasInside(i)) return false;
    float f = local / shotDur[i];
    for (const Seg* sg : {&INS[i].a, &INS[i].b}) {
        if (sg->sc == world3d::NONE) continue;
        if (f >= sg->from && f < sg->to) { world3d::draw(sg->sc, local, (f - sg->from) / std::max(0.001f, sg->to - sg->from), INS[i].p, render::width(), render::height()); return true; }
    }
    return false;
}
float cutPulse(int i, float local) {                       // short dip to black masking each cut between 2D and 3D
    if (!hasInside(i)) return 0.0f;
    float m = 0.0f;
    for (const Seg* sg : {&INS[i].a, &INS[i].b}) { if (sg->sc == world3d::NONE) continue; m = std::max(m, 1.0f - std::fabs(local - sg->from * shotDur[i]) / 0.3f); }
    return ease(m);
}

// illustrative turnout panel (values from assets/events.json) shown on the voting shots
void turnoutPanel(int i, float local) {
    static const char* names[4] = {"NAIROBI", "MOMBASA", "KISUMU", "NAKURU"};
    static const float tv[4] = {0.68f, 0.63f, 0.65f, 0.70f};
    float a = fade(local, 4.0f, 5.0f, shotDur[i] - 2.5f, shotDur[i] - 1.5f); if (a <= 0.01f) return;
    render::fillPolygonAlpha(rect(520, 18, 270, 150), {0, 0, 0}, 0.6f * a);
    render::fillPolygonAlpha(rect(520, 18, 6, 150), {240, 190, 70}, a);
    font::draw("TURNOUT BY COUNTY", 540, 26, 2.0f, {240, 190, 70}, a);
    font::draw("ILLUSTRATIVE VALUES", 540, 42, 1.5f, {170, 175, 185}, a);
    for (int k = 0; k < 4; k++) {
        float y = 64 + k * 24, g = ease((local - 5.0f - k * 0.25f) / 1.2f);
        font::draw(names[k], 540, y, 1.8f, {255, 255, 255}, a);
        render::fillPolygonAlpha(rect(620, y, 130, 12), {255, 255, 255}, 0.18f * a);
        render::fillPolygonAlpha(rect(620, y, 130 * tv[k] * g, 12), {80, 190, 110}, a);
        char b[8]; std::snprintf(b, sizeof(b), "%d%%", (int)(tv[k] * 100 * g + 0.5f)); font::draw(b, 755, y + 1, 1.5f, {255, 255, 255}, a);
    }
}

void drawShot(int i, float local, float slideX) {
    const Shot& s = SHOTS[i];
    float p = std::max(0.0f, std::min(1.0f, local / shotDur[i]));         // linear: the camera never stops, so shots feel alive
    float zoom = 1.02f + 0.07f * (p * (2 - p));                          // gentle push-in that eases out
    float pan  = ((i % 2) ? 1.0f : -1.0f) * (p - 0.5f) * 26.0f;          // alternating sideways drift
    float lift = ((i % 3) - 1) * (p - 0.5f) * 8.0f;                      // tiny vertical drift
    float shk = s.mood.energy * s.mood.energy * 3.0f;                    // handheld shake grows with the crowd's energy
    float shx = shk * (std::sin(local * 13.1f + i) + 0.6f * std::sin(local * 29.7f)), shy = shk * 0.7f * std::sin(local * 17.3f + 2.0f);
    Mat3 M = translate(400 + slideX + pan + shx, 300 + lift + shy) * scale(zoom, zoom) * translate(-400, -300);
    drawLocation(s.loc, local, s.mood, M);
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

void lowerThird(const Shot& s, float local, float length) {
    float in  = easeOut((local - 1.0f) / 0.7f);
    float a   = fade(local, 1.0f, 1.6f, length - 1.5f, length - 0.9f);
    if (a <= 0.01f) return;
    float dx  = -50.0f * (1.0f - in);                                    // text slides in from the left
    float bar = 104.0f * in;                                             // gold bar grows downward
    render::fillPolygonAlpha(rect(0, 496, 800, 104), {0, 0, 0}, 0.62f * a);
    render::fillPolygonAlpha(rect(0, 496, 8, bar), {240, 190, 70}, a);
    font::draw(s.place,    30 + dx, 504, 2.0f, {190, 195, 205}, a);
    font::draw(s.date,     30 + dx * 1.2f, 524, font::fit(s.date, 740, 3.0f), {240, 190, 70}, a);
    font::draw(s.headline, 30 + dx * 1.4f, 554, font::fit(s.headline, 740, 3.0f), {255, 255, 255}, a);
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

void grain(float t) {                                                    // film grain: tiny flickering specks
    int f = (int)(t * 24.0f);
    for (int k = 0; k < 110; k++) {
        unsigned h = (unsigned)f * 7919u + (unsigned)k * 104729u; h ^= h >> 13; h *= 0x5bd1e995u; h ^= h >> 15;
        render::fillPolygonAlpha(rect((float)(h % 800), (float)((h >> 10) % 600), 2, 2), (h >> 20) & 1 ? Color{255, 255, 255} : Color{0, 0, 0}, 0.07f);
    }
}

void whipStreaks(int i, float p) {                                       // horizontal speed lines at the peak of a whip-pan
    float peak = std::sin(p * 3.14159265f);
    if (peak <= 0.02f) return;
    for (int k = 0; k < 14; k++) {
        unsigned h = (unsigned)(i * 2654435761u + k * 40503u); h ^= h >> 13; h *= 0x5bd1e995u; h ^= h >> 15;
        float y = (float)(h % 560) + 20.0f, len = 260.0f + (float)((h >> 8) % 440), thick = 2.0f + (float)((h >> 16) % 5);
        float x = (p < 0.5f ? 1 : -1) * 0.0f + 800.0f * (1.0f - p) - len * 0.5f + (float)((h >> 4) % 200) - 100.0f;
        render::fillPolygonAlpha(rect(x, y, len, thick), {255, 255, 255}, 0.22f * peak);
    }
    render::fillPolygonAlpha(rect(0, 0, 800, 600), {0, 0, 0}, 0.18f * peak);   // slight darkening sells the speed
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
    narration.assign(N, -1);
    for (int i = 0; i < N; i++) {
        char path[64]; std::snprintf(path, sizeof(path), "assets/narration/%02d.wav", i + 1);
        narration[i] = audio::load(path);                                // -1 if the file is not there
    }
    // Auto-fit: a shot lasts at least its table value, and long enough for its narration plus a short tail.
    starts.clear(); shotDur.assign(N, 0.0f); total = 0;
    for (int i = 0; i < N; i++) {
        shotDur[i] = SHOTS[i].dur;
        if (narration[i] >= 0) shotDur[i] = std::max(shotDur[i], audio::length(narration[i]) + NARRATION_TAIL);
        starts.push_back(total); total += shotDur[i];
    }
    buildInside();
    musicId = audio::load("assets/music.wav");
    if (musicId >= 0) audio::play(musicId, 0.98f, true);
    T = 0; paused = false; done = false; lastShot = -1;
}

float totalTime() { return total; }
bool  finished()  { return done; }
void  seek(float t) { T = std::max(0.0f, std::min(total - 0.01f, t)); lastShot = -1; }

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
    bool whip = (i > 0 && SHOTS[i].trans == 0 && !hasInside(i - 1) && local < TRANSITION);
    if (whip) {                                                          // whip-pan: old shot leaves left, new one arrives right
        float p = easeIO3(local / TRANSITION);
        drawShot(i - 1, shotDur[i - 1] + local, -800.0f * p);
        drawShot(i, local, 800.0f * (1.0f - p));
    } else if (!draw3D(i, local)) {
        drawShot(i, local, 0);
    }
    vignette();
    grain(t);
    if (whip) whipStreaks(i, local / TRANSITION);
    const Shot& s = SHOTS[i];
    if (s.kind == 1 || s.kind == 2) titleCard(local, shotDur[i]);
    else { lowerThird(s, local, shotDur[i]); if (s.yearCard) yearCard(s.yearCard, local); if (s.loc == Loc::BALLOT) turnoutPanel(i, local); }

    // dip through black: fade in at the start of a "time jump" shot, fade out at the end of a shot that is followed by one
    float black = 0.0f;
    if ((s.trans == 1 || hasInside(i - 1)) && i > 0) black = std::max(black, 1.0f - ease(local / DIP));
    black = std::max(black, cutPulse(i, local));
    if (i + 1 < N && (SHOTS[i + 1].trans == 1 || hasInside(i))) black = std::max(black, ease((local - (shotDur[i] - DIP)) / DIP));
    black = std::max(black, std::max(1.0f - ease(t / 1.2f), ease((t - (total - 2.2f)) / 2.2f)));   // film fade in / out
    if (black > 0.001f) render::fillPolygonAlpha(rect(0, 0, 800, 600), {0, 0, 0}, black);
}

}
