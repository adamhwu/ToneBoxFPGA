#include <portaudio.h>
#include <iostream>
#include <cassert>
#include "audioCallback.h"
#include "effect.h"
#include "ringBuffer.cpp"

int audioCallback(
    const void* inputBuffer,
    void* outputBuffer,
    unsigned long framesPerBuffer,
    const PaStreamCallbackTimeInfo*,
    PaStreamCallbackFlags,
    void* userData
) {
    auto* params = static_cast<paParameters*>(userData);
    auto* effects = params->effects;
    auto* cleanBuf = params->cleanBuffer;
    auto* dirtyBuf = params->dirtyBuffer;

    const float* in = static_cast<const float*>(inputBuffer);
    float* out = static_cast<float*>(outputBuffer);

    for (unsigned long i = 0; i < framesPerBuffer; ++i) {
        float x = in ? in[i] : 0.0f;
        float x_clean = x;
        for (auto* effect : *effects){
            if (effect->enabled.load() == true){
                assert (effect != nullptr);
                x = effect->process(x);
            }
        }

        out[2*i] = x;
        out[2*i+1] = x;
        cleanBuf->write(x_clean);
        dirtyBuf->write(x);
    }
    return paContinue;
}