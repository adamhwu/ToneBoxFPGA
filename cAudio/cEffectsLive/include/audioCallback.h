#pragma once

class Effect;
struct RingBuffer;

struct paParameters {
    std::vector<Effect*>* effects;
    RingBuffer* cleanBuffer;
    RingBuffer* dirtyBuffer;

    paParameters(std::vector<Effect*>* eff, RingBuffer* cleanBuf, RingBuffer* dirtyBuf)
        : effects(eff), cleanBuffer(cleanBuf), dirtyBuffer(dirtyBuf) {}
};

int audioCallback(
    const void* inputBuffer,
    void* outputBuffer,
    unsigned long framesPerBuffer,
    const PaStreamCallbackTimeInfo*,
    PaStreamCallbackFlags,
    void* userData
);