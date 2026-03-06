#include <cmath>
#include "distortion.h"

Distortion::Distortion(float drive, float mix, std::string mode)
    : drive(drive), mix(mix), mode(mode) {}

float Distortion::process(float input) {
    float distorted;
    float driven = input * drive/2;
    distorted = screamer(driven);
    driven = distorted * drive / 2;
    distorted = softClip(driven);
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

float Distortion::softClip(float x) {
    return std::tanh(x);
}

float Distortion::screamer(float x) {
    return (2.0f / M_PI) * std::atan(x);
}