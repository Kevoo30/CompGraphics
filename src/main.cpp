#include "graphics.h"
#include "lines.h"
#include "circle.h"
#include "polygon.h"
#include "scene.h"
#include <cstdlib>
#include <cstring>

// Usage:  ./people_are_watching            -> opens window
//         ./people_are_watching --shot     -> renders one frame, saves screenshot.bmp, exits
int main(int argc, char** argv) {
    bool shotOnly = (argc > 1 && std::strcmp(argv[1], "--shot") == 0);
    gfx::init(800, 600, "The People Are Watching");

    auto drawFrame = [] {
        gfx::clear({135, 206, 235});            // sky blue
        drawBuilding();
        drawFlag(600, 100);
        drawPodium(400, 450);
        drawPerson(400, 380);

        // Line algorithm demo (Person 2's work): DDA (red) vs Bresenham (blue)
        drawLineDDA(20, 20, 220, 90, {220, 0, 0});
        drawLineBresenham(20, 30, 220, 100, {0, 0, 220});
    };

    if (shotOnly) {
        drawFrame();
        gfx::saveBMP("screenshot.bmp");
        gfx::shutdown();
        return 0;
    }

    bool saved = false;
    while (!gfx::pollQuit()) {
        drawFrame();
        gfx::present();
        if (!saved) { gfx::saveBMP("screenshot.bmp"); saved = true; }
    }
    gfx::shutdown();
    return 0;
}
