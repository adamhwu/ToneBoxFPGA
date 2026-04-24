#pragma once

class Effect;
struct RingBuffer;

struct paParameters {
    std::vector<Effect*>* effects;
    RingBuffer* cleanBuffer;
    RingBuffer* dirtyBuffer;

    std::atomic<bool>* recording;
    std::atomic<bool>* paused;

    std::vector<float>* looperData;

    paParameters(std::vector<Effect*>* eff, RingBuffer* cleanBuf, RingBuffer* dirtyBuf, std::atomic<bool>* recording, std::atomic<bool>* paused, std::vector<float>* looperData)
        : effects(eff), cleanBuffer(cleanBuf), dirtyBuffer(dirtyBuf), recording(recording), paused(paused), looperData(looperData) {}
};

int audioCallback(
    const void* inputBuffer,
    void* outputBuffer,
    unsigned long framesPerBuffer,
    const PaStreamCallbackTimeInfo*,
    PaStreamCallbackFlags,
    void* userData
);