#pragma once
#include <vector>

// 3D scenes for the film: the places the 2D exteriors cannot show (inside the court, inside the chamber,
// the polling station, the swearing-in, the protest street). Fixed-function OpenGL, same as render.cpp:
// perspective cameras, depth test, lighting, fog, procedural floor textures, articulated people.
// People are generic stylised figures; no real person is depicted.
namespace world3d {

enum Scene { NONE = 0, COURT, CHAMBER, POLLING, INAUGURATION, PROTEST_STREET, PROTEST_PARLIAMENT, STATE_HOUSE };

struct Barrier { float x, z, r; };            // a road block / burning-tyre barricade (x, z in metres, radius)

struct Params {
    int   crowd    = 40;                      // people in the scene (capped per scene)
    float energy   = 0.5f;                    // 0 calm ... 1 surging
    int   seed     = 1;
    bool  night    = false;
    bool  smoke    = false;                   // tear-gas haze
    bool  police   = false;                   // line of police with shields
    bool  candles  = false;                   // vigil
    bool  mourning = false;                   // dark clothes, bowed heads, flag-draped coffin (INAUGURATION = funeral)
    bool  vote     = false;                   // CHAMBER: members stand and vote
    int   stations = 4;                       // POLLING: number of booths
    int   variant  = 0;                       // STATE_HOUSE: 0 press statement, 1 results declaration
    std::vector<Barrier> barriers;
};

// u = progress 0..1 through this 3D segment (drives the camera), t = seconds since the shot began (drives animation).
// Draws a full frame (clears colour + depth), then puts the 2D state back so the overlays can be drawn on top.
void draw(Scene s, float t, float u, const Params& p, int W, int H);

}
