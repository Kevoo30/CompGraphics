#include "film.h"
#include "render.h"
#include "transform.h"
#include "animation.h"
#include "locations.h"
#include "font.h"
#include "audio.h"
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
};

//                  sky crowd plac  fist  cand  mourn half  energy seed
#define MOOD(sky,crowd,pl,fi,ca,mo,hm,en,sd) Mood{sky,crowd,pl,fi,ca,mo,hm,en,sd}

const Shot SHOTS[] = {
 {Loc::COURT,       MOOD(5,18,0,0,0,0,0,0.2f,1),  10, "", "", "", 0, false, 1},
 {Loc::BALLOT,      MOOD(0,34,0,0,0,0,0,0.3f,17), 15, "POLLING STATIONS, KENYA",                 "9 AUGUST 2022",          "KENYANS VOTE IN THE GENERAL ELECTION",       2022, false, 0},
 {Loc::BOMAS,       MOOD(0,22,0,0,0,0,0,0.3f,2),  15, "BOMAS OF KENYA, NAIROBI",                 "15 AUGUST 2022",         "PRESIDENTIAL RESULT DECLARED",              0,    false, 0},
 {Loc::COURT,       MOOD(1,26,0,0,0,0,0,0.5f,3),  14, "SUPREME COURT, NAIROBI",                  "5 SEPTEMBER 2022",       "COURT UPHOLDS THE ELECTION RESULT",         0,    true,  0},
 {Loc::STADIUM,     MOOD(1,90,0,0,0,0,0,0.4f,13), 15, "MOI INTERNATIONAL SPORTS CENTRE, KASARANI","13 SEPTEMBER 2022",     "SWEARING-IN OF THE NEW PRESIDENT",          0,    false, 0},
 {Loc::STREET,      MOOD(2,34,0,0,0,0,0,0.3f,14), 14, "NAIROBI",                                 "2023",                   "FUEL AND FOOD TAXES RAISE THE COST OF LIVING", 2023, false, 0},
 {Loc::STREET,      MOOD(2,40,1,0,0,0,0,0.6f,1),  15, "NAIROBI",                                 "7 JULY 2023",            "SABA SABA PROTESTS OVER THE FINANCE ACT",   0,    false, 0},
 {Loc::COURT,       MOOD(2,14,0,0,0,0,0,0.1f,4),  14, "COURTS, NAIROBI",                         "NOVEMBER 2023",          "HIGH COURT RULING ON THE HOUSING LEVY",     0,    false, 0},
 {Loc::PARLIAMENT,  MOOD(1,20,0,0,0,0,0,0.2f,18), 14, "PARLIAMENT, NAIROBI",                     "MAY 2024",               "FINANCE BILL 2024 INTRODUCED IN PARLIAMENT", 2024, false, 0},
 {Loc::STREET,      MOOD(4,62,1,1,0,0,0,0.9f,15), 15, "NAIROBI",                                 "20 JUNE 2024",           "PEOPLE PROTEST PROPOSED TAX RISES",         0,    false, 0},
 {Loc::PARLIAMENT,  MOOD(3,60,1,1,0,0,0,1.0f,5),  18, "PARLIAMENT, NAIROBI",                     "25 JUNE 2024",           "FINANCE BILL PASSES, PROTESTS REACH PARLIAMENT", 0, false, 0},
 {Loc::STATE_HOUSE, MOOD(1,0,0,0,0,0,0,0.0f,6),   14, "STATE HOUSE, NAIROBI",                    "26 JUNE 2024",           "PRESIDENT DECLINES TO SIGN THE BILL",       0,    false, 0},
 {Loc::STATE_HOUSE, MOOD(0,0,0,0,0,0,0,0.0f,7),   14, "STATE HOUSE, NAIROBI",                    "11 JULY 2024",           "CABINET DISMISSED",                         0,    false, 0},
 {Loc::PARLIAMENT,  MOOD(4,16,0,0,0,0,0,0.1f,8),  15, "PARLIAMENT, NAIROBI",                     "17 OCTOBER 2024",        "DEPUTY PRESIDENT REMOVED AFTER SENATE VOTE", 0,   false, 0},
 {Loc::STREET,      MOOD(4,30,0,0,1,1,0,0.1f,2),  16, "NAIROBI",                                 "JUNE 2025",              "DEATH IN POLICE CUSTODY SPARKS PROTESTS",   2025, false, 0},
 {Loc::STREET,      MOOD(3,55,1,1,0,0,0,0.9f,3),  15, "NAIROBI",                                 "25 JUNE AND 7 JULY 2025","ANNIVERSARY AND SABA SABA PROTESTS",        0,    false, 0},
 {Loc::STADIUM,     MOOD(3,90,0,0,0,1,1,0.0f,9),  16, "NYAYO NATIONAL STADIUM, NAIROBI",         "17 OCTOBER 2025",        "STATE FUNERAL OF RAILA ODINGA",             0,    false, 0},
 {Loc::HOMESTEAD,   MOOD(0,24,0,0,0,1,1,0.0f,10), 14, "BONDO, SIAYA COUNTY",                     "19 OCTOBER 2025",        "LAID TO REST WITH MILITARY HONOURS",        0,    false, 0},
 {Loc::PARLIAMENT,  MOOD(3,52,1,0,0,0,0,0.6f,16), 14, "PARLIAMENT, NAIROBI",                     "18 JUNE 2026",           "FINANCE BILL 2026 PASSES AFTER AMENDMENTS",  2026, false, 0},
 {Loc::STREET,      MOOD(1,45,1,0,0,0,0,0.5f,4),  15, "NAIROBI",                                 "25 JUNE 2026",           "TWO YEARS ON, MARCHES MARK THE ANNIVERSARY", 0,   false, 0},
 {Loc::BALLOT,      MOOD(1,30,0,0,0,0,0,0.2f,11), 13, "KENYA",                                   "10 AUGUST 2027",         "THE NEXT GENERAL ELECTION",                 0,    false, 0},
 {Loc::PARLIAMENT,  MOOD(5,70,0,0,0,0,0,0.1f,12), 10, "", "", "", 0, false, 2},
};
const int N = (int)(sizeof(SHOTS) / sizeof(SHOTS[0]));
const float TRANSITION = 0.9f;                    // whip-pan length in seconds

std::vector<float> starts;
std::vector<float> shotDur;                       // real length of each shot: max(table value, narration clip + tail)
const float NARRATION_TAIL = 2.0f;                // quiet time after the voice finishes
float total = 0, T = 0;
bool  paused = false, done = false;
int   lastShot = -1;
std::vector<int> narration;                       // audio ids, -1 when the file is missing
int   musicId = -1;

float ease(float t) { t = std::max(0.0f, std::min(1.0f, t)); return t * t * (3 - 2 * t); }
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

void drawShot(int i, float local, float slideX) {
    const Shot& s = SHOTS[i];
    float p = ease(local / shotDur[i]);
    float zoom = 1.0f + 0.06f * p;                                       // slow push-in
    float pan  = ((i % 2) ? 1.0f : -1.0f) * (p - 0.5f) * 20.0f;          // small alternating drift
    Mat3 M = translate(400 + slideX + pan, 300) * scale(zoom, zoom) * translate(-400, -300);
    drawLocation(s.loc, local, s.mood, M);
}

void titleCard(float local, float dur) {
    float a = fade(local, 0.8f, 2.0f, dur - 1.6f, dur - 0.4f);
    render::fillPolygonAlpha(rect(0, 190, 800, 150), {0, 0, 0}, 0.42f * a);
    font::draw("THE PEOPLE ARE WATCHING", 400, 212, font::fit("THE PEOPLE ARE WATCHING", 740, 5.0f), {255, 255, 255}, a, true);
    font::draw("KENYA 2022 - 2026", 400, 280, 4.0f, {240, 190, 70}, a, true);
}

void yearCard(int year, float local) {
    float a = fade(local, 0.9f, 1.6f, 2.6f, 3.6f);
    if (a <= 0.01f) return;
    char buf[8]; std::snprintf(buf, sizeof(buf), "%d", year);
    render::fillPolygonAlpha(rect(0, 70, 800, 150), {0, 0, 0}, 0.35f * a);
    font::draw(buf, 400, 100, 14.0f, {255, 255, 255}, a, true);
    render::fillPolygonAlpha(rect(300, 206, 200, 4), {240, 190, 70}, a);
}

void lowerThird(const Shot& s, float local, float length) {
    float a = fade(local, 0.9f, 1.5f, length - 1.0f, length - 0.2f);
    if (a <= 0.01f) return;
    render::fillPolygonAlpha(rect(0, 496, 800, 104), {0, 0, 0}, 0.62f * a);
    render::fillPolygonAlpha(rect(0, 496, 8, 104), {240, 190, 70}, a);
    font::draw(s.place,    30, 504, 2.0f, {190, 195, 205}, a);
    font::draw(s.date,     30, 524, font::fit(s.date, 740, 3.0f), {240, 190, 70}, a);
    font::draw(s.headline, 30, 554, font::fit(s.headline, 740, 3.0f), {255, 255, 255}, a);
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
    musicId = audio::load("assets/music.wav");
    if (musicId >= 0) audio::play(musicId, 0.25f, true);
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
    if (i > 0 && local < TRANSITION) {                                   // whip-pan: old shot leaves left, new one arrives right
        float p = ease(local / TRANSITION);
        drawShot(i - 1, shotDur[i - 1] + local, -800.0f * p);
        drawShot(i, local, 800.0f * (1.0f - p));
    } else {
        drawShot(i, local, 0);
    }
    const Shot& s = SHOTS[i];
    if (s.kind == 1 || s.kind == 2) titleCard(local, shotDur[i]);
    else { lowerThird(s, local, shotDur[i]); if (s.yearCard) yearCard(s.yearCard, local); }

    float black = std::max(1.0f - ease(t / 1.0f), ease((t - (total - 1.8f)) / 1.8f));   // fade in at start, out at end
    if (black > 0.001f) render::fillPolygonAlpha(rect(0, 0, 800, 600), {0, 0, 0}, black);
}

}
