#define SDL_MAIN_HANDLED
#include "graphics.h"
#include <SDL.h>
#include <fstream>
#include <cstdio>

namespace {
    SDL_Window*   window   = nullptr;
    SDL_Renderer* renderer = nullptr;
    SDL_Texture*  texture  = nullptr;
    std::vector<uint32_t> buffer;   // 0xAARRGGBB
    int W = 0, H = 0;
    bool quit = false;
    bool lastKey[512] = {false};
}

namespace gfx {

void init(int w, int h, const char* title) {
    W = w; H = h;
    buffer.assign(W * H, 0xFF000000);
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        std::printf("SDL_Init failed: %s\n", SDL_GetError());
        return;
    }
    window = SDL_CreateWindow(title, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, W, H, 0);
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_SOFTWARE);
    texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888,
                                SDL_TEXTUREACCESS_STREAMING, W, H);
}

void putPixel(int x, int y, Color c) {
    if (x < 0 || y < 0 || x >= W || y >= H) return;
    buffer[y * W + x] = 0xFF000000u | (uint32_t(c.r) << 16) | (uint32_t(c.g) << 8) | c.b;
}

void clear(Color c) {
    uint32_t v = 0xFF000000u | (uint32_t(c.r) << 16) | (uint32_t(c.g) << 8) | c.b;
    for (auto& p : buffer) p = v;
}

void present() {
    if (!texture) return;
    SDL_UpdateTexture(texture, nullptr, buffer.data(), W * sizeof(uint32_t));
    SDL_RenderClear(renderer);
    SDL_RenderCopy(renderer, texture, nullptr, nullptr);
    SDL_RenderPresent(renderer);
}

bool pollQuit() {
    SDL_Event e;
    while (SDL_PollEvent(&e)) {
        if (e.type == SDL_QUIT) quit = true;
        if (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_ESCAPE) quit = true;
    }
    SDL_Delay(16);
    return quit;
}

bool keyPressed(int sdlKeycode) {
    const Uint8* s = SDL_GetKeyboardState(nullptr);
    return s[SDL_GetScancodeFromKey(sdlKeycode)] != 0;
}

void saveBMP(const char* path) {
    // 24-bit uncompressed BMP, written by hand (no SDL needed)
    int rowSize = (3 * W + 3) & ~3;
    uint32_t imgSize = rowSize * H, fileSize = 54 + imgSize;
    unsigned char hdr[54] = {'B','M'};
    auto put32 = [&](int o, uint32_t v){ for (int i=0;i<4;i++) hdr[o+i] = (v >> (8*i)) & 0xFF; };
    put32(2, fileSize); put32(10, 54); put32(14, 40);
    put32(18, W); put32(22, H);
    hdr[26] = 1; hdr[28] = 24; put32(34, imgSize);
    std::ofstream f(path, std::ios::binary);
    if (!f) { std::printf("Could not write %s\n", path); return; }
    f.write((char*)hdr, 54);
    std::vector<unsigned char> row(rowSize, 0);
    for (int y = H - 1; y >= 0; y--) {          // BMP rows go bottom-up
        for (int x = 0; x < W; x++) {
            uint32_t p = buffer[y * W + x];
            row[3*x]   = p & 0xFF;              // B
            row[3*x+1] = (p >> 8) & 0xFF;       // G
            row[3*x+2] = (p >> 16) & 0xFF;      // R
        }
        f.write((char*)row.data(), rowSize);
    }
    std::printf("Saved %s\n", path);
}

void shutdown() {
    if (texture) SDL_DestroyTexture(texture);
    if (renderer) SDL_DestroyRenderer(renderer);
    if (window) SDL_DestroyWindow(window);
    SDL_Quit();
}

int width()  { return W; }
int height() { return H; }

}
