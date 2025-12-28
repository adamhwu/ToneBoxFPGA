#include <vector>
#include "reverb.h"

Comb::Comb(int delaySamples, float feedback)
    : buffer(delaySamples, 0.0f),
        feedback(feedback),
        index(0) {}

float Comb::process(float x) {
    float y = buffer[index];
    buffer[index] = x + y * feedback;
    index = (index + 1) % buffer.size();
    return y;
};

Allpass::Allpass(int delaySamples, float gain)
    : buffer(delaySamples, 0.0f),
        gain(gain),
        index(0) {}

float Allpass::process(float x) {
    float y = buffer[index];
    float out = -x + y;
    buffer[index] = x + y * gain;
    index = (index + 1) % buffer.size();
    return out;
}

Reverb::Reverb(int sampleRate, float feedback, float decay)
    : comb1(sampleRate * 0.029f * decay, feedback),
        comb2(sampleRate * 0.037f * decay, feedback),
        comb3(sampleRate * 0.041f * decay, feedback),
        mix(0.3f) {}

float Reverb::process(float input) {
    float wet =
        comb1.process(input) +
        comb2.process(input) +
        comb3.process(input);

    wet *= 0.33f; // normalize
    return mix * wet + (1.0f - mix) * input;
}
