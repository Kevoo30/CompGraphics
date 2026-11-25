#include "graphics.h"     // CP2 software renderer
#include "lines.h"
#include "circle.h"
#include "polygon.h"
#include "scene.h"        // CP2 software scene
#include "render.h"       // CP3 OpenGL renderer
#include "input.h"
#include "animation.h"
#include "scene_gl.h"
#include "transform_demo.h"
#include "film.h"
#include "interior3d.h"
#include "audio.h"
#include <cstring>
#include <cstdlib>

// Usage:
//   people_are_watching.exe              documentary film (realism version)
//   people_are_watching.exe --software   original pure C++ software version (CP2)
//   people_are_watching.exe --transform  Person 2's transformations demo (SPACE pause, P screenshot)
//   people_are_watching.exe --film       the 2022-2026 documentary (SPACE pause, LEFT/RIGHT change shot, ESC quit)
//   people_are_watching.exe --film --at 35   save the film frame at 35 s to film_frame.bmp and exit
//   add --shot to any of them: render one frame, save the screenshot and exit

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

static int runTransformDemo(bool shotOnly) {
    render::init(800, 600, "Transformations demo (Person 2)");
    float t = shotOnly ? 2.2f : 0.0f;                     // screenshot mode: a frame with everything mid-motion
    bool paused = shotOnly;
    int frame = 0;
    while (!render::pollQuit()) {
        Input in = pollInput();
        if (in.pause) paused = !paused;

        render::beginFrame({0, 0, 0});
        if (!paused) t += render::deltaTime();
        drawTransformDemo(t, {(float)in.mouseX, (float)in.mouseY});
        render::endFrame();

        if (in.screenshot) render::saveScreenshot("screenshot_transform.bmp");
        if (shotOnly && ++frame == 3) {
            render::saveScreenshot("screenshot_transform.bmp");
            break;
        }
    }
    render::shutdown();
    return 0;
}

static int runFilm(bool haveAt, float atSeconds) {
    render::init(800, 600, "The People Are Watching - 2022 to 2026");
    if (!haveAt) audio::init();                            // no sound when just checking a frame
    film::init();
    if (haveAt) {
        film::seek(atSeconds);
        film::update(0.0f, Input{});
        render::beginFrame({0, 0, 0});
        film::draw();
        render::endFrame();
        render::beginFrame({0, 0, 0});                     // draw twice so the buffer being read is the finished frame
        film::draw();
        render::saveScreenshot("film_frame.bmp");
        render::endFrame();
        render::shutdown();
        return 0;
    }
    while (!render::pollQuit() && !film::finished()) {
        Input in = pollInput();
        audio::update();
        film::update(render::deltaTime(), in);
        render::beginFrame({0, 0, 0});
        film::draw();
        render::endFrame();
        if (in.screenshot) render::saveScreenshot("screenshot_film.bmp");
    }
    audio::shutdown();
    render::shutdown();
    return 0;
}

// Debug: render one 3D scene frame.   people_are_watching --s3d <scene 1-7> <u 0..1> [seconds] [crowd] [flags: smoke police night candles mourning vote]
static int runScene3D(int argc, char** argv) {
    int id = 1; float u = 0.0f, t = 3.0f; world3d::Params p; p.vote = false;
    for (int i = 1; i < argc; i++) if (!std::strcmp(argv[i], "--s3d") && i + 3 < argc + 1) {
        id = std::atoi(argv[i + 1]); u = (float)std::atof(argv[i + 2]); if (i + 3 < argc) t = (float)std::atof(argv[i + 3]);
        if (i + 4 < argc) p.crowd = std::atoi(argv[i + 4]);
        for (int j = i + 5; j < argc; j++) {
            if (!std::strcmp(argv[j], "smoke")) p.smoke = true; if (!std::strcmp(argv[j], "police")) p.police = true; if (!std::strcmp(argv[j], "night")) p.night = true;
            if (!std::strcmp(argv[j], "candles")) p.candles = true; if (!std::strcmp(argv[j], "mourning")) p.mourning = true; if (!std::strcmp(argv[j], "vote")) p.vote = true;
            if (!std::strcmp(argv[j], "results")) p.variant = 1;
        }
    }
    render::init(800, 600, "scene3d");
    for (int k = 0; k < 2; k++) { render::beginFrame({0, 0, 0}); world3d::draw((world3d::Scene)id, t, u, p, 800, 600); if (k == 1) render::saveScreenshot("s3d.bmp"); render::endFrame(); }
    render::shutdown();
    return 0;
}

int main(int argc, char** argv) {
    for (int i = 1; i < argc; i++) if (!std::strcmp(argv[i], "--s3d")) return runScene3D(argc, argv);
    bool software = false, shot = false, transformDemo = false, filmMode = false, haveAt = false, modeSpecified = false;
    float atSeconds = 0.0f;
    for (int i = 1; i < argc; i++) {
        if (std::strcmp(argv[i], "--software") == 0) { software = true; modeSpecified = true; }
        if (std::strcmp(argv[i], "--shot") == 0) shot = true;
        if (std::strcmp(argv[i], "--transform") == 0) { transformDemo = true; modeSpecified = true; }
        if (std::strcmp(argv[i], "--film") == 0) { filmMode = true; modeSpecified = true; }
        if (std::strcmp(argv[i], "--at") == 0 && i + 1 < argc) { haveAt = true; atSeconds = (float)std::atof(argv[++i]); }
    }
    if (!modeSpecified) filmMode = true;
    if (filmMode) return runFilm(haveAt, atSeconds);
    if (transformDemo) return runTransformDemo(shot);
    return software ? runSoftware(shot) : runOpenGL(shot);
}
