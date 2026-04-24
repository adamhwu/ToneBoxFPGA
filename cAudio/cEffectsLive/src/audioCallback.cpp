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
    static size_t looperIndex = 0;
    static size_t looperSamples = 0;
    static std::vector<float> looperBuffer(44100*60);

    auto* params = static_cast<paParameters*>(userData);
    auto* effects = params->effects;
    auto* cleanBuf = params->cleanBuffer;
    auto* dirtyBuf = params->dirtyBuffer;
    auto* recording = params->recording;
    auto* paused = params->paused;

    // auto* looperBuffer = params->looperData->data();

    // if (!looperBuffer) {
    //     std::cout << "Dangling" << std::endl;
    //     std::cout << params->looperData->size() << std::endl;
    //     return 0;
    // }

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

        cleanBuf->write(x_clean);
        dirtyBuf->write(x);

        if (recording->load()) {
            looperBuffer[looperSamples] = x;
            looperSamples = (looperSamples + 1) % (44100*60);
        }

        if (!paused->load()) {
            x += looperBuffer[looperIndex];
            looperIndex = (looperIndex + 1) % looperSamples;
        }

        out[2*i] = x;
        out[2*i+1] = x;

    }
    return paContinue;
}