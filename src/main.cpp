#include "graphics.h"     // CP2 software renderer
#include "lines.h"
#include "circle.h"
#include "polygon.h"
#include "scene.h"        // CP2 software scene
#include "render.h"       // CP3 OpenGL renderer
#include "input.h"
#include "animation.h"
#include "scene_gl.h"
#include <cstring>

// Usage:
//   people_are_watching.exe              OpenGL version (CP3)
//   people_are_watching.exe --software   original pure C++ software version (CP2)
//   add --shot to either: render one frame, save the screenshot and exit

static int runSoftware(bool shotOnly) {
    gfx::init(800, 600, "The People Are Watching (software renderer)");
    auto drawFrame = [] {
        gfx::clear({135, 206, 235});
        drawBuilding();
        drawFlag(600, 100);
        drawPodium(400, 450);
        drawPerson(400, 380);
        drawLineDDA(20, 20, 220, 90, {220, 0, 0});          // DDA vs Bresenham demo
        drawLineBresenham(20, 30, 220, 100, {0, 0, 220});
    };
    if (shotOnly) {
        drawFrame();
        gfx::saveBMP("screenshot_software.bmp");
        gfx::shutdown();
        return 0;
    }
    bool saved = false;
    while (!gfx::pollQuit()) {
        drawFrame();
        gfx::present();
        if (!saved) { gfx::saveBMP("screenshot_software.bmp"); saved = true; }
    }
    gfx::shutdown();
    return 0;
}

static int runOpenGL(bool shotOnly) {
    render::init(800, 600, "The People Are Watching (OpenGL)");
    int frame = 0;
    while (!render::pollQuit()) {
        Input in = pollInput();
        updateAnimations(render::deltaTime());

        render::beginFrame({0, 0, 0});
        drawScene(render::time(), in);
        render::endFrame();

        if (in.screenshot) render::saveScreenshot("screenshot_opengl.bmp");
        if (shotOnly && ++frame == 3) {                    // let the first frames settle, then capture
            render::saveScreenshot("screenshot_opengl.bmp");
            break;
        }
    }
    render::shutdown();
    return 0;
}

int main(int argc, char** argv) {
    bool software = false, shot = false;
    for (int i = 1; i < argc; i++) {
        if (std::strcmp(argv[i], "--software") == 0) software = true;
        if (std::strcmp(argv[i], "--shot") == 0) shot = true;
    }
    return software ? runSoftware(shot) : runOpenGL(shot);
}