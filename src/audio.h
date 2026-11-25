#pragma once

// Audio for the project. Built on SDL2's audio device (nothing extra to install).
// The mixing is our own code: several sounds can play at once.
//
// Controls handled by audio::update():   A = applause   V = speech (assets/speech.wav)   M = mute
//
namespace audio {
    bool init();                                    // call once, after render::init. Returns false if no audio device (everything else then does nothing)
    void shutdown();                                // call before render::shutdown

    int  load(const char* wavPath);                 // load a .wav file (any rate/channels). Returns a sound id, or -1 if the file is missing
    float length(int id);                           // length of a loaded sound in seconds (0 if the id is not valid)
    void play(int id, float volume = 1.0f, bool loop = false);
    void stopAll(bool keepAmbience = false);        // keepAmbience = true leaves the looping crowd murmur playing
    void setMuted(bool muted);

    int  applause();                                // id of the built-in generated applause (no file needed)
    void update();                                  // call once per frame: handles the keys above
}