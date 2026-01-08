#pragma once

#include <portaudio.h>
#include <vector>

#include "effect.h"

constexpr int SAMPLE_RATE = 44100;
constexpr int FRAMES_PER_BUFFER = 64;

void showDevices();
int initPortAudio(PaStreamCallback audioCallback, PaStream* stream, std::vector<Effect*>* effects);
int stopPortAudio(PaStream* stream);