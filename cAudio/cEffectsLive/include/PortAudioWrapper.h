#pragma once

#include <portaudio.h>
#include <vector>

#include "effect.h"

constexpr int SAMPLE_RATE = 44100;
constexpr int FRAMES_PER_BUFFER = 64;
struct AudioEngine;

void showDevices();
int initPortAudio(PaStreamCallback audioCallback, PaStream* stream, AudioEngine* gEngine);
int stopPortAudio(PaStream* stream);