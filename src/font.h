#pragma once
#include "graphics.h"   // Color

// Tiny pixel font so the film can show years, places and captions (OpenGL has no built-in text).
// Supports A-Z, 0-9, space and  - , . : ' / ( ) &   (lowercase is shown as uppercase)
namespace font {
    float width(const char* text, float px);                                   // width in screen pixels
    void  draw(const char* text, float x, float y, float px, Color c,
               float alpha = 1.0f, bool centered = false, bool shadow = true); // x,y = top-left (or top-centre)
    float fit(const char* text, float maxWidth, float px);                     // shrinks px until the text fits
}