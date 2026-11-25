#define SDL_MAIN_HANDLED
#include "render.h"
#include <SDL.h>
#include <SDL_opengl.h>
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <fstream>

namespace {
    SDL_Window*   window = nullptr;
    SDL_GLContext glctx  = nullptr;
    int   W = 0, H = 0;
    bool  quit = false;
    Uint64 startTicks = 0, lastTicks = 0;
    float dt = 0.016f;

    // ---------- triangulation (ear clipping) so concave polygons fill correctly ----------
    float cross(Vec2 a, Vec2 b, Vec2 c) {
        return (b.x - a.x) * (c.y - a.y) - (b.y - a.y) * (c.x - a.x);
    }
    bool inTriangle(Vec2 p, Vec2 a, Vec2 b, Vec2 c) {
        float d1 = cross(a, b, p), d2 = cross(b, c, p), d3 = cross(c, a, p);
        bool neg = d1 < 0 || d2 < 0 || d3 < 0;
        bool pos = d1 > 0 || d2 > 0 || d3 > 0;
        return !(neg && pos);
    }
    std::vector<int> triangulate(const std::vector<Vec2>& v) {
        std::vector<int> tris;
        int n = (int)v.size();
        if (n < 3) return tris;
        std::vector<int> idx(n);
        for (int i = 0; i < n; i++) idx[i] = i;
        float area = 0;                                    // make orientation consistent
        for (int i = 0; i < n; i++) {
            Vec2 a = v[i], b = v[(i + 1) % n];
            area += a.x * b.y - b.x * a.y;
        }
        if (area < 0) std::reverse(idx.begin(), idx.end());

        int guard = n * n;
        while ((int)idx.size() > 3 && guard-- > 0) {
            int m = (int)idx.size();
            bool clipped = false;
            for (int i = 0; i < m; i++) {
                int ia = idx[(i + m - 1) % m], ib = idx[i], ic = idx[(i + 1) % m];
                Vec2 a = v[ia], b = v[ib], c = v[ic];
                float cr = cross(a, b, c);
                if (cr == 0) { idx.erase(idx.begin() + i); clipped = true; break; }  // collinear vertex
                if (cr < 0) continue;                                                // reflex: not an ear
                bool ear = true;
                for (int j = 0; j < m && ear; j++) {
                    int k = idx[j];
                    if (k == ia || k == ib || k == ic) continue;
                    Vec2 p = v[k];
                    if ((p.x == a.x && p.y == a.y) || (p.x == b.x && p.y == b.y) ||
                        (p.x == c.x && p.y == c.y)) continue;
                    if (inTriangle(p, a, b, c)) ear = false;
                }
                if (!ear) continue;
                tris.insert(tris.end(), {ia, ib, ic});
                idx.erase(idx.begin() + i);
                clipped = true;
                break;
            }
            if (!clipped) break;                           // degenerate: fall through to fan
        }
        for (size_t i = 1; i + 1 < idx.size(); i++)        // remaining triangle (or fan fallback)
            tris.insert(tris.end(), {idx[0], idx[i], idx[i + 1]});
        return tris;
    }

    Color lerpColor(Color a, Color b, float t) {
        t = std::max(0.0f, std::min(1.0f, t));
        return { uint8_t(a.r + (b.r - a.r) * t), uint8_t(a.g + (b.g - a.g) * t), uint8_t(a.b + (b.b - a.b) * t) };
    }
}

namespace render {

void init(int w, int h, const char* title) {
    W = w; H = h;
    if (SDL_Init(SDL_INIT_VIDEO) != 0) { std::printf("SDL_Init failed: %s\n", SDL_GetError()); return; }
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1);     // smooth edges (anti-aliasing)
    SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 4);
    window = SDL_CreateWindow(title, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, W, H, SDL_WINDOW_OPENGL);
    if (window) glctx = SDL_GL_CreateContext(window);
    if (!glctx) {                                          // retry without anti-aliasing
        if (window) SDL_DestroyWindow(window);
        SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 0);
        SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 0);
        window = SDL_CreateWindow(title, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, W, H, SDL_WINDOW_OPENGL);
        if (window) glctx = SDL_GL_CreateContext(window);
    }
    if (!glctx) { std::printf("OpenGL context failed: %s\n", SDL_GetError()); return; }
    SDL_GL_SetSwapInterval(1);                             // vsync

    glViewport(0, 0, W, H);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, W, H, 0, -1, 1);                            // pixel coordinates, y down
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_LINE_SMOOTH);
    glEnable(GL_MULTISAMPLE);
    glLineWidth(1.5f);

    startTicks = lastTicks = SDL_GetPerformanceCounter();
}

void shutdown() {
    if (glctx) SDL_GL_DeleteContext(glctx);
    if (window) SDL_DestroyWindow(window);
    SDL_Quit();
}

bool pollQuit() {
    SDL_Event e;
    while (SDL_PollEvent(&e)) {
        if (e.type == SDL_QUIT) quit = true;
        if (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_ESCAPE) quit = true;
    }
    return quit;
}

void beginFrame(Color c) {
    Uint64 now = SDL_GetPerformanceCounter();
    dt = float(double(now - lastTicks) / double(SDL_GetPerformanceFrequency()));
    lastTicks = now;
    if (dt > 0.1f) dt = 0.1f;                              // avoid huge jumps after a pause
    glClearColor(c.r / 255.0f, c.g / 255.0f, c.b / 255.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
}

void endFrame()      { SDL_GL_SwapWindow(window); }
float deltaTime()    { return dt; }
float time()         { return float(double(SDL_GetPerformanceCounter() - startTicks) / double(SDL_GetPerformanceFrequency())); }
int width()          { return W; }
int height()         { return H; }

void fillPolygonGradient(const std::vector<Vec2>& pts, Color c1, Color c2, bool horizontal) {
    std::vector<int> tris = triangulate(pts);
    if (tris.empty()) return;
    float minV = horizontal ? pts[0].x : pts[0].y, maxV = minV;
    for (const Vec2& p : pts) {
        float v = horizontal ? p.x : p.y;
        minV = std::min(minV, v); maxV = std::max(maxV, v);
    }
    float range = std::max(1e-6f, maxV - minV);
    glBegin(GL_TRIANGLES);
    for (int i : tris) {
        Vec2 p = pts[i];
        Color c = lerpColor(c1, c2, ((horizontal ? p.x : p.y) - minV) / range);
        glColor4ub(c.r, c.g, c.b, 255);
        glVertex2f(p.x, p.y);
    }
    glEnd();
}

void fillPolygon(const std::vector<Vec2>& pts, Color c) { fillPolygonGradient(pts, c, c, false); }

void fillPolygonAlpha(const std::vector<Vec2>& pts, Color c, float alpha) {
    std::vector<int> tris = triangulate(pts);
    glColor4ub(c.r, c.g, c.b, (GLubyte)(std::max(0.0f, std::min(1.0f, alpha)) * 255));
    glBegin(GL_TRIANGLES);
    for (int i : tris) glVertex2f(pts[i].x, pts[i].y);
    glEnd();
}

void fillCircle(Vec2 ctr, float r, Color c) {
    int seg = std::max(16, std::min(128, int(r * 2)));
    glColor4ub(c.r, c.g, c.b, 255);
    glBegin(GL_TRIANGLE_FAN);
    glVertex2f(ctr.x, ctr.y);
    for (int i = 0; i <= seg; i++) {
        float a = 6.2831853f * i / seg;
        glVertex2f(ctr.x + std::cos(a) * r, ctr.y + std::sin(a) * r);
    }
    glEnd();
}

void drawPolygon(const std::vector<Vec2>& pts, Color c) {
    glColor4ub(c.r, c.g, c.b, 255);
    glBegin(GL_LINE_LOOP);
    for (const Vec2& p : pts) glVertex2f(p.x, p.y);
    glEnd();
}

void drawLine(Vec2 a, Vec2 b, Color c) {
    glColor4ub(c.r, c.g, c.b, 255);
    glBegin(GL_LINES);
    glVertex2f(a.x, a.y);
    glVertex2f(b.x, b.y);
    glEnd();
}

void drawCircle(Vec2 ctr, float r, Color c) {
    int seg = std::max(16, std::min(128, int(r * 2)));
    glColor4ub(c.r, c.g, c.b, 255);
    glBegin(GL_LINE_LOOP);
    for (int i = 0; i < seg; i++) {
        float a = 6.2831853f * i / seg;
        glVertex2f(ctr.x + std::cos(a) * r, ctr.y + std::sin(a) * r);
    }
    glEnd();
}

void saveScreenshot(const char* path) {
    std::vector<unsigned char> px(W * H * 3);
    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    glReadPixels(0, 0, W, H, GL_RGB, GL_UNSIGNED_BYTE, px.data());   // rows come bottom-up, like BMP

    int rowSize = (3 * W + 3) & ~3;
    uint32_t imgSize = rowSize * H, fileSize = 54 + imgSize;
    unsigned char hdr[54] = {'B', 'M'};
    auto put32 = [&](int o, uint32_t v) { for (int i = 0; i < 4; i++) hdr[o + i] = (v >> (8 * i)) & 0xFF; };
    put32(2, fileSize); put32(10, 54); put32(14, 40); put32(18, W); put32(22, H);
    hdr[26] = 1; hdr[28] = 24; put32(34, imgSize);
    std::ofstream f(path, std::ios::binary);
    if (!f) { std::printf("Could not write %s\n", path); return; }
    f.write((char*)hdr, 54);
    std::vector<unsigned char> row(rowSize, 0);
    for (int y = 0; y < H; y++) {
        for (int x = 0; x < W; x++) {
            const unsigned char* p = &px[(y * W + x) * 3];
            row[3 * x] = p[2]; row[3 * x + 1] = p[1]; row[3 * x + 2] = p[0];   // RGB -> BGR
        }
        f.write((char*)row.data(), rowSize);
    }
    std::printf("Saved %s\n", path);
}

}