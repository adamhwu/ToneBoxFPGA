#include "delay.h"

// Constructor
Delay::Delay(int delaySamples, float feedback, float mix)
    : buffer(delaySamples, 0.0f), feedback(feedback), mix(mix), writeIndex(0) {}


float Delay::process(float input) {
    float delayed = buffer[writeIndex];
    float output = (1-mix) * input + mix * delayed;

    buffer[writeIndex] = input + delayed * feedback;
    writeIndex = (writeIndex + 1) % buffer.size();
    return tanh(output);
}


// Reset function
void Delay::reset() {
    std::fill(buffer.begin(), buffer.end(), 0.0f);
    writeIndex = 0;
}