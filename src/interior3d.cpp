#include "interior3d.h"
#include <SDL_opengl.h>
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <vector>

namespace world3d {
namespace {

const float PI = 3.14159265f;
struct V3 { float x, y, z; };
V3 operator+(V3 a, V3 b) { return {a.x + b.x, a.y + b.y, a.z + b.z}; }
V3 operator-(V3 a, V3 b) { return {a.x - b.x, a.y - b.y, a.z - b.z}; }
V3 operator*(V3 a, float s) { return {a.x * s, a.y * s, a.z * s}; }
V3 cross(V3 a, V3 b) { return {a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x}; }
V3 norm(V3 a) { float l = std::sqrt(a.x * a.x + a.y * a.y + a.z * a.z); return l > 1e-6f ? a * (1.0f / l) : V3{0, 0, 1}; }
struct C { float r, g, b; };
C mixc(C a, C b, float t) { return {a.r + (b.r - a.r) * t, a.g + (b.g - a.g) * t, a.b + (b.b - a.b) * t}; }
C scalec(C a, float s) { return {a.r * s, a.g * s, a.b * s}; }

float hashf(int n) {
    unsigned x = (unsigned)n * 2654435761u; x ^= x >> 15; x *= 2246822519u; x ^= x >> 13;
    return float(x & 0xFFFF) / 65535.0f;
}
float clamp01(float v) { return std::max(0.0f, std::min(1.0f, v)); }
float smooth(float t) { t = clamp01(t); return t * t * (3 - 2 * t); }
float mixf(float a, float b, float t) { return a + (b - a) * t; }
void col(C c, float a = 1.0f) { glColor4f(c.r, c.g, c.b, a); }

// ------------------------------------------------------------------ primitives
void cube() {
    static const float F[6][4][3] = {
        {{-.5f,-.5f, .5f},{ .5f,-.5f, .5f},{ .5f, .5f, .5f},{-.5f, .5f, .5f}}, {{ .5f,-.5f,-.5f},{-.5f,-.5f,-.5f},{-.5f, .5f,-.5f},{ .5f, .5f,-.5f}},
        {{-.5f,-.5f,-.5f},{-.5f,-.5f, .5f},{-.5f, .5f, .5f},{-.5f, .5f,-.5f}}, {{ .5f,-.5f, .5f},{ .5f,-.5f,-.5f},{ .5f, .5f,-.5f},{ .5f, .5f, .5f}},
        {{-.5f, .5f, .5f},{ .5f, .5f, .5f},{ .5f, .5f,-.5f},{-.5f, .5f,-.5f}}, {{-.5f,-.5f,-.5f},{ .5f,-.5f,-.5f},{ .5f,-.5f, .5f},{-.5f,-.5f, .5f}}};
    static const float N[6][3] = {{0,0,1},{0,0,-1},{-1,0,0},{1,0,0},{0,1,0},{0,-1,0}};
    glBegin(GL_QUADS);
    for (int f = 0; f < 6; f++) { glNormal3fv(N[f]); for (int v = 0; v < 4; v++) glVertex3fv(F[f][v]); }
    glEnd();
}
void box(float x, float y, float z, float w, float h, float d) { glPushMatrix(); glTranslatef(x, y, z); glScalef(w, h, d); cube(); glPopMatrix(); }
void boxc(C c, float x, float y, float z, float w, float h, float d) { col(c); box(x, y, z, w, h, d); }
void ell(float x, float y, float z, float a, float b, float c, int seg = 10) {
    glPushMatrix(); glTranslatef(x, y, z); glScalef(a, b, c);
    int rings = std::max(3, seg / 2);
    for (int j = 0; j < rings; j++) {
        float v0 = -PI / 2 + j * PI / rings, v1 = v0 + PI / rings;
        glBegin(GL_QUAD_STRIP);
        for (int i = 0; i <= seg; i++) {
            float u = i * 2 * PI / seg;
            for (float v : {v0, v1}) { float nx = std::cos(v) * std::cos(u), ny = std::sin(v), nz = std::cos(v) * std::sin(u); glNormal3f(nx, ny, nz); glVertex3f(nx, ny, nz); }
        }
        glEnd();
    }
    glPopMatrix();
}
void cyl(float x, float y, float z, float r, float h, int seg = 14, float r2 = -1.0f) {      // vertical, base at y; r2 >= 0 makes a cone / taper
    if (r2 < 0) r2 = r;
    glBegin(GL_QUAD_STRIP);
    for (int i = 0; i <= seg; i++) { float a = i * 2 * PI / seg, c = std::cos(a), s = std::sin(a); glNormal3f(c, 0, s); glVertex3f(x + c * r, y, z + s * r); glVertex3f(x + c * r2, y + h, z + s * r2); }
    glEnd();
    glBegin(GL_TRIANGLE_FAN); glNormal3f(0, 1, 0); glVertex3f(x, y + h, z);
    for (int i = 0; i <= seg; i++) { float a = i * 2 * PI / seg; glVertex3f(x + std::cos(a) * r2, y + h, z + std::sin(a) * r2); }
    glEnd();
}

// ------------------------------------------------------------------ procedural floor textures
enum Tex { T_WOOD, T_CARPET, T_GRASS, T_ASPHALT, T_TILE, T_COUNT };
GLuint texId[T_COUNT]; bool texReady = false;
void texInit() {
    if (texReady) return; texReady = true;
    const int N = 64; static unsigned char px[N * N * 3];
    for (int k = 0; k < T_COUNT; k++) {
        for (int y = 0; y < N; y++) for (int x = 0; x < N; x++) {
            float n = hashf(x * 131 + y * 71 + k * 997), r, g, b;
            switch (k) {
                case T_WOOD:    { int plank = y / 16; float sh = 0.78f + 0.2f * hashf(plank * 13 + 5), grain = 0.94f + 0.06f * std::sin(x * 0.7f + plank * 3.0f) + 0.04f * n;
                                  bool seam = (y % 16 == 0) || (x == ((plank * 29) % N)); float s = sh * grain * (seam ? 0.55f : 1.0f); r = 0.52f * s; g = 0.33f * s; b = 0.19f * s; break; }
                case T_CARPET:  { float s = 0.8f + 0.2f * n; r = 0.07f * s; g = 0.30f * s; b = 0.15f * s; break; }
                case T_GRASS:   { float stripe = ((y / 8) % 2) ? 1.0f : 0.88f, s = (0.85f + 0.15f * n) * stripe; r = 0.18f * s; g = 0.47f * s; b = 0.16f * s; break; }
                case T_ASPHALT: { float s = 0.7f + 0.3f * n; r = g = b = 0.22f * s; break; }
                default:        { bool d = ((x / 16) + (y / 16)) % 2 == 0; float s = (d ? 0.86f : 0.70f) * (0.96f + 0.04f * n); r = 0.84f * s; g = 0.80f * s; b = 0.72f * s; break; }
            }
            px[(y * N + x) * 3] = (unsigned char)(255 * clamp01(r)); px[(y * N + x) * 3 + 1] = (unsigned char)(255 * clamp01(g)); px[(y * N + x) * 3 + 2] = (unsigned char)(255 * clamp01(b));
        }
        glGenTextures(1, &texId[k]); glBindTexture(GL_TEXTURE_2D, texId[k]);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR); glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameterf(GL_TEXTURE_2D, 0x84FE /*GL_TEXTURE_MAX_ANISOTROPY_EXT, ignored when unsupported*/, 4.0f);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT); glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
        {   // build the whole mip chain by hand (works on any GL version)
            std::vector<unsigned char> cur(px, px + N * N * 3); int sz = N;
            for (int lvl = 0;; lvl++) {
                glTexImage2D(GL_TEXTURE_2D, lvl, GL_RGB, sz, sz, 0, GL_RGB, GL_UNSIGNED_BYTE, cur.data());
                if (sz == 1) break;
                int ns = sz / 2; std::vector<unsigned char> nx(ns * ns * 3);
                for (int y = 0; y < ns; y++) for (int x = 0; x < ns; x++) for (int ch = 0; ch < 3; ch++)
                    nx[(y * ns + x) * 3 + ch] = (unsigned char)((cur[((2 * y) * sz + 2 * x) * 3 + ch] + cur[((2 * y) * sz + 2 * x + 1) * 3 + ch] + cur[((2 * y + 1) * sz + 2 * x) * 3 + ch] + cur[((2 * y + 1) * sz + 2 * x + 1) * 3 + ch]) / 4);
                cur.swap(nx); sz = ns;
            }
        }
    }
}
void floorQuad(Tex k, float x0, float z0, float x1, float z1, float y, float rep, C tint = {1, 1, 1}) {
    texInit(); glEnable(GL_TEXTURE_2D); glBindTexture(GL_TEXTURE_2D, texId[k]); col(tint);
    glBegin(GL_QUADS); glNormal3f(0, 1, 0);
    glTexCoord2f(x0 * rep, z0 * rep); glVertex3f(x0, y, z0); glTexCoord2f(x1 * rep, z0 * rep); glVertex3f(x1, y, z0);
    glTexCoord2f(x1 * rep, z1 * rep); glVertex3f(x1, y, z1); glTexCoord2f(x0 * rep, z1 * rep); glVertex3f(x0, y, z1);
    glEnd(); glDisable(GL_TEXTURE_2D);
}

// ------------------------------------------------------------------ time of day
// One table drives sky, sun, ambient, window glow, street lamps and building brightness in EVERY 3D scene,
// so a shot marked dawn / day / midday / dusk / night / pre-dawn looks that way inside as well as outside.
struct Tod {
    C sky, sun, amb, win;      // sky + fog colour, sun/moon light colour, ambient, light coming through windows and doors
    V3 sunPos;                 // light position
    float sunK;                // 0..1 how much direct sun reaches interiors (light shafts)
    float ik;                  // interior exposure (rooms are lit by lamps but feel darker at night)
    float lamps;               // 0..1 street lamps / floodlights / ceiling lamps on
    float bld;                 // brightness of buildings
    bool night;
};
Tod tod(int s) {
    switch (s) {
        case 0:  return {{.93f,.70f,.52f}, {1.0f,.74f,.50f}, {.40f,.37f,.40f}, {1.0f,.80f,.55f}, {35, 9, 24},  .60f, .90f, .4f, .90f, false};   // dawn: low warm sun
        case 2:  return {{.62f,.78f,.96f}, {1.0f,.98f,.92f}, {.46f,.46f,.46f}, {1.0f,.98f,.90f}, {4, 60, 6},   1.0f, 1.0f, 0.f, 1.0f, false};   // harsh midday
        case 3:  return {{.88f,.50f,.30f}, {1.0f,.56f,.30f}, {.37f,.29f,.31f}, {1.0f,.62f,.32f}, {-34, 9, 12}, .65f, .85f, .8f, .75f, false};   // dusk / evening: orange, lamps coming on
        case 4:  return {{.03f,.04f,.09f}, {.40f,.44f,.62f}, {.17f,.19f,.30f}, {.08f,.12f,.25f}, {-8, 14, 8},  0.f,  .60f, 1.f, .35f, true };   // night: blue moonlight, lamps on
        case 5:  return {{.24f,.29f,.50f}, {.42f,.47f,.68f}, {.25f,.27f,.37f}, {.35f,.45f,.70f}, {-20, 10, -10}, .10f, .70f, .8f, .50f, false};  // pre-dawn: deep blue
        default: return {{.50f,.72f,.94f}, {1.0f,.96f,.85f}, {.50f,.50f,.50f}, {1.0f,.97f,.85f}, {20, 40, 20}, 1.0f, 1.0f, 0.f, 1.0f, false};   // day
    }
}

// ------------------------------------------------------------------ camera, light, atmosphere
void camera(V3 eye, V3 tgt, float fov, int W, int H, float nearP = 0.25f, float farP = 250.0f) {
    glMatrixMode(GL_PROJECTION); glLoadIdentity();
    float top = nearP * std::tan(fov * PI / 360.0f); glFrustum(-top * W / H, top * W / H, -top, top, nearP, farP);
    glMatrixMode(GL_MODELVIEW); glLoadIdentity();
    V3 f = norm(tgt - eye), r = norm(cross(f, {0, 1, 0})), u = cross(r, f);
    float m[16] = {r.x, u.x, -f.x, 0, r.y, u.y, -f.y, 0, r.z, u.z, -f.z, 0, 0, 0, 0, 1};
    glMultMatrixf(m); glTranslatef(-eye.x, -eye.y, -eye.z);
}
V3 shake(float t, float amt, float seed = 0) {            // handheld camera
    return {amt * (std::sin(t * 7.3f + seed) + 0.5f * std::sin(t * 17.1f)), amt * 0.7f * std::sin(t * 9.1f + 2 + seed), amt * 0.5f * std::sin(t * 5.7f + 1)};
}
void begin(int W, int H, C clear, C ambient, bool fog, C fogC, float fogA, float fogB) {
    glViewport(0, 0, W, H);
    glClearColor(clear.r, clear.g, clear.b, 1); glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST); glDepthFunc(GL_LEQUAL); glEnable(GL_NORMALIZE); glEnable(GL_LIGHTING);
    glEnable(GL_COLOR_MATERIAL); glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE); glShadeModel(GL_SMOOTH);
    glEnable(GL_CULL_FACE); glCullFace(GL_BACK); glDisable(GL_CULL_FACE);
    float amb[] = {ambient.r, ambient.g, ambient.b, 1}; glLightModelfv(GL_LIGHT_MODEL_AMBIENT, amb);
    glDisable(GL_LIGHT0); glDisable(GL_LIGHT1); glDisable(GL_LIGHT2);
    if (fog) { float fc[] = {fogC.r, fogC.g, fogC.b, 1}; glEnable(GL_FOG); glFogi(GL_FOG_MODE, GL_LINEAR); glFogfv(GL_FOG_COLOR, fc); glFogf(GL_FOG_START, fogA); glFogf(GL_FOG_END, fogB); }
    else glDisable(GL_FOG);
}
void light(int n, V3 pos, C c, float atten = 0.0f) {
    GLenum L = GL_LIGHT0 + n; float p[] = {pos.x, pos.y, pos.z, 1}, d[] = {c.r, c.g, c.b, 1}, z[] = {0, 0, 0, 1};
    glEnable(L); glLightfv(L, GL_POSITION, p); glLightfv(L, GL_DIFFUSE, d); glLightfv(L, GL_SPECULAR, z); glLightfv(L, GL_AMBIENT, z);
    glLightf(L, GL_CONSTANT_ATTENUATION, 1.0f); glLightf(L, GL_QUADRATIC_ATTENUATION, atten);
}
void end2D(int W, int H) {                                // put the 2D renderer's state back
    glDisable(GL_LIGHTING); glDisable(GL_LIGHT0); glDisable(GL_LIGHT1); glDisable(GL_LIGHT2); glDisable(GL_FOG); glDisable(GL_DEPTH_TEST);
    glDisable(GL_COLOR_MATERIAL); glDisable(GL_TEXTURE_2D); glDisable(GL_NORMALIZE); glDisable(GL_CULL_FACE);
    glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); glShadeModel(GL_SMOOTH);
    float amb[] = {0.2f, 0.2f, 0.2f, 1}; glLightModelfv(GL_LIGHT_MODEL_AMBIENT, amb);
    glMatrixMode(GL_PROJECTION); glLoadIdentity(); glOrtho(0, W, H, 0, -1, 1); glMatrixMode(GL_MODELVIEW); glLoadIdentity();
    glColor4f(1, 1, 1, 1);
}
// soft translucent things (light shafts, smoke): no lighting, no depth writes
void softBegin() { glDisable(GL_LIGHTING); glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); glDepthMask(GL_FALSE); }
void softEnd()   { glDepthMask(GL_TRUE); glEnable(GL_LIGHTING); }
void shaft(V3 a0, V3 a1, V3 b0, V3 b1, C c, float alpha) {  // slanted beam of light made of two quads
    softBegin(); glBlendFunc(GL_SRC_ALPHA, GL_ONE);
    glBegin(GL_QUADS); col(c, alpha); glVertex3f(a0.x, a0.y, a0.z); glVertex3f(a1.x, a1.y, a1.z); col(c, 0.0f); glVertex3f(b1.x, b1.y, b1.z); glVertex3f(b0.x, b0.y, b0.z); glEnd();
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); softEnd();
}
void dust(V3 centre, V3 size, float t, int n, float seed) {
    softBegin(); glPointSize(2.2f); glBegin(GL_POINTS);
    for (int i = 0; i < n; i++) {
        float a = hashf(i * 3 + (int)seed), b = hashf(i * 5 + 7), c = hashf(i * 11 + 3);
        glColor4f(1, 0.95f, 0.8f, 0.25f + 0.2f * std::sin(t * 1.5f + i));
        glVertex3f(centre.x + (a - 0.5f) * size.x + std::sin(t * 0.3f + i) * 0.2f, centre.y + (b - 0.5f) * size.y - std::fmod(t * 0.05f * (1 + c) + c, 1.0f) * 0.4f, centre.z + (c - 0.5f) * size.z);
    }
    glEnd(); softEnd();
}
void puff(V3 p, float r, C c, float alpha) {              // one smoke blob: three crossing translucent discs facing every way
    softBegin();
    for (int k = 0; k < 3; k++) {
        glPushMatrix(); glTranslatef(p.x, p.y, p.z); glRotatef(k * 60.0f, 0, 1, 0);
        glBegin(GL_TRIANGLE_FAN); col(c, alpha); glVertex3f(0, 0, 0); col(c, 0.0f);
        for (int i = 0; i <= 14; i++) { float a = i * 2 * PI / 14; glVertex3f(std::cos(a) * r, std::sin(a) * r * 0.8f, 0); }
        glEnd(); glPopMatrix();
    }
    softEnd();
}

// ------------------------------------------------------------------ people
struct Look { C skin, top, bottom, hair; int kind; float h; bool female = false; };   // kind: 0 casual 1 robe 2 suit 3 police 4 dress 5 uniform 6 gown(lawyer) 7 hi-vis
// One consistent family of dark-brown tones for everybody (the old palette included light tones, which made faces jump from scene to scene).
C skinTone(int id) { static const C s[5] = {{.48f,.29f,.15f},{.41f,.25f,.13f},{.37f,.22f,.12f},{.29f,.17f,.10f},{.44f,.27f,.14f}}; return s[int(hashf(id * 7 + 1) * 5) % 5]; }
Look makeLook(int id, int kind, bool dark = false) {
    static const C tops[10] = {{.16f,.24f,.43f},{.59f,.16f,.16f},{.2f,.4f,.25f},{.35f,.35f,.4f},{.67f,.51f,.18f},{.27f,.18f,.4f},{.8f,.8f,.82f},{.14f,.14f,.16f},{.78f,.43f,.16f},{.24f,.47f,.59f}};
    static const C bots[5] = {{.14f,.18f,.31f},{.12f,.12f,.14f},{.25f,.22f,.2f},{.19f,.24f,.36f},{.1f,.1f,.11f}};
    Look L; L.skin = skinTone(id); L.kind = kind; L.h = 0.94f + 0.12f * hashf(id * 3 + 2); L.female = (kind == 4) || hashf(id * 23) < 0.18f;
    L.top = dark ? (hashf(id * 5) < 0.7f ? C{.1f,.1f,.12f} : C{.8f,.8f,.82f}) : tops[int(hashf(id * 11 + 4) * 10) % 10];
    L.bottom = bots[int(hashf(id * 13 + 6) * 5) % 5];
    L.hair = hashf(id * 17) < 0.1f ? C{.55f,.55f,.56f} : C{.07f,.055f,.05f};
    switch (kind) {
        case 1: L.top = {.09f, .07f, .08f}; L.bottom = L.top; break;                         // judge: dark robe
        case 2: { static const C suits[6] = {{.1f,.12f,.2f},{.14f,.14f,.15f},{.22f,.16f,.12f},{.08f,.2f,.14f},{.3f,.1f,.14f},{.17f,.19f,.3f}}; L.top = suits[int(hashf(id * 19) * 6) % 6]; L.bottom = L.top; break; }
        case 3: L.top = {.1f, .15f, .3f}; L.bottom = {.08f, .1f, .2f}; break;                // police navy
        case 5: L.top = {.045f, .045f, .05f}; L.bottom = {.08f, .24f, .12f}; break;           // Kenyan ceremonial guard colors
        case 6: L.top = {.05f, .05f, .06f}; L.bottom = L.top; break;                         // advocate's black gown
        case 7: L.top = {.85f, .8f, .1f}; break;                                             // hi-vis vest (election officials)
    }
    return L;
}
// Recurring characters have ONE fixed look, whatever scene or time of day they appear in.
// The president is always dark-skinned, in the same navy suit, with short black hair.
const C PRESIDENT_SKIN = {.24f, .14f, .085f};
Look presidentLook(int /*unused: kept so every call site draws the same man*/ = 0) {
    Look L = makeLook(9001, 2);
    L.skin = PRESIDENT_SKIN; L.top = L.bottom = {.07f, .09f, .17f}; L.hair = {.05f, .045f, .045f}; L.h = 1.0f; L.female = false;
    return L;
}
Look chiefJusticeLook() { Look L = makeLook(9002, 1); L.skin = {.33f, .20f, .11f}; L.hair = {.45f, .45f, .47f}; L.h = 1.0f; L.female = false; return L; }
Look speakerLook()      { Look L = makeLook(9003, 1); L.skin = {.31f, .19f, .11f}; L.hair = {.9f, .9f, .92f};    L.h = 1.0f; L.female = false; return L; }

void decalBegin(float n); void decalEnd(); void kenyaShield(float x, float y, float z, float w, float h, float g, bool detail);

// arm poses: 0 down  1 raised fist  2 placard  3 writing / hands forward  4 oath (right hand up, left on book)
//            5 clapping  6 phone  7 hand raised (voting)  8 speaking gesture  9 candle  10 riot shield  11 salute
void person(float x, float y, float z, float yaw, const Look& L, float ph, float walk, int arm, bool sit, float t, int id = 0) {
    glPushMatrix(); glTranslatef(x, y, z); glRotatef(yaw * 180.0f / PI, 0, 1, 0); glScalef(L.h, L.h, L.h);
    float hip = sit ? 0.52f : 0.9f;
    bool robe = (L.kind == 1 || L.kind == 6 || L.kind == 4);
    // legs
    for (int s = -1; s <= 1; s += 2) {
        float sw = std::sin(ph + (s > 0 ? PI : 0.0f)) * 0.6f * walk;
        glPushMatrix(); glTranslatef(s * 0.1f, hip, 0);
        col(robe ? L.bottom : L.bottom);
        if (sit) { glPushMatrix(); glRotatef(-90, 1, 0, 0); box(0, -0.21f, 0, 0.14f, 0.42f, 0.14f); glPopMatrix(); glTranslatef(0, 0, 0.42f); box(0, -0.25f, 0, 0.12f, 0.5f, 0.12f); col({.06f,.05f,.05f}); box(0, -0.5f, 0.06f, 0.13f, 0.07f, 0.25f); }
        else { glRotatef(sw * 57.3f, 1, 0, 0); box(0, -hip / 2, 0, 0.14f, hip, 0.14f); col({.06f,.05f,.05f}); box(0, -hip + 0.02f, 0.05f, 0.13f, 0.07f, 0.27f); }
        glPopMatrix();
    }
    if (robe && !sit) { col(L.top); box(0, hip - 0.05f, 0, 0.5f, 0.85f, 0.32f); }
    if (robe && sit)  { col(L.top); box(0, hip + 0.02f, 0.18f, 0.5f, 0.1f, 0.5f); }
    // torso
    col(L.top); box(0, hip + 0.3f, 0, 0.4f, 0.6f, 0.22f);
    if (L.kind == 1) { col({.62f, .08f, .1f}); box(0, hip + 0.45f, 0.115f, 0.1f, 0.4f, 0.02f); box(0, hip + 0.56f, 0.115f, 0.3f, 0.05f, 0.02f); }          // red sash
    if (L.kind == 3 || L.kind == 5) {
        // Chest badge = the same Kenya shield drawn on the coat of arms and the flag (black top, red middle, green bottom, white fimbriation).
        decalBegin(1.0f); kenyaShield(-0.1f, hip + 0.43f, 0.118f, 0.13f, 0.24f, 0.0016f, false); decalEnd();
    }
    if (L.kind == 6) { col({.95f, .95f, .95f}); box(-0.03f, hip + 0.55f, 0.115f, 0.05f, 0.14f, 0.02f); box(0.03f, hip + 0.55f, 0.115f, 0.05f, 0.14f, 0.02f); }  // white bands
    if (L.kind == 2) { col({.95f, .95f, .95f}); box(0, hip + 0.5f, 0.115f, 0.1f, 0.2f, 0.02f); col({.55f, .08f, .1f}); box(0, hip + 0.45f, 0.125f, 0.04f, 0.3f, 0.02f); }
    if (L.kind == 7) { col({.9f, .9f, .9f}); box(0, hip + 0.3f, 0.115f, 0.4f, 0.04f, 0.02f); }
    if (L.kind == 3) { col({.8f, .8f, .8f}); box(0, hip + 0.3f, 0.115f, 0.04f, 0.5f, 0.02f); }
    // arms
    float a[2], b[2];
    for (int s = 0; s < 2; s++) { a[s] = (s ? 1 : -1) * std::sin(ph) * 0.5f * walk; b[s] = -0.12f; }
    switch (arm) {
        case 1: a[1] = -2.7f + 0.25f * std::sin(t * 5 + ph); b[1] = -0.2f; break;
        case 2: a[1] = -2.55f; b[1] = -0.25f; break;
        case 3: a[0] = a[1] = -0.8f; b[0] = b[1] = -1.0f; break;
        case 4: a[1] = -2.95f; b[1] = -0.1f; a[0] = -1.0f; b[0] = -1.15f; break;
        case 5: a[0] = a[1] = -0.95f; b[0] = b[1] = -1.35f + 0.3f * std::sin(t * 9 + ph); break;
        case 6: a[1] = -1.7f; b[1] = -1.5f; break;
        case 7: a[1] = -2.8f; b[1] = -0.1f; break;
        case 8: a[1] = -1.1f + 0.5f * std::sin(t * 2.1f + ph); b[1] = -0.9f + 0.3f * std::sin(t * 3.3f + ph); a[0] = -0.3f; break;
        case 9: a[1] = -0.8f; b[1] = -1.1f; break;
        case 10: a[0] = -1.0f; b[0] = -0.9f; a[1] = -0.6f; b[1] = -0.6f; break;
        case 11: a[1] = -2.0f; b[1] = -2.2f; break;
        case 12: a[1] = -1.75f; b[1] = -1.0f; break;
    }
    for (int s = 0; s < 2; s++) {
        float side = s ? 1.0f : -1.0f;
        glPushMatrix(); glTranslatef(side * 0.25f, hip + 0.56f, 0); glRotatef(a[s] * 57.3f, 1, 0, 0);
        col(L.top); box(0, -0.15f, 0, 0.1f, 0.3f, 0.1f);
        glTranslatef(0, -0.3f, 0); glRotatef(b[s] * 57.3f, 1, 0, 0);
        col(robe || L.kind == 2 || L.kind == 3 ? L.top : L.skin); box(0, -0.14f, 0, 0.09f, 0.28f, 0.09f);
        col(L.skin); ell(0, -0.3f, 0, 0.05f, 0.06f, 0.05f, 6);
        if (s == 1 && arm == 9) { col({.95f, .92f, .8f}); box(0, -0.2f, 0.05f, 0.04f, 0.16f, 0.04f); glDisable(GL_LIGHTING); col({1, .8f, .3f}); ell(0, -0.08f, 0.05f, 0.03f, 0.045f + 0.01f * std::sin(t * 12 + ph), 0.03f, 5); glEnable(GL_LIGHTING); }
        if (s == 1 && arm == 12) { col({.38f, .12f, .62f}); ell(0, -0.37f, 0.0f, 0.03f, 0.055f, 0.03f, 6); }
        if (s == 1 && arm == 6) { col({.05f, .05f, .07f}); box(0, -0.32f, 0.04f, 0.07f, 0.13f, 0.015f); }
        if (s == 0 && arm == 4) { col({.12f, .1f, .08f}); box(0.0f, -0.36f, 0.05f, 0.22f, 0.03f, 0.3f); col({.95f, .9f, .8f}); box(0, -0.345f, 0.05f, 0.2f, 0.01f, 0.28f); }   // the book
        glPopMatrix();
    }
    if (arm == 2) {                                         // placard on a stick above the raised hand
        C pc = hashf(id * 7 + 1) < 0.5f ? C{.95f, .95f, .92f} : (hashf(id * 7 + 1) < 0.75f ? C{.95f, .8f, .25f} : C{.85f, .3f, .25f});
        col({.45f, .32f, .2f}); box(0.3f, hip + 1.15f, 0.0f, 0.03f, 0.9f, 0.03f);
        col(pc); box(0.3f, hip + 1.72f, 0.0f, 0.78f, 0.52f, 0.025f);
        col({.1f, .08f, .08f}); for (int q = 0; q < 3; q++) box(0.3f - 0.05f * q, hip + 1.86f - q * 0.14f, 0.02f, 0.58f - 0.12f * q, 0.045f, 0.01f);
    }
    if (arm == 10) { glDisable(GL_LIGHTING); glEnable(GL_BLEND); col({.7f, .85f, 1.0f}, 0.45f); box(-0.15f, hip + 0.35f, 0.32f, 0.6f, 1.0f, 0.03f); col({.9f, .95f, 1.0f}, 0.9f); box(-0.15f, hip + 0.86f, 0.32f, 0.62f, 0.03f, 0.04f); glEnable(GL_LIGHTING); }
    // head
    float hd = hip + 0.76f;
    col(L.skin); box(0, hip + 0.64f, 0, 0.09f, 0.1f, 0.09f);
    ell(0, hd, 0.01f, 0.115f, 0.135f, 0.12f, 14);
    col(L.hair);
    if (L.kind == 3) { col({.08f, .12f, .26f}); ell(0, hd + 0.06f, 0.0f, 0.125f, 0.09f, 0.13f, 8); box(0, hd + 0.03f, 0.12f, 0.2f, 0.02f, 0.08f); }
    else if (L.kind == 5) { col({.035f, .035f, .04f}); ell(0, hd + 0.07f, 0.0f, 0.125f, 0.075f, 0.13f, 8); }
    else if (L.female) { ell(0, hd + 0.03f, -0.02f, 0.14f, 0.15f, 0.13f, 8); col(L.skin); ell(0, hd - 0.01f, 0.05f, 0.1f, 0.115f, 0.085f, 8); }
    else ell(0, hd + 0.075f, -0.005f, 0.12f, 0.075f, 0.125f, 8);
    // face: same layout on every person (eyes, brows, nose, mouth, ears), drawn after the hair so it is always visible
    { float fz = L.female ? 0.138f : 0.126f;
      col({.97f, .96f, .93f}); box(-0.04f, hd + 0.02f, fz, 0.034f, 0.016f, 0.006f); box(0.04f, hd + 0.02f, fz, 0.034f, 0.016f, 0.006f);      // eye whites
      col({.03f, .02f, .02f}); box(-0.04f, hd + 0.02f, fz + 0.004f, 0.016f, 0.014f, 0.006f); box(0.04f, hd + 0.02f, fz + 0.004f, 0.016f, 0.014f, 0.006f);
      col(L.kind == 3 || L.kind == 5 ? C{.03f, .03f, .035f} : L.hair); box(-0.04f, hd + 0.05f, fz, 0.04f, 0.009f, 0.006f); box(0.04f, hd + 0.05f, fz, 0.04f, 0.009f, 0.006f);   // brows
      col(scalec(L.skin, 0.86f)); box(0, hd - 0.012f, fz + 0.008f, 0.026f, 0.044f, 0.024f);                                                   // nose
      col({.22f, .08f, .07f}); box(0, hd - 0.06f, fz - 0.004f, 0.05f, 0.009f, 0.006f);                                                       // mouth
      col(L.skin); ell(-0.115f, hd, 0.0f, 0.016f, 0.034f, 0.024f, 6); ell(0.115f, hd, 0.0f, 0.016f, 0.034f, 0.024f, 6); }
    glPopMatrix();
}
void chair(float x, float y, float z, float yaw, C c, float back = 0.9f) {
    glPushMatrix(); glTranslatef(x, y, z); glRotatef(yaw * 180.0f / PI, 0, 1, 0);
    col(c); box(0, 0.45f, 0, 0.5f, 0.07f, 0.5f); box(0, 0.45f + back / 2, -0.22f, 0.5f, back, 0.07f);
    col(scalec(c, 0.5f)); for (int i = 0; i < 4; i++) box((i % 2 ? 1 : -1) * 0.22f, 0.22f, (i < 2 ? 1 : -1) * 0.22f, 0.05f, 0.44f, 0.05f);
    glPopMatrix();
}
// ---- flat decal helpers: every emblem layer is a flat plate with a REAL gap to the layer below + a polygon offset,
//      so nothing is ever coplanar or intersecting (that was the flicker while the camera turned).
void decalBegin(float n) { glEnable(GL_POLYGON_OFFSET_FILL); glPolygonOffset(-n, -2.0f * n); }
void decalEnd()          { glDisable(GL_POLYGON_OFFSET_FILL); }
void disc2(float cx, float cy, float z, float a, float b, C c, int seg = 18, float rotDeg = 0.0f) {
    float cr = std::cos(rotDeg * PI / 180.0f), sr = std::sin(rotDeg * PI / 180.0f);
    glNormal3f(0, 0, 1); col(c); glBegin(GL_TRIANGLE_FAN); glVertex3f(cx, cy, z);
    for (int i = 0; i <= seg; i++) { float an = i * 2 * PI / seg, px = std::cos(an) * a, py = std::sin(an) * b; glVertex3f(cx + px * cr - py * sr, cy + px * sr + py * cr, z); }
    glEnd();
}
void quad2(float z, C c, float x0, float y0, float x1, float y1, float x2, float y2, float x3, float y3) {
    glNormal3f(0, 0, 1); col(c); glBegin(GL_QUADS); glVertex3f(x0, y0, z); glVertex3f(x1, y1, z); glVertex3f(x2, y2, z); glVertex3f(x3, y3, z); glEnd();
}
void bar2(float z, C c, float x0, float y0, float x1, float y1, float wd) {      // thick line from (x0,y0) to (x1,y1)
    float dx = x1 - x0, dy = y1 - y0, l = std::sqrt(dx * dx + dy * dy); if (l < 1e-6f) return; float nx = -dy / l * wd * 0.5f, ny = dx / l * wd * 0.5f;
    quad2(z, c, x0 + nx, y0 + ny, x1 + nx, y1 + ny, x1 - nx, y1 - ny, x0 - nx, y0 - ny);
}
float shieldHalfW(float v) { float cc = std::cos(clamp01(std::fabs(v)) * PI * 0.5f); return std::pow(cc, 0.5f); }       // v in -1..1 up the shield: 1 in the middle, 0 at the tips
void shieldBand(float x, float y, float z, float w, float h, float v0, float v1, C c, float inset) {
    const int N = 10; glNormal3f(0, 0, 1); col(c); glBegin(GL_QUAD_STRIP);
    for (int i = 0; i <= N; i++) { float v = v0 + (v1 - v0) * i / N, hw = std::max(0.0f, shieldHalfW(v) * 0.5f * w - inset), yy = y + v * 0.5f * h; glVertex3f(x - hw, yy, z); glVertex3f(x + hw, yy, z); }
    glEnd();
}
// THE Kenya shield, used by the wall coat of arms and by the guards' and police chest badges, so it is identical in every scene:
// black (top), red (middle), green (bottom), separated and framed by white fimbriation. (x, y) = centre, z = back layer, g = gap between layers.
// With detail = true the red band carries the cockerel holding an axe.
void kenyaShield(float x, float y, float z, float w, float h, float g, bool detail) {
    const C white = {.95f, .94f, .90f}, black = {.04f, .04f, .05f}, red = {.74f, .08f, .10f}, green = {.06f, .40f, .18f};
    float ins = w * 0.05f;
    shieldBand(x, y, z, w, h, -1.0f, 1.0f, white, 0);                          // white rim, also shows between the bands
    shieldBand(x, y, z + g, w, h,  0.37f,  1.0f, black, ins);                  // black (top)
    shieldBand(x, y, z + g, w, h, -0.27f,  0.27f, red,   ins);                 // red (middle)
    shieldBand(x, y, z + g, w, h, -1.0f, -0.37f, green, ins);                  // green (bottom)
    if (!detail) return;
    float s = h * 0.10f, z2 = z + 2 * g, z3 = z + 3 * g; const C gold = {.92f, .72f, .2f};
    disc2(x - s * 0.55f, y + s * 0.35f, z2, s * 0.25f, s * 0.6f, white, 10, 20.0f);        // tail
    disc2(x, y - s * 0.1f, z2, s * 0.6f, s * 0.75f, white, 14);                             // body
    disc2(x + s * 0.15f, y + s * 0.85f, z2, s * 0.25f, s * 0.28f, white, 12);               // head
    disc2(x + s * 0.12f, y + s * 1.15f, z2, s * 0.1f, s * 0.12f, gold, 8);                  // comb
    disc2(x + s * 0.42f, y + s * 0.88f, z2, s * 0.1f, s * 0.05f, gold, 6);                  // beak
    bar2(z2, {.45f, .30f, .15f}, x + s * 0.45f, y - s * 0.6f, x + s * 0.8f, y + s * 0.6f, s * 0.07f);      // axe handle
    quad2(z3, {.75f, .76f, .8f}, x + s * 0.72f, y + s * 0.5f, x + s * 1.1f, y + s * 0.62f, x + s * 1.1f, y + s * 0.9f, x + s * 0.76f, y + s * 0.82f);   // axe head
    disc2(x + s * 0.18f, y + s * 0.9f, z3, s * 0.04f, s * 0.04f, black, 6);                 // eye
}
void spear(float x0, float y0, float x1, float y1, float z, float wd) {
    float dx = x1 - x0, dy = y1 - y0, l = std::sqrt(dx * dx + dy * dy); dx /= l; dy /= l;
    bar2(z, {.45f, .30f, .15f}, x0, y0, x1, y1, wd);
    quad2(z, {.82f, .82f, .85f}, x1 - dy * wd * 1.4f, y1 + dx * wd * 1.4f, x1 + dy * wd * 1.4f, y1 - dx * wd * 1.4f, x1 + dx * wd * 5.0f, y1 + dy * wd * 5.0f, x1 + dx * wd * 5.0f, y1 + dy * wd * 5.0f);
}
void lion(float x, float y, float z, float r, float s, float g) {            // flat gold supporter facing the shield (s = -1 left, +1 right)
    const C gold = {.84f, .64f, .18f}, dark = {.62f, .45f, .12f};
    float cx = x + s * 1.0f * r;
    disc2(cx, y + 0.42f * r, z, 0.25f * r, 0.27f * r, dark, 14);                                               // mane
    disc2(cx, y - 0.25f * r, z + g, 0.26f * r, 0.55f * r, gold, 16, -s * 10.0f);                               // body
    bar2(z + g, gold, cx - s * 0.1f * r, y - 0.5f * r, cx - s * 0.14f * r, y - 1.0f * r, 0.11f * r);           // hind legs
    bar2(z + g, gold, cx + s * 0.1f * r, y - 0.5f * r, cx + s * 0.16f * r, y - 1.0f * r, 0.11f * r);
    bar2(z + g, gold, cx - s * 0.12f * r, y + 0.12f * r, cx - s * 0.47f * r, y + 0.35f * r, 0.09f * r);        // raised paw towards the shield
    bar2(z + g, dark, cx + s * 0.2f * r, y - 0.45f * r, cx + s * 0.42f * r, y + 0.1f * r, 0.05f * r);          // tail
    disc2(cx + s * 0.42f * r, y + 0.13f * r, z + g, 0.07f * r, 0.09f * r, dark, 8);
    disc2(cx - s * 0.04f * r, y + 0.45f * r, z + 2 * g, 0.16f * r, 0.18f * r, gold, 14);                       // head
    disc2(cx - s * 0.1f * r, y + 0.5f * r, z + 3 * g, 0.025f * r, 0.025f * r, {.05f, .04f, .03f}, 6);        // eye
}
void flag3(float x, float y, float z, float w, float t, float seed) {   // identical design to the 2D flag in locations.cpp, waving
    const int NX = 24, NY = 30; const float h = w * 46.0f / 74.0f;       // same 74:46 proportions as the outside flags
    auto waveAt = [&](float u) { return std::sin(t * 2.2f + u * 5.4f + seed) * 0.05f * w * (0.15f + u); };   // pinned at the pole, free at the fly
    auto flagColor = [&](float u, float v) -> C {
        const C white = {.93f, .93f, .93f};
        float dx = (u - 0.5f) * w, dy = (v - 0.5f) * h;
        float sa = dx / (0.18f * w), sb = dy / (0.30f * h);                 // crossed white spears: the two diagonals of the 2D flag
        if (std::fabs(sa) <= 1.0f && std::fabs(std::fabs(sa) - std::fabs(sb)) < 0.09f) return white;
        float e = (dx * dx) / (0.10f * w * 0.10f * w) + (dy * dy) / (0.30f * h * 0.30f * h);
        if (e <= 1.0f) return e > 0.78f ? white : C{.76f, .11f, .11f};      // shield: red with white outline
        if (v < 0.27f) return {.05f, .05f, .05f};                           // black
        if (v < 0.31f) return white;
        if (v < 0.69f) return {.67f, .09f, .09f};                           // red
        if (v < 0.73f) return white;
        return {.08f, .45f, .20f};                                          // green
    };
    for (int j = 0; j < NY; j++) for (int i = 0; i < NX; i++) {             // one continuous cloth mesh: no overlapping boxes, no seams
        float u0 = (float)i / NX, u1 = (float)(i + 1) / NX, v0 = (float)j / NY, v1 = (float)(j + 1) / NY;
        float dz = (waveAt(u1) - waveAt(u0)) / (w / NX); V3 n = norm({-dz, 0, 1});
        col(flagColor((u0 + u1) * 0.5f, (v0 + v1) * 0.5f)); glNormal3f(n.x, n.y, n.z);
        glBegin(GL_QUADS);
        glVertex3f(x + u0 * w, y - v0 * h, z + waveAt(u0)); glVertex3f(x + u1 * w, y - v0 * h, z + waveAt(u1));
        glVertex3f(x + u1 * w, y - v1 * h, z + waveAt(u1)); glVertex3f(x + u0 * w, y - v1 * h, z + waveAt(u0));
        glEnd();
    }
    col({.4f, .4f, .42f}); box(x - 0.03f, y - h / 2 - 0.5f, z, 0.06f, h + 1.4f, 0.06f);
    col({.84f, .70f, .27f}); ell(x - 0.03f, y + 0.26f, z, 0.07f, 0.07f, 0.07f, 8);                  // gold ball on top, like the outside poles
}
// The wall coat of arms: dark plaque, Mount Kenya, crossed spears, two lions, scroll, and the black/red/green shield with the cockerel.
// (x, y) = centre of the shield, r = overall scale. All layers are flat plates stacked with real gaps (see decalBegin).
void coatOfArms(float x, float y, float z, float r) {
    float g = r * 0.014f; decalBegin(1.0f);
    disc2(x, y - 0.1f * r, z, 1.45f * r, 1.75f * r, {.09f, .08f, .07f}, 30);                                           // plaque
    disc2(x, y - 1.55f * r, z + g, 1.15f * r, 0.2f * r, {.25f, .30f, .18f}, 24);                                       // Mount Kenya compartment
    disc2(x, y - 1.5f * r, z + 2 * g, 0.3f * r, 0.12f * r, {.9f, .9f, .92f}, 12);                                       // snow cap
    spear(x - 0.65f * r, y - 1.15f * r, x + 0.5f * r, y + 1.25f * r, z + g, 0.05f * r);                                 // crossed spears behind the shield
    spear(x + 0.65f * r, y - 1.15f * r, x - 0.5f * r, y + 1.25f * r, z + g, 0.05f * r);
    lion(x, y, z + g, r, -1.0f, g); lion(x, y, z + g, r, 1.0f, g);
    quad2(z + 2 * g, {.9f, .84f, .65f}, x - 1.15f * r, y - 1.28f * r, x + 1.15f * r, y - 1.28f * r, x + 1.15f * r, y - 1.0f * r, x - 1.15f * r, y - 1.0f * r);   // scroll
    for (int k = 0; k < 9; k++) quad2(z + 3 * g, {.25f, .12f, .1f}, x - 0.9f * r + k * 0.225f * r, y - 1.2f * r, x - 0.8f * r + k * 0.225f * r, y - 1.2f * r, x - 0.8f * r + k * 0.225f * r, y - 1.08f * r, x - 0.9f * r + k * 0.225f * r, y - 1.08f * r);
    kenyaShield(x, y, z + 4 * g, 1.05f * r, 1.6f * r, g, true);
    decalEnd();
}
void lightPanel(float x, float y, float z, float w, float d, C c) {
    glDisable(GL_LIGHTING); col(c); box(x, y, z, w, 0.05f, d); glEnable(GL_LIGHTING);
}
void wallWindow(float x, float y, float z, float w, float h, bool alongX, C wc = {1.0f, .97f, .85f}) {
    glDisable(GL_LIGHTING); col(wc); if (alongX) box(x, y, z, w, h, 0.05f); else box(x, y, z, 0.05f, h, w); glEnable(GL_LIGHTING);
}

// ================================================================== COURT: inside the Supreme Court
void sceneCourt(float t, float u, const Params& p, int W, int H) {
    Tod d = tod(p.sky);
    begin(W, H, {.05f, .04f, .04f}, scalec({.34f, .30f, .26f}, d.ik), false, {0, 0, 0}, 0, 0);
    light(0, {0, 6.4f, -3}, scalec({.95f, .86f, .7f}, mixf(1.0f, d.ik, 0.5f))); light(1, {0, 3.0f, 12}, scalec(mixc({.35f, .38f, .45f}, d.sun, 0.5f), 0.75f));
    V3 eye, tgt;
    if (u < 0.5f)       { float v = smooth(u * 2); eye = {mixf(-2.0f, 0.5f, v), mixf(2.9f, 2.5f, v), mixf(11.6f, 6.0f, v)}; tgt = {0, 2.3f, -9}; }
    else if (u < 0.78f) { float v = (u - 0.5f) / 0.28f; eye = {mixf(-4.4f, -2.2f, v), 1.55f, mixf(-0.8f, -3.2f, v)}; tgt = {mixf(-2.0f, 2.0f, v), 2.1f, -8.8f}; }
    else                { float v = (u - 0.78f) / 0.22f; eye = {mixf(3.4f, 2.4f, v), 2.15f, -5.3f}; tgt = {mixf(0.4f, 0.0f, v), 1.95f, -9.0f}; }
    camera(eye + shake(t, 0.015f), tgt, 54, W, H);
    floorQuad(T_WOOD, -8, -12, 8, 12.5f, 0, 0.45f);
    C wood = {.24f, .14f, .08f}, dark = {.15f, .085f, .05f};
    boxc(wood, -8, 3.5f, 0, 0.3f, 7, 25); boxc(wood, 8, 3.5f, 0, 0.3f, 7, 25); boxc(wood, 0, 3.5f, -12.2f, 16.6f, 7, 0.3f); boxc(wood, 0, 3.5f, 12.7f, 16.6f, 7, 0.3f);
    boxc({.85f, .82f, .75f}, 0, 7.1f, 0, 16.6f, 0.3f, 25);
    for (int i = 0; i < 12; i++) { boxc(dark, -7.8f, 1.7f, -11 + i * 2.0f, 0.12f, 3.4f, 0.45f); boxc(dark, 7.8f, 1.7f, -11 + i * 2.0f, 0.12f, 3.4f, 0.45f); }
    boxc(dark, 0, 0.1f, -11.96f, 16, 0.2f, 0.1f);
    for (int i = 0; i < 4; i++) { lightPanel(-4.5f + (i % 2) * 9.0f, 6.9f, -7.0f + (i / 2) * 11.0f, 1.8f, 1.0f, {1, .96f, .85f}); }
    for (int i = 0; i < 4; i++) wallWindow(7.8f, 5.3f, -8 + i * 5.0f, 2.2f, 1.6f, false, d.win);
    coatOfArms(0, 4.9f, -11.9f, 1.15f);
    flag3(-6.4f, 5.6f, -11.6f, 2.2f, t, 0); flag3(4.2f, 5.6f, -11.6f, 2.2f, t, 1);
    // the bench: platform, long desk, seven high-backed chairs, seven judges
    boxc(dark, 0, 0.25f, -9.3f, 11.5f, 0.5f, 3.6f); boxc(wood, 0, 1.0f, -8.2f, 10.6f, 1.0f, 0.8f); boxc({.8f, .62f, .2f}, 0, 1.2f, -7.78f, 10.4f, 0.06f, 0.03f);
    boxc(dark, 0, 1.52f, -8.2f, 10.8f, 0.08f, 0.95f);
    for (int k = 0; k < 7; k++) {
        float x = -4.5f + k * 1.5f;
        chair(x, 0.5f, -9.6f, 0, {.38f, .06f, .09f}, 1.35f);
        boxc({.95f, .95f, .95f}, x, 1.35f, -7.77f, 0.4f, 0.1f, 0.02f);
        Look L = (k == 3) ? chiefJusticeLook() : makeLook(100 + k, 1);
        person(x, 0.5f, -9.35f, 0, L, 0, 0, (k == 3 && std::fmod(t, 9.0f) > 3.5f) ? 8 : 3, true, t, 100 + k);
    }
    // clerks, advocates' lecterns and tables
    boxc(wood, 0, 0.55f, -5.2f, 4.4f, 1.1f, 0.8f); for (int k = 0; k < 2; k++) person(-1.0f + k * 2.0f, 0, -5.9f, PI, makeLook(130 + k, 6), 0, 0, 3, true, t, 130 + k);
    for (int s = -1; s <= 1; s += 2) {
        boxc(wood, s * 3.6f, 0.6f, -1.5f, 3.4f, 0.12f, 1.1f); boxc(dark, s * 3.6f, 0.3f, -1.5f, 3.2f, 0.6f, 0.9f);
        for (int k = 0; k < 3; k++) person(s * (2.6f + k * 1.0f), 0, -0.4f, PI, makeLook(140 + (s > 0 ? 3 : 0) + k, 6), 0, 0, 3, true, t, 140 + k);
    }
    boxc(dark, -1.0f, 0.55f, -3.1f, 0.9f, 1.1f, 0.6f); boxc(dark, -1.0f, 1.12f, -3.0f, 0.7f, 0.04f, 0.5f);
    person(-1.0f, 0, -3.8f, 0, makeLook(160, 6), 0, 0, 8, false, t, 160);          // the advocate addressing the bench
    // public gallery behind the bar
    boxc(wood, 0, 0.5f, 3.0f, 15.6f, 1.0f, 0.12f);
    int n = std::min(p.crowd, 52), idx = 0;
    for (int row = 0; row < 5 && idx < n; row++) {
        float z = 4.2f + row * 1.45f;
        boxc(dark, 0, 0.45f, z, 15, 0.1f, 0.55f); boxc(dark, 0, 0.9f, z + 0.35f, 15, 0.9f, 0.08f);
        for (int k = 0; k < 11 && idx < n; k++, idx++) {
            float x = -6.4f + k * 1.28f + (hashf(idx) - 0.5f) * 0.2f;
            bool react = u > 0.6f && hashf(idx * 3 + 1) < 0.25f;
            person(x, 0, z, PI, makeLook(200 + idx, hashf(idx * 9) < 0.3f ? 2 : 0), 0, 0, react ? 3 : (hashf(idx * 5) < 0.12f ? 3 : 0), true, t + idx, 200 + idx);
        }
    }
    for (int s = -1; s <= 1; s += 2) person(s * 6.6f, 0, 1.2f, s > 0 ? -PI / 2 : PI / 2, makeLook(300 + s, 3), 0, 0, 0, false, t, 300);     // court guards at the doors
    // morning light through the windows and floating dust
    if (d.sunK > 0.05f) for (int i = 0; i < 4; i++) { float z = -8 + i * 5.0f; shaft({7.7f, 6.1f, z - 0.9f}, {7.7f, 4.5f, z + 0.9f}, {2.5f, 0.0f, z - 3.2f}, {1.5f, 0.0f, z - 1.4f}, d.win, 0.16f * d.sunK); }
    dust({2, 3, -3}, {10, 5, 14}, t, 70, 3);
    end2D(W, H);
}

// ================================================================== CHAMBER: inside Parliament (green benches, Speaker, Mace, galleries)
void sceneChamber(float t, float u, const Params& p, int W, int H) {
    Tod d = tod(p.sky);
    begin(W, H, {.05f, .05f, .05f}, scalec({.32f, .31f, .28f}, d.ik), false, {0, 0, 0}, 0, 0);
    light(0, {0, 8.2f, 0}, scalec({.95f, .9f, .78f}, mixf(1.0f, d.ik, 0.5f))); light(1, {0, 3.5f, 14}, scalec(mixc({.3f, .32f, .38f}, d.sun, 0.5f), 0.75f));
    V3 eye, tgt;
    if (u < 0.4f)       { float v = smooth(u / 0.4f); eye = {mixf(-5.0f, 3.0f, v), mixf(5.6f, 4.4f, v), 12.0f}; tgt = {0, 1.2f, -6}; }              // from the public gallery, drifting across
    else if (u < 0.72f) { float v = (u - 0.4f) / 0.32f; eye = {mixf(-1.0f, -3.4f, v), 2.5f, mixf(3.0f, 0.0f, v)}; tgt = {mixf(4.5f, 5.5f, v), 1.7f, mixf(-3.0f, -6.0f, v)}; }  // along the benches
    else                { float v = (u - 0.72f) / 0.28f; eye = {mixf(2.0f, 0.4f, v), mixf(2.6f, 2.3f, v), mixf(1.0f, -3.0f, v)}; tgt = {0, 2.3f, -12.5f}; }      // towards the Speaker's chair
    camera(eye + shake(t, 0.02f + 0.03f * p.energy), tgt, 56, W, H);
    floorQuad(T_CARPET, -13, -14, 13, 15, 0, 0.35f);
    C wood = {.28f, .17f, .1f}, dark = {.17f, .1f, .06f}, green = {.07f, .33f, .17f};
    boxc(wood, -13, 5, 0, 0.4f, 10, 30); boxc(wood, 13, 5, 0, 0.4f, 10, 30); boxc(wood, 0, 5, -14.5f, 26.4f, 10, 0.4f); boxc(wood, 0, 5, 15.5f, 26.4f, 10, 0.4f);
    boxc({.8f, .78f, .7f}, 0, 10.1f, 0, 26.4f, 0.3f, 30);
    for (int i = 0; i < 12; i++) { boxc(dark, -12.7f, 2.2f, -13 + i * 2.5f, 0.15f, 4.4f, 0.5f); boxc(dark, 12.7f, 2.2f, -13 + i * 2.5f, 0.15f, 4.4f, 0.5f); }
    for (int i = 0; i < 6; i++) lightPanel(-8 + (i % 3) * 8.0f, 9.9f, -6 + (i / 3) * 12.0f, 3.0f, 2.0f, {1, .97f, .86f});
    // public galleries: balcony along the back and both sides, spectators leaning on the rail
    boxc(dark, 0, 4.6f, 14.0f, 26, 0.3f, 3.0f); boxc(wood, 0, 5.2f, 12.5f, 26, 1.0f, 0.15f);
    boxc(dark, -11.5f, 4.6f, 0, 3.0f, 0.3f, 28); boxc(wood, -10.0f, 5.2f, 0, 0.15f, 1.0f, 28);
    boxc(dark, 11.5f, 4.6f, 0, 3.0f, 0.3f, 28); boxc(wood, 10.0f, 5.2f, 0, 0.15f, 1.0f, 28);
    for (int i = 0; i < 22; i++) {
        float x = -11 + i, z = 13.2f; bool left = hashf(i * 3) < 0.5f;
        person(x, 4.75f, z + hashf(i) * 0.8f, PI, makeLook(400 + i, left ? 0 : 2), 0, 0, hashf(i * 7) < 0.2f ? 6 : 0, false, t + i, 400 + i);
    }
    for (int i = 0; i < 9; i++) { person(-11.5f, 4.75f, -11 + i * 2.6f, PI / 2, makeLook(430 + i, 0), 0, 0, 0, false, t + i, 430 + i); person(11.5f, 4.75f, -10 + i * 2.6f, -PI / 2, makeLook(450 + i, 0), 0, 0, 0, false, t + i, 450 + i); }
    // Speaker's chair under a canopy at the far end, clerks' table with the Mace between the two benches
    boxc(dark, 0, 0.6f, -12.3f, 6.5f, 1.2f, 2.6f); boxc(wood, 0, 1.3f, -12.9f, 3.4f, 0.2f, 1.2f);
    chair(0, 1.2f, -12.8f, 0, {.3f, .08f, .08f}, 1.7f);
    for (int s = -1; s <= 1; s += 2) boxc(dark, s * 2.2f, 4.2f, -13.8f, 0.4f, 6.0f, 0.4f);
    boxc(dark, 0, 7.3f, -13.8f, 5.2f, 0.7f, 0.5f); coatOfArms(0, 5.3f, -14.2f, 1.1f);
    { Look S = speakerLook(); person(0, 1.2f, -12.5f, 0, S, 0, 0, (std::fmod(t, 8.0f) > 4.0f) ? 8 : 3, true, t, 500);
      glPushMatrix(); glTranslatef(0, 2.3f, -12.5f); col({.9f, .9f, .92f}); ell(0, 0.1f, -0.01f, 0.15f, 0.16f, 0.15f, 8); glPopMatrix(); }       // the wig
    boxc(wood, 0, 0.8f, -8.7f, 3.6f, 1.0f, 1.3f); boxc(dark, 0, 1.33f, -8.7f, 3.8f, 0.08f, 1.5f);
    col({.85f, .68f, .22f}); box(0, 1.5f, -8.7f, 0.08f, 0.08f, 1.0f); ell(0, 1.52f, -8.2f, 0.16f, 0.16f, 0.16f, 8); ell(0, 1.52f, -9.2f, 0.1f, 0.1f, 0.1f, 6);     // the Mace
    for (int k = 0; k < 3; k++) person(-1.0f + k, 0, -9.7f, 0, makeLook(520 + k, 2), 0, 0, 3, true, t, 520 + k);
    // green benches facing each other, MPs seated (and rising to vote)
    int seats = 0, want = 1000;                          // full house: every seat is taken
    for (int row = 0; row < 5; row++) {
        for (int side = -1; side <= 1; side += 2) {
            float x = side * (3.6f + row * 1.55f), y = row * 0.42f;
            boxc(green, x, y + 0.45f, 0, 0.8f, 0.12f, 20); boxc(dark, x + side * 0.45f, y + 0.9f, 0, 0.1f, 0.9f, 20); boxc(dark, x, y + 0.22f, 0, 0.8f, 0.44f, 20);
        }
    }
    // Fill matching positions on both sides before moving deeper into the chamber.
    for (int row = 0; row < 5; row++)
        for (int k = 0; k < 18; k++)
            for (int side = -1; side <= 1; side += 2) {
            if (seats >= want) continue;
            seats++;
            float x = side * (3.6f + row * 1.55f), y = row * 0.42f;
            float z = -9.35f + k * 1.1f + (hashf(seats) - 0.5f) * 0.1f;
            float standAt = 0.15f + 0.8f * hashf(seats * 13 + 2);
            bool stand = p.vote && u > standAt && u < standAt + 0.35f;
            int arm = stand ? 7 : (hashf(seats * 17) < 0.1f ? 6 : (hashf(seats * 19) < 0.2f ? 3 : 0));
            if (stand) person(x + side * 0.1f, y + 0.0f, z, side > 0 ? -PI / 2 : PI / 2, makeLook(600 + seats, 2), 0, 0, 7, false, t, 600 + seats);
            else person(x, y + 0.0f, z, side > 0 ? -PI / 2 : PI / 2, makeLook(600 + seats, 2), 0, 0, arm, true, t + seats, 600 + seats);
        }
    flag3(-9.5f, 6.4f, -14.1f, 2.4f, t, 2); flag3(7.2f, 6.4f, -14.1f, 2.4f, t, 3);
    for (int i = 0; i < 5; i++) shaft({-12.0f + i * 6.0f, 9.9f, -4}, {-11.0f + i * 6.0f, 9.9f, -4}, {-9.0f + i * 6.0f, 0.2f, 1}, {-7.5f + i * 6.0f, 0.2f, 1}, {1, .93f, .7f}, 0.06f);
    dust({0, 4, 0}, {20, 7, 22}, t, 90, 5);
    end2D(W, H);
}

float yawToward(float x, float z, float tx, float tz) { return std::atan2(tx - x, tz - z); }
void spectator(float x, float y, float z, float yaw, C top, C skin, float bob) {
    glPushMatrix(); glTranslatef(x, y + bob, z); glRotatef(yaw * 180.0f / PI, 0, 1, 0);
    col(top); box(0, 0.35f, 0, 0.45f, 0.55f, 0.28f); col(skin); ell(0, 0.78f, 0, 0.13f, 0.15f, 0.13f, 6); col({.05f, .04f, .04f}); ell(0, 0.86f, -0.01f, 0.135f, 0.08f, 0.135f, 6);
    glPopMatrix();
}
void tyrePile(float x, float z, float t, float seed) {
    col({.04f, .04f, .04f}); for (int k = 0; k < 6; k++) { float a = k * 1.1f + seed; cyl(x + std::cos(a) * 0.45f * (k > 2 ? 0.4f : 1.0f), (k > 2 ? 0.28f : 0.0f), z + std::sin(a) * 0.45f * (k > 2 ? 0.4f : 1.0f), 0.42f, 0.26f, 10); }
    col({.35f, .25f, .15f}); box(x + 0.7f, 0.12f, z - 0.2f, 1.4f, 0.12f, 0.1f); box(x - 0.6f, 0.2f, z + 0.5f, 1.2f, 0.1f, 0.12f);
    glDisable(GL_LIGHTING); for (int k = 0; k < 4; k++) { float f = 0.7f + 0.3f * std::sin(t * 11 + k * 2.0f + seed); col({1.0f, 0.5f + 0.25f * f, 0.1f}); ell(x + (k - 1.5f) * 0.22f, 0.55f + 0.25f * f, z + std::sin(k + seed) * 0.2f, 0.16f, 0.4f * f, 0.16f, 6); } glEnable(GL_LIGHTING);
    for (int k = 0; k < 7; k++) { float age = std::fmod(t * 0.25f + k * 0.15f + seed, 1.0f); puff({x + age * 1.2f, 1.0f + age * 6.0f, z}, 0.7f + age * 2.2f, {.18f, .17f, .17f}, 0.4f * (1 - age)); }
}
void streetBuildings(float zEnd, float zStart, bool lit, float k, int seed) {
    for (int side = -1; side <= 1; side += 2) {
        float z = zStart;
        for (int i = 0; z > zEnd; i++) {
            float w = 7 + hashf(seed * 31 + i * 3 + side) * 6, h = 8 + hashf(seed * 17 + i * 5 + side) * 18, x = side * (8.0f + w / 2);
            C c = mixc({.52f, .5f, .46f}, {.4f, .42f, .46f}, hashf(i * 7 + side + seed)); c = scalec(c, k);
            boxc(c, x, h / 2, z - w / 2, w, h, w);
            for (int r = 0; r < (int)(h / 2.6f); r++) for (int kk = 0; kk < 3; kk++) {
                bool on = lit && hashf(i * 101 + r * 13 + kk * 7 + side) < 0.4f;
                glDisable(GL_LIGHTING); col(on ? C{1, .85f, .5f} : mixc(C{.08f, .1f, .16f}, C{.35f, .45f, .55f}, clamp01((k - 0.35f) / 0.65f)));
                box(side * (8.0f - 0.02f), 2.2f + r * 2.6f, z - (kk + 0.8f) * w / 3.4f, 0.04f, 1.3f, 0.9f); glEnable(GL_LIGHTING);
            }
            z -= w + 0.4f;
        }
    }
}
void lampPost(float x, float z, bool night) {
    col({.3f, .3f, .32f}); box(x, 3.2f, z, 0.1f, 6.4f, 0.1f); box(x - (x > 0 ? 0.6f : -0.6f), 6.3f, z, 1.2f, 0.08f, 0.1f);
    if (night) { glDisable(GL_LIGHTING); col({1, .9f, .6f}); box(x - (x > 0 ? 1.1f : -1.1f), 6.2f, z, 0.35f, 0.08f, 0.2f); glEnable(GL_LIGHTING); }
}
void matatu(float x, float z, float yaw, C body) {
    glPushMatrix(); glTranslatef(x, 0, z); glRotatef(yaw * 180.0f / PI, 0, 1, 0);
    col(body); box(0, 1.2f, 0, 1.9f, 1.6f, 5.2f); col({.15f, .2f, .26f}); box(0, 1.5f, 0.1f, 1.95f, 0.7f, 4.4f); col({.95f, .95f, .95f}); box(0, 0.8f, 0, 1.95f, 0.18f, 5.0f);
    col({.05f, .05f, .05f}); for (int i = 0; i < 4; i++) cyl((i % 2 ? 1 : -1) * 0.95f, 0, (i < 2 ? 1 : -1) * 1.7f, 0.38f, 0.25f, 10);
    glPopMatrix();
}

// ================================================================== POLLING: a polling station (streams, clerks, booths, ballot boxes, ink)
void scenePolling(float t, float u, const Params& p, int W, int H) {
    Tod d = tod(p.sky);
    begin(W, H, d.sky, mixc({.62f, .6f, .56f}, d.amb, 0.55f), true, d.sky, 45, 120);
    light(0, d.sunPos, d.sun); light(1, {0, 3, -4}, scalec({.38f, .36f, .33f}, mixf(1.0f, d.ik, 0.5f)));
    if (d.lamps > 0.3f) light(2, {0, 3.2f, 0}, scalec({.6f, .55f, .45f}, d.lamps), 0.03f);       // ceiling lamps on at dusk / night / before dawn
    V3 eye, tgt;
    if (u < 0.3f)       { float v = smooth(u / 0.3f); eye = {mixf(-6, -2, v), mixf(3.4f, 1.9f, v), mixf(20, 13.5f, v)}; tgt = {0, 1.4f, 6}; }
    else if (u < 0.58f) { float v = (u - 0.3f) / 0.28f; eye = {mixf(-0.5f, 1.0f, v), 2.1f, -6.8f}; tgt = {0, 1.3f, 9}; }
    else if (u < 0.8f)  { float v = (u - 0.58f) / 0.22f; eye = {mixf(-3.0f, -1.2f, v), 1.75f, mixf(-2.2f, -3.2f, v)}; tgt = {-1.5f, 1.45f, -6.8f}; }
    else                { float v = (u - 0.8f) / 0.2f; eye = {mixf(1.6f, 0.8f, v), 1.5f, -0.2f}; tgt = {0.3f, 1.2f, mixf(-3.5f, -3.0f, v)}; }
    camera(eye + shake(t, 0.012f), tgt, 58, W, H);
    floorQuad(T_GRASS, -30, 9, 30, 60, -0.02f, 0.2f, {.9f, .85f, .6f}); floorQuad(T_TILE, -6, -8, 6, 9, 0, 0.5f);
    floorQuad(T_ASPHALT, -6.2f, 9, 6.2f, 30, 0.01f, 0.25f, {1.3f, 1.2f, 1.0f});
    C wall = {.8f, .74f, .6f}, roof = {.32f, .34f, .38f};
    boxc(wall, -6.1f, 1.8f, 0.5f, 0.25f, 3.6f, 17); boxc(wall, 6.1f, 1.8f, 0.5f, 0.25f, 3.6f, 17); boxc(wall, 0, 1.8f, -8.1f, 12.4f, 3.6f, 0.25f);
    float segs[5][2] = {{-6, -5.6f}, {-3.4f, -2.6f}, {-0.4f, 0.4f}, {2.6f, 3.4f}, {5.6f, 6}};
    for (auto& s : segs) boxc(wall, (s[0] + s[1]) / 2, 1.8f, 9, s[1] - s[0], 3.6f, 0.25f);
    for (int i = 0; i < 4; i++) boxc(wall, -4.5f + i * 3.0f, 3.2f, 9, 2.2f, 0.8f, 0.25f);
    boxc(roof, 0, 3.75f, 0.5f, 13.4f, 0.2f, 18.5f); for (int i = 0; i < 8; i++) boxc({.25f, .22f, .2f}, 0, 3.55f, -7.5f + i * 2.3f, 12.4f, 0.14f, 0.18f);
    for (int i = 0; i < 4; i++) { glDisable(GL_LIGHTING); col(d.win); box(6.0f, 2.5f, -5 + i * 3.8f, 0.05f, 1.0f, 1.8f); box(-6.0f, 2.5f, -5 + i * 3.8f, 0.05f, 1.0f, 1.8f); glEnable(GL_LIGHTING); }
    // sunlight pouring through the four doorways
    if (d.sunK > 0.05f) for (int i = 0; i < 4; i++) { float x = -4.5f + i * 3.0f; shaft({x - 0.9f, 3.2f, 9.0f}, {x + 0.9f, 3.2f, 9.0f}, {x - 1.4f + 1.0f, 0.0f, 3.5f}, {x + 1.4f + 1.0f, 0.0f, 3.5f}, d.win, 0.22f * d.sunK); }
    if (d.lamps > 0.3f) for (int i = 0; i < 4; i++) lightPanel(-4.5f + i * 3.0f, 3.45f, 0, 1.6f, 0.8f, scalec({1, .94f, .78f}, d.lamps));
    dust({0, 2, 3}, {11, 3, 11}, t, 70, 8);
    // posters on the back wall
    for (int i = 0; i < 6; i++) { boxc({.95f, .95f, .92f}, -5 + i * 2.0f, 2.6f, -7.95f, 1.0f, 0.7f, 0.02f); boxc(i % 2 ? C{.75f, .1f, .1f} : C{.1f, .45f, .22f}, -5 + i * 2.0f, 2.85f, -7.93f, 0.9f, 0.14f, 0.02f); }
    // streams: a table, a clerk with a tablet, a register and ink at each doorway, a queue that creeps forward
    float C0 = 4.5f, ph = std::fmod(t, C0); int step = (int)(t / C0); float mv = smooth((ph - (C0 - 1.5f)) / 1.5f);
    int perLine = std::max(3, std::min(9, p.crowd / 6));
    for (int line = 0; line < 4; line++) {
        float x = -4.5f + line * 3.0f;
        boxc({.55f, .4f, .25f}, x, 0.72f, 3.2f, 1.9f, 0.06f, 0.8f); boxc({.45f, .32f, .2f}, x, 0.35f, 3.2f, 1.8f, 0.7f, 0.7f);
        boxc({.2f, .2f, .24f}, x - 0.4f, 0.86f, 3.0f, 0.4f, 0.22f, 0.03f); boxc({.95f, .95f, .95f}, x + 0.3f, 0.77f, 3.3f, 0.5f, 0.01f, 0.35f);
        col({.4f, .15f, .6f}); cyl(x + 0.7f, 0.75f, 3.1f, 0.05f, 0.1f, 8);
        chair(x, 0, 2.4f, PI, {.2f, .2f, .6f}, 0.7f); person(x, 0, 2.55f, 0, makeLook(700 + line, line % 2 ? 7 : 0), 0, 0, 3, true, t, 700 + line);
        for (int k = 0; k < perLine; k++) {
            int id = step + k; float slot = k - mv; float z = 4.2f + slot * 1.0f;
            if (z < 3.5f) continue;
            bool moving = mv > 0.001f && mv < 0.999f;
            int arm = (k == 0 && !moving) ? 3 : ((hashf(id * 7 + line) < 0.2f && !moving) ? 6 : 0);
            person(x + (hashf(id + line * 50) - 0.5f) * 0.25f, 0, z, PI, makeLook(800 + id * 4 + line, hashf(id * 3 + line) < 0.3f ? 2 : 0), t * 6.0f + id, moving ? 1.0f : 0.0f, arm, false, t, 800 + id * 4 + line);
        }
    }
    // booths with voters marking their papers
    for (int k = 0; k < std::max(2, std::min(p.stations, 6)); k++) {
        float x = -4.5f + k * (9.0f / std::max(1, std::min(p.stations, 6) - 1 ? std::min(p.stations, 6) - 1 : 1));
        C card = {.86f, .8f, .66f};
        boxc(card, x, 0.75f, -6.6f, 1.3f, 1.5f, 0.05f); boxc(card, x - 0.65f, 0.75f, -6.1f, 0.05f, 1.5f, 1.0f); boxc(card, x + 0.65f, 0.75f, -6.1f, 0.05f, 1.5f, 1.0f);
        boxc({.8f, .74f, .6f}, x, 0.95f, -6.1f, 1.2f, 0.05f, 0.5f);
        if (std::fmod(t + k * 2.3f, 7.0f) < 5.2f) { person(x, 0, -6.3f, PI, makeLook(900 + k, 0), 0, 0, 3, false, t, 900 + k); boxc({.97f, .97f, .95f}, x, 0.99f, -6.1f, 0.3f, 0.01f, 0.4f); }
    }
    // six ballot boxes in a row, clear sides, papers dropping through the slots
    for (int k = 0; k < 6; k++) {
        float x = -5.0f + k * 2.0f; C lid[6] = {{.1f,.2f,.7f},{.8f,.15f,.15f},{.1f,.5f,.2f},{.9f,.75f,.1f},{.5f,.2f,.6f},{.95f,.95f,.95f}};
        boxc({.4f, .42f, .46f}, x, 0.3f, -3.0f, 0.6f, 0.6f, 0.6f);
        boxc(lid[k], x, 0.95f, -3.0f, 0.66f, 0.06f, 0.66f); boxc({.05f, .05f, .05f}, x, 0.99f, -3.0f, 0.3f, 0.01f, 0.03f);
        glDepthMask(GL_FALSE); glDisable(GL_LIGHTING); glEnable(GL_BLEND); col({.75f, .88f, .98f}, 0.3f); box(x, 0.65f, -3.0f, 0.6f, 0.62f, 0.6f);
        glEnable(GL_LIGHTING); glDepthMask(GL_TRUE);
        for (int q = 0; q < 6; q++) boxc({.95f, .95f, .92f}, x + (hashf(k * 9 + q) - 0.5f) * 0.4f, 0.4f + q * 0.04f, -3.0f + (hashf(k * 5 + q) - 0.5f) * 0.4f, 0.22f, 0.012f, 0.3f);
        float f = std::fmod(t * 0.3f + k * 0.17f, 1.0f); if (f < 0.35f) boxc({.97f, .97f, .94f}, x, 1.25f - f * 1.0f, -3.0f, 0.2f, 0.012f, 0.28f);
        person(x, 0, -2.2f, PI, makeLook(950 + k, 0), 0, 0, f < 0.35f ? 3 : 0, false, t, 950 + k);
    }
    // party agents and observers along the right wall
    for (int i = 0; i < 5; i++) { chair(5.3f, 0, -2 + i * 2.2f, -PI / 2, {.3f, .3f, .35f}, 0.8f); person(5.3f, 0, -2 + i * 2.2f, -PI / 2, makeLook(980 + i, i % 2 ? 7 : 0), 0, 0, i == 2 ? 3 : 0, true, t, 980 + i); }
    // a voter holds up an inked finger as she leaves
    float inkT = std::fmod(t, 9.0f); person(0.6f, 0, -0.2f, 0.4f, makeLook(990, 4), 0, 0, (inkT > 3.0f && inkT < 7.0f) ? 12 : 0, false, t, 990);
    end2D(W, H);
}

// ================================================================== INAUGURATION / FUNERAL: the stadium
void sceneStadium(float t, float u, const Params& p, int W, int H) {
    bool fun = p.mourning;
    Tod d = tod(p.sky);
    C sky = fun ? mixc(d.sky, C{.55f, .57f, .62f}, 0.55f) : d.sky;                // funeral: overcast version of the same time of day
    begin(W, H, sky, d.amb, true, sky, 70, 190);
    light(0, d.sunPos, fun ? scalec(d.sun, 0.85f) : d.sun); light(1, {-10, 10, -10}, scalec({.35f, .38f, .45f}, mixf(1.0f, d.ik, 0.5f)));
    if (d.lamps > 0.3f) light(2, {0, 28, 0}, scalec({.9f, .85f, .7f}, d.lamps), 0.0006f);      // floodlights
    V3 eye, tgt; float dz = -6.0f;
    if (u < 0.35f)      { float v = smooth(u / 0.35f); eye = {mixf(-30, -12, v), mixf(11, 5.5f, v), mixf(24, 16, v)}; tgt = {0, 1.8f, dz}; }
    else if (u < 0.7f)  { float v = (u - 0.35f) / 0.35f; eye = {mixf(1.5f, 0.6f, v), 1.9f, mixf(9, 3.0f, v)}; tgt = {0, 1.8f, dz}; }
    else                { float v = (u - 0.7f) / 0.3f; eye = {mixf(2.4f, 1.6f, v), 1.85f, -1.0f}; tgt = {-0.1f, 1.85f, -5.4f}; }
    camera(eye + shake(t, 0.015f), tgt, 55, W, H);
    floorQuad(T_GRASS, -26, -20, 26, 20, 0, 0.3);
    // running track and the bowl of stands
    const int N = 44; float a = 34, b = 25;
    for (int tier = 0; tier < 6; tier++) {
        float s = 1.0f + tier * 0.055f, y = 0.4f + tier * 1.05f;
        for (int i = 0; i < N; i++) {
            float th = i * 2 * PI / N, th2 = (i + 1) * 2 * PI / N;
            float x0 = a * s * std::cos(th), z0 = b * s * std::sin(th), x1 = a * s * std::cos(th2), z1 = b * s * std::sin(th2);
            float mx = (x0 + x1) / 2, mz = (z0 + z1) / 2, tx = x1 - x0, tz = z1 - z0, L = std::sqrt(tx * tx + tz * tz);
            glPushMatrix(); glTranslatef(mx, y, mz); glRotatef(std::atan2(-tz, tx) * 180.0f / PI, 0, 1, 0);
            col(tier % 2 ? C{.55f, .55f, .58f} : C{.45f, .45f, .5f}); box(0, 0, 0, L + 0.05f, 1.0f, 1.1f); glPopMatrix();
            int per = 4; float dens = std::min(1.0f, p.crowd / 90.0f);
            for (int q = 0; q < per; q++) {
                int id = (tier * N + i) * 4 + q; if (hashf(id * 3 + 1) > 0.25f + 0.7f * dens) continue;
                float f = (q + 0.5f) / per, sx = x0 + (x1 - x0) * f, sz = z0 + (z1 - z0) * f, sy = y + 0.5f;
                float cheer = (u > 0.55f && !fun) ? std::fabs(std::sin(t * 6 + id)) * 0.18f : 0.0f;
                C c = fun ? (hashf(id) < 0.7f ? C{.09f, .09f, .1f} : C{.8f, .8f, .82f}) : mixc({.8f, .2f, .15f}, {.15f, .45f, .25f}, hashf(id * 5)); if (!fun && hashf(id * 7) < 0.3f) c = {.9f, .9f, .9f};
                spectator(sx * 0.985f, sy, sz * 0.985f, yawToward(sx, sz, 0, 0), c, skinTone(id), cheer);
            }
        }
    }
    softBegin(); glBegin(GL_TRIANGLE_STRIP); for (int i = 0; i <= 60; i++) { float th = i * 2 * PI / 60; col({.6f, .22f, .15f}, 1.0f); glVertex3f(26 * std::cos(th), 0.02f, 19 * std::sin(th)); glVertex3f(30 * std::cos(th), 0.02f, 22 * std::sin(th)); } glEnd(); softEnd();
    // the dais
    boxc({.5f, .5f, .55f}, 0, 0.45f, dz - 1.2f, 12, 0.9f, 5.0f); boxc({.65f, .1f, .12f}, 0, 0.92f, dz - 0.9f, 8.5f, 0.03f, 4.4f);
    boxc({.2f, .2f, .26f}, 0, 4.2f, dz - 4.0f, 14, 6.5f, 0.3f); coatOfArms(0, 4.6f, dz - 3.8f, 1.4f);
    for (int s = -1; s <= 1; s += 2) { flag3(s * 5.4f - (s > 0 ? 0 : 0), fun ? 4.0f : 6.0f, dz - 2.5f, 2.4f, t, s); boxc({.4f, .4f, .42f}, s * 5.4f, 3.5f, dz - 2.5f, 0.06f, 7, 0.06f); }
    for (int k = 0; k < 8; k++) { float x = -5.6f + k * 1.6f; chair(x, 0.9f, dz - 3.0f, 0, {.35f, .08f, .1f}, 0.8f); person(x, 0.9f, dz - 2.9f, 0, makeLook(1000 + k, k % 3 == 0 ? 1 : (k % 3 == 1 ? 2 : 4), fun), 0, 0, 0, true, t, 1000 + k); }
    if (!fun) {
        boxc({.35f, .22f, .12f}, 0, 1.4f, dz + 0.5f, 1.3f, 0.9f, 0.5f); boxc({.1f, .1f, .1f}, 0, 1.95f, dz + 0.55f, 0.03f, 0.3f, 0.03f);
        person(-0.55f, 0.9f, dz - 0.6f, yawToward(-0.55f, dz - 0.6f, 0.6f, dz + 1.5f), makeLook(1100, 1), 0, 0, 8 - (u > 0.7f ? 5 : 0), false, t, 1100);                      // the Chief Justice
        person(0.65f, 0.9f, dz - 0.5f, 0.0f, presidentLook(1101), 0, 0, u > 0.35f ? 4 : 0, false, t, 1101);                                               // the president-elect, right hand raised
        person(1.7f, 0.9f, dz - 0.5f, 0.0f, makeLook(1102, 4), 0, 0, 5 * (u > 0.8f ? 1 : 0), false, t, 1102);
    } else {
        boxc({.3f, .3f, .32f}, 0, 1.1f, dz + 1.0f, 2.8f, 0.4f, 1.2f); boxc({.12f, .12f, .12f}, 0, 1.5f, dz + 1.0f, 2.4f, 0.45f, 0.9f);
        static const C fb[5] = {{.05f,.05f,.05f},{.95f,.95f,.95f},{.75f,.1f,.1f},{.95f,.95f,.95f},{.05f,.45f,.2f}};
        for (int k = 0; k < 5; k++) boxc(fb[k], 0, 1.76f, dz + 1.0f - 0.45f + k * 0.225f, 2.5f, 0.03f, 0.2f);
        for (int s = -1; s <= 1; s += 2) for (int k = 0; k < 3; k++) { col({.85f, .85f, .85f}); ell(s * (2.0f + k * 0.5f), 1.2f, dz + 0.4f, 0.3f, 0.3f, 0.3f, 8); col({.1f, .4f, .2f}); ell(s * (2.0f + k * 0.5f), 1.0f, dz + 0.4f, 0.34f, 0.2f, 0.34f, 8); }
        for (int k = 0; k < 8; k++) person((k < 4 ? -1 : 1) * (1.6f + (k % 4) * 0.55f), 0, dz + 2.8f, PI, makeLook(1200 + k, 5), 0, 0, 11, false, t, 1200 + k);
    }
    // honour guard marching across the field, band behind them
    for (int row = 0; row < 3; row++) for (int k = 0; k < 8; k++) {
        float x = -18 + std::fmod(t * 1.5f + k * 1.2f, 36.0f); if (fun && u < 0.4f) x = -18 + std::fmod(t * 1.0f + k * 1.2f, 36.0f);
        person(x, 0, 6.5f + row * 1.2f, PI / 2, makeLook(1300 + row * 10 + k, 5), t * 5.0f + k, 1.0f, 0, false, t, 1300 + k);
    }
    for (int s = -1; s <= 1; s += 2) for (int k = 0; k < 6; k++) person(s * 7.2f, 0, dz + 3.5f - k * 0.0f + k * 1.8f - 4, s > 0 ? -PI / 2 : PI / 2, makeLook(1400 + k + (s > 0 ? 8 : 0), 5), 0, 0, 11, false, t, 1400 + k);
    if (d.lamps > 0.3f) for (int m = 0; m < 4; m++) {                              // floodlight masts when it is dusk or dark
        float mx = (m % 2 ? 1 : -1) * 47.0f, mz = (m < 2 ? 1 : -1) * 30.0f;
        boxc({.35f, .35f, .38f}, mx, 9, mz, 0.3f, 18, 0.3f); glDisable(GL_LIGHTING); col(scalec({1, .95f, .8f}, 0.6f + 0.4f * d.lamps)); box(mx, 18.3f, mz, 2.0f, 0.6f, 0.6f); glEnable(GL_LIGHTING);
    }
    if (!fun && u > 0.5f) for (int k = 0; k < 4; k++) { float age = std::fmod(t * 0.5f + k * 0.7f, 2.8f) / 2.8f; puff({-14.0f + k * 1.5f + age * 2, 1.2f + age * 3.5f, 9.0f}, 0.8f + age * 2.6f, {.88f, .88f, .86f}, 0.5f * (1 - age)); }
    end2D(W, H);
}

// ================================================================== PROTEST: the street (and the road to Parliament)
void sceneProtest(float t, float u, const Params& p, int W, int H, bool parliament) {
    Tod d = tod(p.night ? 4 : p.sky);
    bool night = d.night, lampsOn = d.lamps > 0.5f;
    C sky = p.smoke ? mixc(d.sky, C{.6f, .62f, .64f}, night ? 0.25f : 0.6f) : d.sky;
    begin(W, H, sky, night ? C{.3f, .32f, .45f} : d.amb, true, sky, night ? 25 : 30, night ? 80 : 95);
    light(0, d.sunPos, d.sun); light(1, {0, 4, 10}, night ? C{.2f, .16f, .12f} : scalec(mixc({.3f, .32f, .38f}, d.sun, 0.3f), 1.0f));
    if (lampsOn) light(2, {0, 1.6f, -3}, scalec({1.3f, .7f, .3f}, d.lamps), 0.012f);                // burning tyres / street light glow
    float frontZ = parliament ? -12.5f : -14.0f;
    V3 eye, tgt;
    if (u < 0.4f)       { float v = smooth(u / 0.4f); eye = {mixf(-2.5f, 1.5f, v), mixf(2.6f, 2.2f, v), mixf(19, 15, v)}; tgt = {0, 1.8f, -10}; }            // behind the crowd, looking down the street
    else if (u < 0.75f) { float v = (u - 0.4f) / 0.35f; eye = {6.3f, 1.7f, mixf(8.0f, -4.0f, v)}; tgt = {0, 1.7f, mixf(0.0f, -10.0f, v)}; }                  // tracking along the edge of the march
    else                { float v = (u - 0.75f) / 0.25f; eye = {mixf(-6.0f, -5.4f, v), 1.9f, -4.5f}; tgt = {0, 1.9f, -17}; }                                // at the front, facing the line
    camera(eye + shake(t, 0.03f + 0.05f * p.energy, 1.7f), tgt, 60, W, H);
    floorQuad(T_ASPHALT, -7.5f, -80, 7.5f, 30, 0, 0.3f, scalec(C{1, 1, 1}, 0.55f + 0.45f * clamp01((d.bld - 0.35f) / 0.65f)));
    boxc({.55f, .55f, .52f}, -8.1f, 0.08f, -25, 1.8f, 0.16f, 110); boxc({.55f, .55f, .52f}, 8.1f, 0.08f, -25, 1.8f, 0.16f, 110);
    for (int i = 0; i < 14; i++) boxc({.9f, .9f, .85f}, 0, 0.02f, 24 - i * 7.0f, 0.18f, 0.02f, 3.0f);
    if (parliament) {                                                    // Parliament at the end of the road: block, columns, clock tower, iron fence
        for (int side = -1; side <= 1; side += 2) { float z = 28; for (int i = 0; z > -14; i++) { float w = 7 + hashf(i + side * 9) * 4, h = 6 + hashf(i * 3 + side) * 8; boxc(scalec(C{.52f, .5f, .46f}, d.bld), side * (8.0f + w / 2), h / 2, z - w / 2, w, h, w); z -= w + 0.5f; } }
        boxc(scalec(C{.8f, .78f, .72f}, d.bld), 0, 4, -34, 36, 8, 10); for (int i = 0; i < 9; i++) boxc(scalec(C{.9f, .88f, .82f}, d.bld), -14 + i * 3.5f, 4, -28.8f, 0.7f, 8, 0.7f);
        boxc(scalec(C{.85f, .82f, .76f}, d.bld), 0, 11, -34, 4.5f, 16, 4.5f); col({.95f, .95f, .9f}); ell(0, 14, -31.6f, 1.1f, 1.1f, 0.1f, 14); boxc({.2f, .2f, .25f}, 0, 20, -34, 5.5f, 1.5f, 5.5f);
        for (int i = 0; i < 40; i++) boxc({.1f, .1f, .12f}, -9.5f + i * 0.5f, 1.2f, -16, 0.05f, 2.4f, 0.05f); boxc({.1f, .1f, .12f}, 0, 2.3f, -16, 20, 0.08f, 0.08f); boxc({.1f, .1f, .12f}, 0, 0.3f, -16, 20, 0.08f, 0.08f);
    } else streetBuildings(-60, 28, lampsOn, d.bld, p.seed);
    for (int i = 0; i < 6; i++) { lampPost(-7.2f, 20 - i * 10.0f, lampsOn); lampPost(7.2f, 15 - i * 10.0f, lampsOn); }
    matatu(6.5f, 20.0f, 0.0f, {.2f, .55f, .3f}); matatu(-6.5f, 4.0f, 0.0f, {.85f, .85f, .88f});
    // the crowd walks forward and gathers at the police line / fence
    int n = std::min(p.crowd, 110); bool vigil = p.candles;
    for (int i = 0; i < n; i++) {
        float x = -5.2f + 10.4f * hashf(i * 3 + 1 + p.seed * 17), z0 = (vigil ? -10.0f : -2.0f) + (vigil ? 14.0f : 14.0f) * hashf(i * 5 + 2 + p.seed * 13);
        float v = vigil ? 0.0f : (0.9f + 0.7f * hashf(i * 7 + 3)) * (0.5f + p.energy);
        float stopZ = frontZ + 3.0f * hashf(i * 11 + 4) + 5.0f * hashf(i * 13 + 5) * hashf(i * 3 + 9);
        float z = vigil ? z0 : std::max(stopZ, z0 - t * v); bool moving = !vigil && z > stopZ + 0.02f;
        int arm = vigil ? 9 : (hashf(i * 17 + 6) < 0.35f ? 2 : (hashf(i * 19 + 7) < 0.6f ? 1 : (hashf(i * 23 + 8) < 0.15f ? 6 : 0)));
        if (!moving && !vigil && hashf(i * 3 + 1) < 0.5f) arm = 1;
        Look L = makeLook(1500 + i + p.seed * 200, hashf(i * 29) < 0.28f ? 4 : 0, p.mourning || vigil);
        float jig = moving ? 0.0f : std::fabs(std::sin(t * 5.0f + i)) * 0.06f * p.energy;
        person(x + (moving ? 0 : std::sin(t * 2 + i) * 0.05f), jig, z, PI + (hashf(i) - 0.5f) * 0.3f, L, t * 5.5f + i * 1.7f, moving ? 1.0f : 0.0f, arm, false, t, 1500 + i);
        if (vigil) { glDisable(GL_LIGHTING); softBegin(); glBlendFunc(GL_SRC_ALPHA, GL_ONE); puff({x + 0.3f, 1.25f, z + 0.2f}, 0.45f, {1, .7f, .3f}, 0.2f); glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); softEnd(); glEnable(GL_LIGHTING); }
    }
    // banner between two marchers at the front
    { float bz = std::max(frontZ + 4.0f, 6.0f - t * 1.2f); if (!vigil) { boxc({.95f, .95f, .92f}, 0, 2.2f, bz, 4.2f, 0.9f, 0.03f); boxc({.75f, .1f, .1f}, 0, 2.35f, bz + 0.03f, 3.6f, 0.2f, 0.01f); boxc({.1f, .1f, .1f}, 0, 2.05f, bz + 0.03f, 3.2f, 0.12f, 0.01f);
        for (int s = -1; s <= 1; s += 2) person(s * 2.1f, 0, bz + 0.3f, PI, makeLook(1900 + s, 0), t * 5.5f, bz > frontZ + 4.05f ? 1.0f : 0.0f, 1, false, t, 1900 + s); } }
    // barricades from the event data
    for (size_t k = 0; k < p.barriers.size(); k++) tyrePile(p.barriers[k].x * 1.6f, -6.0f + p.barriers[k].z * 2.2f, t, (float)k);
    // police line with shields, an armoured vehicle behind
    if (p.police || parliament) {
        float pz = parliament ? -17.0f : -19.0f;
        for (int i = 0; i < 16; i++) { float x = -7.0f + i * 0.9f; person(x, 0, pz + std::sin(t * 0.7f + i) * 0.1f, 0, makeLook(1600 + i, 3), 0, 0, 10, false, t, 1600 + i); }
        for (int i = 0; i < 6; i++) person(-6 + i * 2.4f, 0, pz - 1.2f, 0, makeLook(1650 + i, 3), 0, 0, 0, false, t, 1650 + i);
        boxc({.22f, .26f, .3f}, 4.5f, 1.4f, pz - 4, 3.0f, 2.2f, 6.5f); boxc({.15f, .17f, .2f}, 4.5f, 2.7f, pz - 2.5f, 2.6f, 0.7f, 2.4f);
        glDisable(GL_LIGHTING); float fl = std::sin(t * 9) > 0 ? 1.0f : 0.0f; col(fl ? C{.2f, .3f, 1} : C{.9f, .1f, .1f}); box(4.5f, 3.2f, pz - 2.5f, 1.4f, 0.12f, 0.3f); glEnable(GL_LIGHTING);
    }
    // tear gas: canisters arc over, clouds bloom and drift down the street
    if (p.smoke) {
        for (int k = 0; k < 3; k++) { float f = std::fmod(t * 0.45f + k * 0.33f, 1.0f); float zz = mixf(-18, -7 + k * 1.5f, f), yy = 1.6f + std::sin(f * PI) * 4.0f; col({.3f, .3f, .32f}); box(-3.0f + k * 3.0f, yy, zz, 0.12f, 0.3f, 0.12f); }
        for (int k = 0; k < 40; k++) {
            float age = std::fmod(t * 0.16f + hashf(k * 7 + 1), 1.0f);
            puff({-6.0f + 12.0f * hashf(k * 5 + 3) + age * 2.5f, 0.8f + age * 4.0f, -4.0f - 12.0f * hashf(k * 11 + 2) + age * 3.0f}, 1.4f + age * 3.2f, {.85f, .85f, .83f}, 0.42f * std::sin(age * PI));
        }
    } else if (p.energy > 0.5f) dust({0, 1.5f, -8}, {14, 3, 24}, t, 60, 2);
    end2D(W, H);
}

// ================================================================== STATE HOUSE: a podium, flags, the press
void sceneStateHouse(float t, float u, const Params& p, int W, int H) {
    Tod d = tod(p.sky);
    begin(W, H, {.05f, .05f, .06f}, scalec({.38f, .37f, .36f}, d.ik), false, {0, 0, 0}, 0, 0);
    light(0, {0, 7, 0}, scalec({1, .95f, .85f}, mixf(1.0f, d.ik, 0.5f))); light(1, {0, 3, 10}, scalec(mixc({.38f, .4f, .46f}, d.sun, 0.4f), 0.8f));
    float fl = 0; for (int k = 0; k < 14; k++) if (std::fmod(t * 1.7f + hashf(k * 3) * 5.0f, 2.3f) < 0.07f) fl = 1.0f;
    if (fl > 0) light(2, {0, 3, 4}, {1.6f, 1.6f, 1.6f}, 0.0f);
    V3 eye, tgt;
    if (u < 0.6f) { float v = smooth(u / 0.6f); eye = {mixf(-2.0f, 1.0f, v), mixf(3.0f, 2.5f, v), mixf(10.0f, 6.0f, v)}; tgt = {0, 1.6f, -6.5f}; }
    else          { float v = (u - 0.6f) / 0.4f; eye = {mixf(3.0f, 1.5f, v), 1.7f, 0.0f}; tgt = {0, 1.75f, -6.5f}; }
    camera(eye + shake(t, 0.02f), tgt, 55, W, H);
    floorQuad(T_TILE, -9, -10, 9, 12, 0, 0.4f, {.95f, .93f, .9f});
    bool res = p.variant == 1; C wall = res ? C{.12f, .2f, .45f} : C{.82f, .78f, .7f};
    boxc(wall, 0, 4, -10.2f, 18.4f, 8, 0.3f); boxc({.8f, .76f, .68f}, -9.2f, 4, 0, 0.3f, 8, 22); boxc({.8f, .76f, .68f}, 9.2f, 4, 0, 0.3f, 8, 22); boxc({.9f, .88f, .84f}, 0, 8.1f, 0, 18.4f, 0.3f, 22); boxc({.3f, .3f, .3f}, 0, 4, 12.2f, 18.4f, 8, 0.3f);
    for (int i = 0; i < 4; i++) for (int s = -1; s <= 1; s += 2) boxc({.93f, .91f, .86f}, s * 7.8f, 3.5f, -8 + i * 5.0f, 0.8f, 7, 0.8f);
    coatOfArms(0, 5.2f, -10.0f, 1.5f);
    for (int s = -1; s <= 1; s += 2) { flag3(s * 4.6f - 1.0f, 6.4f, -9.6f, 2.2f, t, s); }
    for (int i = 0; i < 3; i++) lightPanel(-5 + i * 5.0f, 7.9f, -3, 2.0f, 1.2f, {1, .97f, .86f});
    if (!res) {
        boxc({.4f, .25f, .13f}, 0, 0.55f, -6.0f, 1.5f, 1.1f, 0.7f); for (int k = -1; k <= 1; k++) { boxc({.1f, .1f, .1f}, k * 0.25f, 1.3f, -5.8f, 0.03f, 0.35f, 0.03f); boxc({.1f, .1f, .1f}, k * 0.25f, 1.5f, -5.75f, 0.08f, 0.06f, 0.1f); }
        person(0, 0, -6.6f, 0, presidentLook(2000), 0, 0, 8, false, t, 2000);
        for (int s = -1; s <= 1; s += 2) for (int k = 0; k < 3; k++) person(s * (2.2f + k * 1.0f), 0, -7.3f, 0, makeLook(2010 + k + (s > 0 ? 5 : 0), k == 1 ? 3 : 2), 0, 0, 0, false, t, 2010 + k);
    } else {
        boxc({.45f, .3f, .17f}, 0, 0.6f, -7.5f, 11, 1.2f, 1.1f); for (int k = 0; k < 7; k++) { chair(-4.5f + k * 1.5f, 0, -8.6f, 0, {.15f, .15f, .2f}, 0.9f); person(-4.5f + k * 1.5f, 0, -8.5f, 0, makeLook(2100 + k, k == 3 ? 4 : 2), 0, 0, k == 3 ? 8 : 3, true, t, 2100 + k); }
        glDisable(GL_LIGHTING); col({.05f, .1f, .12f}); box(6.5f, 4.5f, -9.9f, 4.4f, 2.8f, 0.05f); for (int k = 0; k < 5; k++) { float h = 0.4f + 1.9f * smooth(clamp01(t * 0.15f - k * 0.08f)) * (0.5f + 0.5f * hashf(k * 5)); col(k % 2 ? C{.2f, .8f, .4f} : C{.9f, .3f, .2f}); box(5.2f + k * 0.65f, 3.3f + h / 2, -9.85f, 0.4f, h, 0.05f); } glEnable(GL_LIGHTING);
    }
    // the press: stills photographers, TV cameras, phones
    for (int k = 0; k < 14; k++) { float x = -6.0f + k * 0.9f + (hashf(k) - 0.5f) * 0.3f, z = 1.0f + (k % 3) * 1.1f;
        person(x, 0, z, PI, makeLook(2200 + k, 0), 0, 0, k % 4 == 0 ? 6 : 3, false, t, 2200 + k); }
    for (int k = 0; k < 3; k++) { float x = -3 + k * 3.2f; boxc({.1f, .1f, .12f}, x, 1.5f, 4.0f, 0.5f, 0.45f, 0.9f); boxc({.3f, .3f, .32f}, x, 0.75f, 4.0f, 0.06f, 1.5f, 0.06f); boxc({.1f, .1f, .1f}, x, 1.5f, 3.5f, 0.2f, 0.2f, 0.2f); }
    if (fl > 0) { glDisable(GL_LIGHTING); softBegin(); col({1, 1, 1}, 0.25f); box(0, 0, 0, 0, 0, 0); softEnd(); glEnable(GL_LIGHTING); }
    end2D(W, H);
}

} // namespace

void draw(Scene s, float t, float u, const Params& p, int W, int H) {
    u = clamp01(u);
    switch (s) {
        case COURT:   sceneCourt(t, u, p, W, H); break;
        case CHAMBER: sceneChamber(t, u, p, W, H); break;
        case POLLING:            scenePolling(t, u, p, W, H); break;
        case INAUGURATION:       sceneStadium(t, u, p, W, H); break;
        case PROTEST_STREET:     sceneProtest(t, u, p, W, H, false); break;
        case PROTEST_PARLIAMENT: sceneProtest(t, u, p, W, H, true); break;
        case STATE_HOUSE:        sceneStateHouse(t, u, p, W, H); break;
        default: break;
    }
}

} // namespace world3d
