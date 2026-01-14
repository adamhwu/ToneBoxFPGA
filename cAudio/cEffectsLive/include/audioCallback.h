#pragma once

class Effect;
struct RingBuffer;

struct paParameters {
    std::vector<Effect*>* effects;
    RingBuffer* ringBuffer;

    paParameters(std::vector<Effect*>* eff, RingBuffer* rb)
        : effects(eff), ringBuffer(rb) {}
};

int audioCallback(
    const void* inputBuffer,
    void* outputBuffer,
    unsigned long framesPerBuffer,
    const PaStreamCallbackTimeInfo*,
    PaStreamCallbackFlags,
    void* userData
);