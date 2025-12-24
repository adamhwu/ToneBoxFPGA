#include <cmath>
#include "distortion.h"

Distortion::Distortion(float drive, float mix, int mode)
    : drive(drive), mix(mix), mode(mode) {}

float Distortion::process(float input) {
    float driven = input * drive;
    float distorted;

    if (mode == 1) distorted = hardClip(driven);
    else if (mode == 2) distorted = softClip(driven, drive);
    else distorted = softClipPoly(driven);

    return mix * distorted + (1.0f - mix) * input;
}


float Distortion::softClipPoly(float x) {
    if (x > 1.0f)  return 1.0f;
    if (x < -1.0f) return -1.0f;
    return x - (x*x*x)/3.0f;
}

float Distortion::hardClip(float x) {
    if (x > 0.5f)  return 0.5f;
    if (x < -0.5f) return -0.5f;
    return x;
}

float Distortion::softClip(float x, float drive) {
    return std::tanh(drive * x);
}