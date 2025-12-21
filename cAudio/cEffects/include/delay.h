#pragma once
#include <vector>

class Delay {
public:
    Delay(int delaySamples, float feedback, float wet = 0.5f);

    // Process a single sample
    int16_t process(float input);

    // Reset internal buffer
    void reset();

private:
    std::vector<float> buffer;
    float feedback;
    float mix;
    int writeIndex;
};