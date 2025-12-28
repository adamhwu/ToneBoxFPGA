#pragma once

#include "effect.h"
#include "delay.h"
#include "reverb.h"
#include "distortion.h"
#include "PortAudioWrapper.h"


struct AudioEngine {
    Delay delay;
    Distortion distortion;
    Reverb reverb;
    std::vector<Effect*> effects;

    AudioEngine()
        : delay(SAMPLE_RATE/4, 0.08f, 0.1f),
          distortion(3.0f, 0.9f, 2),
          reverb(SAMPLE_RATE, 0.85f, 1.5f),
          effects{ &delay, &reverb } {}
};

int audioCallback(
    const void* inputBuffer,
    void* outputBuffer,
    unsigned long framesPerBuffer,
    const PaStreamCallbackTimeInfo*,
    PaStreamCallbackFlags,
    void* userData
);