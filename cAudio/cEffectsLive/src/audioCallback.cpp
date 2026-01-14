#include <portaudio.h>
#include <vector>
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
    auto* ringBuffer = params->ringBuffer;

    const float* in = static_cast<const float*>(inputBuffer);
    float* out = static_cast<float*>(outputBuffer);

    for (unsigned long i = 0; i < framesPerBuffer; ++i) {
        float x = in ? in[i] : 0.0f;
        for (auto* effect : *effects){
            if (effect->enabled.load() == true){
                assert (effect != nullptr);
                x = effect->process(x);
            }
        }

        out[2*i] = x;
        out[2*i+1] = x;
        ringBuffer->write(x);
    }
    return paContinue;
}