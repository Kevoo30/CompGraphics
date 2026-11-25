#pragma once
#include "input.h"

// Film mode: a self-playing animated documentary about Kenya, 2022 to 2026.
// Run with:  people_are_watching.exe --film          (plays everything, ESC quits)
//            people_are_watching.exe --film --at 35  (renders the frame at 35 s to film_frame.bmp and exits)
namespace film {
    void  init();                              // loads optional narration / music files
    void  update(float dt, const Input& in);   // advances the timeline; SPACE pauses, LEFT/RIGHT jump between shots
    void  draw();                              // draws the current frame (shots, captions, fades)
    void  seek(float seconds);
    bool  finished();
    float totalTime();
}