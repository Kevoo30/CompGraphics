#include "crowd.h"
#include "render.h"
#include <cmath>

// Working placeholder: two rows of simple people bobbing. Person 4: reuse the real person model,
// vary size/colour/clothes, add applause (arms up), phones, flashes, etc.
void drawCrowd(float time, const Mat3& cam) {
    const Color skin[] = {{141, 85, 36}, {198, 134, 66}, {224, 172, 105}, {94, 56, 30}};
    const Color shirt[] = {{40, 60, 110}, {120, 30, 30}, {50, 90, 60}, {90, 90, 95}, {160, 120, 40}};
    const std::vector<Vec2> body = {{-18, -45}, {18, -45}, {22, 0}, {-22, 0}};
    float zoomScale = std::hypot(cam.m[0], cam.m[3]);

    for (int row = 0; row < 2; row++) {
        for (int i = 0; i < 8; i++) {
            float x = 50 + i * 100 + row * 50;
            float y = 540 + row * 50;
            float bob = std::fabs(std::sin(time * 3.0f + i * 1.3f + row)) * 5.0f;     // applause bounce
            float s = 1.0f + row * 0.15f;                                              // front row bigger
            Mat3 m = cam * translate(x, y - bob) * scale(s, s);
            render::fillPolygon(transformPoly(m, body), shirt[(i + row) % 5]);
            Vec2 head = m * Vec2{0, -58};
            render::fillCircle(head, 13.0f * s * zoomScale, skin[(i * 3 + row) % 4]);
        }
    }
}