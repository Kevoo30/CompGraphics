#include "crowd.h"
#include "render.h"
#include "effects.h"
#include <algorithm>
#include <cmath>
#include <vector>

// Person 4: audience of spectators made from the real person model, in three rows (back to front),
// with a different look for each person, idle swaying, and applause: arms up and waving, bouncing,
// and a few people filming with phones that flash.

namespace {

typedef std::vector<Vec2> Poly;
const float kPi = 3.14159265f;

// ---- applause state ----
bool  sustained = false;
float cheerUntil = -1.0f;
float intensity = 0.0f;      // 0 = sitting calmly, 1 = full applause (eases toward the target)
float lastTime = -1.0f;

float hash01(int i, int k) {                 // deterministic pseudo-random 0..1, so the crowd looks the same every run
    float v = std::sin(i * 127.1f + k * 311.7f) * 43758.5453f;
    return v - std::floor(v);
}

struct Spectator {
    Vec2  feet;
    float scale;
    PersonStyle style;
    float phase, freq;     // applause timing, different for everyone
    bool  phone;
};

std::vector<Spectator> crowd;     // built once, already ordered back row -> front row

void build() {
    const Color skins[]   = {{124, 74, 36}, {108, 64, 32}, {116, 69, 34}, {94, 56, 30}, {70, 42, 24}, {84, 50, 27}};
    const Color hairs[]   = {{20, 16, 14}, {40, 28, 20}, {70, 50, 30}, {15, 15, 18}, {110, 80, 45}, {170, 170, 172}};
    const Color suits[]   = {{40, 60, 110}, {120, 30, 30}, {50, 90, 60}, {90, 90, 95}, {160, 120, 40}, {110, 60, 120}, {30, 30, 36}, {200, 200, 205}};
    const Color trousers[]= {{30, 30, 60}, {45, 40, 35}, {25, 25, 28}, {60, 60, 70}};

    struct Row { float y, scale; float x0, dx; int n; };
    // wide enough that panning the camera never reveals an empty lawn
    const Row rows[3] = { {535, 0.38f, -120, 74, 15}, {572, 0.47f, -90, 90, 12}, {612, 0.56f, -130, 104, 11} };

    int id = 0;
    for (int r = 0; r < 3; r++) {
        for (int i = 0; i < rows[r].n; i++, id++) {
            Spectator s;
            s.feet  = { rows[r].x0 + i * rows[r].dx + (hash01(id, 1) - 0.5f) * 24.0f,
                        rows[r].y + (hash01(id, 2) - 0.5f) * 8.0f };
            s.scale = rows[r].scale * (0.92f + 0.16f * hash01(id, 3));
            s.style = { skins[int(hash01(id, 4) * 6) % 6], suits[int(hash01(id, 5) * 8) % 8],
                        {255, 255, 255}, {190, 0, 0}, trousers[int(hash01(id, 6) * 4) % 4],
                        hairs[int(hash01(id, 7) * 6) % 6] };
            s.phase = hash01(id, 8) * 2.0f * kPi;
            s.freq  = 7.0f + 4.0f * hash01(id, 9);
            s.phone = hash01(id, 10) < 0.22f;
            crowd.push_back(s);
        }
    }
}

float lerp(float a, float b, float t) { return a + (b - a) * t; }

Poly circlePoly(Vec2 c, float r, int n = 14) {
    Poly p;
    for (int i = 0; i < n; i++) p.push_back({ c.x + std::cos(2 * kPi * i / n) * r, c.y + std::sin(2 * kPi * i / n) * r });
    return p;
}

} // namespace

void setApplause(bool on)  { sustained = on; }
bool isApplauding()        { return sustained; }
void cheer(float time, float seconds) { cheerUntil = std::max(cheerUntil, time + seconds); }

void drawCrowd(float time, const Mat3& cam) {
    if (crowd.empty()) build();

    // ease the applause intensity toward its target (using the scene clock, so pausing freezes it)
    float dt = (lastTime < 0 || time < lastTime) ? 0.0f : time - lastTime;
    lastTime = time;
    float target = (sustained || time < cheerUntil) ? 1.0f : 0.0f;
    float step = 2.5f * dt;
    intensity += std::max(-step, std::min(step, target - intensity));

    for (const Spectator& s : crowd) {
        // arms: idle = hanging with a slow sway; applause = raised and waving at this person's own speed
        float sway  = 0.05f + 0.03f * std::sin(time * 1.2f + s.phase);
        float wave  = 2.3f + 0.45f * std::sin(time * s.freq + s.phase);
        float waveL = 2.3f + 0.45f * std::sin(time * s.freq + s.phase + kPi);

        effects::Pose pose;
        pose.fromBehind = true;
        pose.leftArm  = lerp(sway, waveL, intensity);
        if (s.phone) pose.rightArm = lerp(sway, 2.55f + 0.03f * std::sin(time * 5.0f + s.phase), intensity);  // holds the phone steady
        else         pose.rightArm = lerp(sway, wave, intensity);

        // bounce on the spot when cheering, plus a tiny breathing motion all the time
        float bob = intensity * std::fabs(std::sin(time * s.freq * 0.5f + s.phase)) * 5.0f * s.scale
                  + 0.5f * s.scale * std::sin(time * 1.6f + s.phase);
        Vec2 feet = { s.feet.x, s.feet.y - bob };

        effects::drawPerson(cam, feet, s.scale, s.style, 1, pose, true);

        // phone in the raised hand, with a camera flash now and then
        if (s.phone && intensity > 0.3f) {
            float r = pose.rightArm + 0.19f;                         // arm points slightly outward even at rest
            Vec2 shoulder = { feet.x + 26.0f * s.scale, feet.y - 147.0f * s.scale };
            Vec2 hand = { shoulder.x + 62.0f * s.scale * std::sin(r), shoulder.y + 62.0f * s.scale * std::cos(r) };
            float pw = 5.0f * s.scale * 2.0f, ph = 9.0f * s.scale * 2.0f;
            Poly phone = { {hand.x - pw / 2, hand.y - ph}, {hand.x + pw / 2, hand.y - ph},
                           {hand.x + pw / 2, hand.y},      {hand.x - pw / 2, hand.y} };
            render::fillPolygon(transformPoly(cam, phone), {25, 25, 30});
            render::drawPolygon(transformPoly(cam, phone), {120, 120, 130});

            float flash = std::fmod(time * 0.45f + s.phase, 2.0f * kPi);          // a short flash every ~14 s per phone
            if (flash < 0.35f) {
                Vec2 c = { hand.x, hand.y - ph };
                render::fillPolygonAlpha(transformPoly(cam, circlePoly(c, 16.0f * s.scale * 2.0f)), {255, 255, 230}, 0.35f);
                render::fillPolygonAlpha(transformPoly(cam, circlePoly(c, 7.0f * s.scale * 2.0f)),  {255, 255, 255}, 0.85f);
            }
        }
    }
}
