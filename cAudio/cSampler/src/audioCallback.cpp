#include <portaudio.h>
#include <iostream>

#include "audioCallback.h"
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
    auto* cleanBuf = params->cleanBuffer;
    auto* dirtyBuf = params->dirtyBuffer;

    const float* in = static_cast<const float*>(inputBuffer);
    float* out = static_cast<float*>(outputBuffer);

    for (unsigned long i = 0; i < framesPerBuffer; ++i) {
        cleanBuf->write(in[i*4]);
        dirtyBuf->write(in[i*4+2]);
        out[2*i] = in[i*4];
        out[2*i+1] = in[i*4+2];
    }
    return paContinue;
}