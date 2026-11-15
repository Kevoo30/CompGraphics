# CP3 evidence - Person 5 (Scene)

## What I did
- `src/effects.h/.cpp`: OpenGL port of the CP2 scene (building, flag, podium, person) using only `render::` + our own `Mat3` maths.
- `src/scene_gl.cpp`: wires sky, ground, building, flag, speaker, podium, clouds and crowd together in back-to-front order.
- `src/scene.h`: `PersonStyle` gained an optional `hair` colour (existing 5-field initialisers still compile).
- `src/scene.cpp` (CP2 software scene) is unchanged, so `--software` still works for the comparison.

## Techniques to point at (CP3 marks)
| Technique | Where |
|---|---|
| Vertical / horizontal colour gradients | sky, lawn, steps, wall, podium, suit, column shading (`grad()` in effects.cpp) |
| Cast shadows by projection (light upper-left) | `castShadow()`: building, flag pole, podium, speaker |
| Alpha blending | shadows, sun halo, column shadows on the wall (`render::fillPolygonAlpha`) |
| Matrix transforms | camera `Mat3`, raised arm via `rotateAbout()`, clock hands via `rotateAbout()`, `facing = -1` mirroring |
| Animation hooks | `flagWave()` warps every cloth/shield vertex; fold shading comes from the wave slope; `speakerArmAngle()` drives the arm |
| Concave polygons | pediment, shield, hair cap (triangulated inside `render.cpp`) |

## Evidence to capture
Run `docs/collect_evidence.sh` from the repo root (needs a built `people_are_watching`). It saves:
1. `docs/evidence/software.bmp`  - CP2 renderer (`--software --shot`)
2. `docs/evidence/opengl.bmp`    - CP3 renderer (`--shot`)
Then, by hand (needs the window):
3. Run normally, press **G** while the arm is down, press **P** mid-gesture -> `screenshot_opengl.bmp` (arm raised + waving flag).
4. Arrow keys + **E** to zoom on the flag -> second screenshot (shows fold shading and the shield rippling).
5. Zoom on the podium base -> shadow and gradient detail.
6. `git log --oneline --author=<you>` pasted into this file as proof of contribution.

## Known limits (be honest in the write-up)
- Flag pole is fixed at x = 700 because `flagWave()` measures from x = 702.
- Clouds are drawn in screen space (Person 3), so they do not follow the camera: this reads as distant parallax.
- Shadows are projected silhouettes, not real lighting; the speaker's shadow is mostly hidden behind the podium.
