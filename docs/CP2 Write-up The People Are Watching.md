# CP2 Write-up: The People Are Watching

Nov 15, 2026 · @Wayne

## Project overview

The People Are Watching is an animation in the style of the XYZ Show, a Kenyan satirical puppet programme: a speaker at a podium in front of a government building, with an audience watching.

For Checkpoint 2 we built the still scene in pure C++. Every pixel is set by code we wrote ourselves through one function, `putPixel`. SDL2 only shows the finished pixel buffer in a window, and nothing in the scene comes from a drawing library.

The CP2 scene is an 800 x 600 frame with a sky, ground, a columned government building with a clock, a Kenyan flag on a pole, a podium, and a speaker built from polygons and circles. A DDA versus Bresenham line comparison sits in the top-left corner.

## Graphics concepts used

CP2 rests on six ideas from the course, all implemented in pure C++.

- **Raster framebuffer.** The screen is an 800 x 600 array of RGB pixels in memory. `putPixel` is the only way anything is drawn, and pixels outside the window are dropped.
- **Scan conversion.** Lines, circles and polygons are turned into pixels by the algorithms in the next section.
- **Painter's algorithm.** Shapes are drawn back to front: sky, ground, building, flag, podium, then the speaker, so nearer objects cover farther ones.
- **Object space to screen space.** The speaker is defined in local coordinates measured from the feet, then scaled, mirrored and translated onto the screen. This lets us resize and flip characters.
- **Colour.** 24-bit RGB, 8 bits per channel.
- **Screenshots.** A hand-written 24-bit BMP writer saves the frame, with no image library.

## Algorithms implemented

Six drawing algorithms are implemented and used to draw the CP2 scene, on top of the framebuffer they write to. DDA appears only in the line-comparison demo.

| Algorithm | Function (file) | Owner | Used in the scene for |
| --- | --- | --- | --- |
| DDA line | `drawLineDDA` (lines.cpp) | Person 2 | Red line in the comparison demo |
| Bresenham line | `drawLineBresenham` (lines.cpp) | Person 2 | Blue demo line, flag spears, building steps and column edges, clock hands, microphone stand, speaker's mouth |
| Midpoint circle, outline | `drawCircleMidpoint` (circle.cpp) | Person 3 | Clock rim, speaker's head outline |
| Midpoint circle, filled | `fillCircleMidpoint` (circle.cpp) | Person 3 | Clock face, flagpole ball, microphone, speaker's head and eyes |
| Polygon outline | `drawPolygon` (polygon.cpp) | Person 4 | Edges of the building wall, door, pediment, podium, flag shield and speaker's body parts |
| Scanline polygon fill | `fillPolygonScanline` (polygon.cpp) | Person 4 | Every solid shape: wall, columns, steps, flag bands and shield, podium, speaker's body |
| Framebuffer, `putPixel`, BMP writer | graphics.cpp | Person 1 | Everything above draws through it; screenshots |

How each one works:

- **DDA:** steps along the longer axis one pixel at a time, adding a fractional increment to the other axis and rounding it.
- **Bresenham:** draws the same line with integers only, using an error term that decides when to step sideways. It handles all eight octants.
- **Midpoint circle:** plots one eighth of the circle with a decision parameter that starts at 1 - r, then mirrors each point into the other seven.
- **Filled circle:** the same loop, drawing horizontal runs of pixels instead of single points.
- **Scanline fill:** for each row, finds where the polygon's edges cross it, sorts the crossings and fills between pairs (even-odd rule), so concave shapes fill correctly.

## Screenshot of the current scene

The scene below comes from the CP2 software renderer: every shape is produced by our own line, circle and polygon code. To regenerate it, run `people_are_watching.exe --software --shot`, which saves `screenshot_software.bmp`.

&#91;image: CP2 scene: government building with columns and clock, Kenyan flag on a pole, podium and speaker, with DDA (red) and Bresenham (blue) demo lines in the top-left corner\]

## GitHub repository and how to run

The code is at [github.com/Kevoo30/CompGraphics](https://github.com/Kevoo30/CompGraphics), branch `main`. To build and run it on Windows with MSYS2 (MinGW64):

1. Install the tools once: `pacman -S --needed mingw-w64-x86_64-SDL2 mingw-w64-x86_64-cmake mingw-w64-x86_64-ninja`
2. From the repository folder, configure and build: `cmake -S . -B build -G Ninja`, then `cmake --build build`
3. Run `.\build\people_are_watching.exe --software` for the CP2 scene. Without `--software` it opens the OpenGL version, which is CP3 work in progress. Add `--shot` to save a screenshot and exit.

## Team contributions

Five people each owned separate files, so merge conflicts stayed rare.

| Person | Role | Files | Delivered for CP2 |
| --- | --- | --- | --- |
| Person 1 | Lead, framework, integration | graphics.h/.cpp, main.cpp, CMakeLists.txt | SDL2 window, `putPixel`, clear, present, BMP screenshots, the GitHub repository, final scene assembly |
| Person 2 | Line algorithms | lines.h/.cpp | DDA and Bresenham for all slopes and octants, side-by-side demo |
| Person 3 | Circle algorithm | circle.h/.cpp | Midpoint circle, outline and filled |
| Person 4 | Polygons and filling | polygon.h/.cpp | Polygon outline and scanline fill |
| Person 5 (Wayne) | Scene assets and documentation | scene.h/.cpp, docs/ | Government building, Kenyan flag, podium, polygon-built speaker, this write-up |

## What remains

The CP2 drawing work is done. What is left is the Checkpoint 3 build (week of 16 Nov: transformations, animation, OpenGL) and the final demo (week of 23 Nov).

The repository already holds a start on CP3: an OpenGL renderer (Person 1), 2D matrix maths for translate, rotate, scale and rotate-about-a-point (Person 2), keyboard controls, and placeholder animation and crowd code. Still to do:

1. **Transformations (Person 2):** `shear`, `reflectX` and `reflectY` in transform.cpp are stubs that return the identity matrix.
2. **Animation (Person 3):** `updateAnimations` is empty and the flag wave is a placeholder sine wave. It needs a flag wave pinned at the pole, cloud drift, a speaker arm gesture and camera moves.
3. **Audience and controls (Person 4):** the crowd is simple bobbing shapes. It should reuse the real person model, vary clothes and size, and add applause. Input still needs mouse controls.
4. **OpenGL scene (Person 5):** `scene_gl.cpp` is a placeholder. The real building, flag, podium and speaker need porting from scene.cpp, then gradients, shadows and the flag shield.
5. **The 5-minute animation (whole team):** the code has no scene sequencer or timeline yet. The script needs splitting into scenes with keyframes and easing to reach five minutes.

Cleanups to the CP2 code:

- polygon.cpp carries its own copy of Bresenham; it should call `drawLineBresenham` from lines.cpp.
- Scanline fill checks every edge on every row. An edge table or active edge list would be faster once crowds are drawn.
- In the software scene the speaker is drawn in front of the podium instead of behind it (main.cpp).
