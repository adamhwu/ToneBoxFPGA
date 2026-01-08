#pragma once
#include <vector>
#include "effect.h"

class Delay : public Effect {
public:
    Delay(int delaySamples, float feedback, float mix = 0.5f);

    // Process a single sample
    float process(float input);

    // Reset internal buffer
    void reset();

    std::vector<float> buffer;
    float feedback;
    float mix;
    int writeIndex;
};