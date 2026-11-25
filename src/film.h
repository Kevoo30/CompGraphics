#pragma once
#include "input.h"
#include <string>
#include <vector>

// Film mode: a self-playing animated documentary about Kenya, 2022 to 2026.
// Run with:  people_are_watching.exe --film          (plays everything, ESC quits)
//            people_are_watching.exe --film --at 35  (renders the frame at 35 s to film_frame.bmp and exits)
namespace film {
    struct AudioCue { std::string path; float start = 0.0f; float volume = 1.0f; bool loop = false; };
    void  init();                              // loads optional narration / music files
    void  update(float dt, const Input& in);   // advances the timeline; SPACE pauses, LEFT/RIGHT jump between shots
    void  draw();                              // draws the current frame (shots, captions, fades)
    void  seek(float seconds);
    bool  finished();
    float totalTime();
    std::vector<AudioCue> audioCues();
}
