#pragma once
#include "effect.h"

class Comb : public Effect {
public:
    Comb(int delaySamples, float feedback);

    // Process a single sample
    float process(float input) override;

private:
    std::vector<float> buffer;
    float feedback;
    int index;
};

class Allpass : public Effect {
public:
    Allpass(int delaySamples, float gain);

    // Process a single sample
    float process(float input) override;

private:
    std::vector<float> buffer;
    float gain;
    int index;
};

class Reverb : public Effect {
public:
    Reverb(int sampleRate, float feedback, float decay);

    // Process a single sample
    float process(float input) override;

private:
Comb comb1, comb2, comb3;
float mix;
};