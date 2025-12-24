#pragma once
#include "effect.h"

class Distortion : public Effect {
public:
    Distortion(float drive, float mix, int mode);

    // Process a single sample
    float process(float input) override;

private:
    float drive;
    float mix;
    int mode;
    float softClipPoly(float x);
    float hardClip(float x);
    float softClip(float x, float drive);
};