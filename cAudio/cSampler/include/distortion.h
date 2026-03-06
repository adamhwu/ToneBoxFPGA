#pragma once
#include "effect.h"
#include <string>

class Distortion : public Effect {
public:
    Distortion(float drive, float mix, std::string mode);

    // Process a single sample
    float process(float input) override;

    float drive;
    float mix;
    std::string mode;
    float marcoDistortion(float x, float drive);
    float softClipPoly(float x);
    float hardClip(float x);
    float softClip(float x);
    float screamer(float x);
};