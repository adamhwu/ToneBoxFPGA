#pragma once

#include "effect.h"
#include "delay.h"
#include "reverb.h"
#include "distortion.h"
#include "PortAudioWrapper.h"

int audioCallback(
    const void* inputBuffer,
    void* outputBuffer,
    unsigned long framesPerBuffer,
    const PaStreamCallbackTimeInfo*,
    PaStreamCallbackFlags,
    void* userData
);