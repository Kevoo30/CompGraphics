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
#include "audio.h"
#include <cstring>
#include <cstdlib>
#include <cstdio>
#include <cmath>
#include <chrono>
#include <filesystem>
#include <sstream>
#include <string>
#include <vector>

#ifdef _WIN32
#define PAW_POPEN _popen
#define PAW_PCLOSE _pclose
#else
#define PAW_POPEN popen
#define PAW_PCLOSE pclose
#endif

// Usage:
//   people_are_watching.exe              OpenGL version (CP3)
//   people_are_watching.exe --software   original pure C++ software version (CP2)
//   people_are_watching.exe --transform  Person 2's transformations demo (SPACE pause, P screenshot)
//   people_are_watching.exe --film       the 2022-2026 documentary (SPACE pause, LEFT/RIGHT change shot, ESC quit)
//   people_are_watching.exe --film --at 35   save the film frame at 35 s to film_frame.bmp and exit
//   people_are_watching.exe --export-video out.mp4   render the film at 1080p / 30 fps
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

static std::string shellPath(const std::string& path) {
#ifdef _WIN32
    // cmd.exe expands these characters even inside double quotes.
    if (path.find_first_of("\"&|<>^%!\r\n") != std::string::npos) return {};
    return "\"" + path + "\"";
#else
    std::string quoted = "'";
    for (char c : path) {
        if (c == '\'') quoted += "'\\''";
        else quoted += c;
    }
    return quoted + "'";
#endif
}

static int runVideoExport(const std::string& outputPath) {
    const std::string quotedOutput = shellPath(outputPath);
    if (quotedOutput.empty()) { std::fprintf(stderr, "Output path contains unsupported shell characters.\n"); return 2; }

    render::init(1920, 1080, "People Are Watching export", true);
    if (!render::isReady()) { std::fprintf(stderr, "Could not create the hidden OpenGL context.\n"); return 2; }
    film::init();
    const float duration = film::totalTime();
    if (!(duration > 0.0f)) { std::fprintf(stderr, "Film timeline is empty.\n"); render::shutdown(); return 2; }
    const std::vector<film::AudioCue> cues = film::audioCues();

    const auto stamp = std::chrono::steady_clock::now().time_since_epoch().count();
    const std::string silentPath = (std::filesystem::temp_directory_path() / ("paw_video_" + std::to_string(stamp) + ".mp4")).string();
    const std::string quotedSilent = shellPath(silentPath);
    if (quotedSilent.empty()) { render::shutdown(); return 2; }
    const char* fps = "30";
    std::ostringstream encode;
    encode << "ffmpeg -hide_banner -loglevel warning -y -f rawvideo -pixel_format rgb24 -video_size 1920x1080 -framerate " << fps
           << " -i - -an -c:v libx264 -preset medium -crf 20 -pix_fmt yuv420p " << quotedSilent;
    FILE* pipe = PAW_POPEN(encode.str().c_str(), "wb");
    if (!pipe) { std::fprintf(stderr, "Could not start ffmpeg. Ensure ffmpeg is on PATH.\n"); render::shutdown(); return 2; }

    const int frameCount = static_cast<int>(std::ceil(duration * 30.0f));
    std::vector<unsigned char> pixels;
    bool writeFailed = false;
    for (int frame = 0; frame < frameCount; ++frame) {
        film::seek(static_cast<float>(frame) / 30.0f);
        render::beginFrame({0, 0, 0});
        film::draw();
        if (!render::readPixelsRGB(pixels) || std::fwrite(pixels.data(), 1, pixels.size(), pipe) != pixels.size()) {
            writeFailed = true;
            break;
        }
        render::endFrame();
        if (frame % 900 == 0) std::printf("Rendered %d / %d frames\n", frame, frameCount);
    }
    int encoderStatus = PAW_PCLOSE(pipe);
    render::shutdown();
    if (writeFailed || encoderStatus != 0) {
        std::fprintf(stderr, "Video encoding failed while rendering frames.\n");
        std::remove(silentPath.c_str());
        return 2;
    }

    std::ostringstream mux;
    mux << "ffmpeg -hide_banner -loglevel warning -y -i " << quotedSilent;
    std::vector<int> audioInputs;
    int input = 1;
    for (const auto& cue : cues) {
        std::string quoted = shellPath(cue.path);
        if (quoted.empty()) continue;
        if (cue.loop) mux << " -stream_loop -1";
        mux << " -i " << quoted;
        audioInputs.push_back(input++);
    }
    mux << " -map 0:v:0";
    if (audioInputs.empty()) {
        mux << " -an -c:v copy " << quotedOutput;
    } else {
        mux << " -filter_complex \"";
        int audioIndex = 0;
        for (const auto& cue : cues) {
            std::string quoted = shellPath(cue.path);
            if (quoted.empty()) continue;
            const int source = audioInputs[audioIndex];
            if (cue.loop) {
                mux << "[" << source << ":a]volume=" << cue.volume << ",atrim=duration=" << duration << "[a" << audioIndex << "];";
            } else {
                mux << "[" << source << ":a]adelay=" << static_cast<int>(std::lround(cue.start * 1000.0f))
                    << ":all=1,volume=" << cue.volume << "[a" << audioIndex << "];";
            }
            ++audioIndex;
        }
        for (int i = 0; i < audioIndex; ++i) mux << "[a" << i << "]";
        mux << "amix=inputs=" << audioIndex << ":duration=longest:normalize=0,apad,atrim=duration=" << duration << "[outa]\""
            << " -map \"[outa]\" -c:v copy -c:a aac -b:a 192k " << quotedOutput;
    }
    int muxStatus = std::system(mux.str().c_str());
    std::remove(silentPath.c_str());
    if (muxStatus != 0) { std::fprintf(stderr, "Audio mux failed. Check ffmpeg and the WAV files.\n"); return 2; }
    std::printf("Exported %s (%d frames, 1080p, 30 fps)\n", outputPath.c_str(), frameCount);
    return 0;
}

int main(int argc, char** argv) {
    bool software = false, shot = false, transformDemo = false, filmMode = false, haveAt = false, exportRequested = false;
    std::string exportPath;
    float atSeconds = 0.0f;
    for (int i = 1; i < argc; i++) {
        if (std::strcmp(argv[i], "--software") == 0) software = true;
        if (std::strcmp(argv[i], "--shot") == 0) shot = true;
        if (std::strcmp(argv[i], "--transform") == 0) transformDemo = true;
        if (std::strcmp(argv[i], "--film") == 0) filmMode = true;
        if (std::strcmp(argv[i], "--at") == 0 && i + 1 < argc) { haveAt = true; atSeconds = (float)std::atof(argv[++i]); }
        if (std::strcmp(argv[i], "--export-video") == 0) {
            exportRequested = true;
            if (i + 1 < argc) exportPath = argv[++i];
        }
    }
    if (exportRequested) {
        if (exportPath.empty()) { std::fprintf(stderr, "Usage: people_are_watching.exe --export-video out.mp4\n"); return 2; }
        return runVideoExport(exportPath);
    }
    if (filmMode) return runFilm(haveAt, atSeconds);
    if (transformDemo) return runTransformDemo(shot);
    return software ? runSoftware(shot) : runOpenGL(shot);
}
