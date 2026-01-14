#pragma once

#include <portaudio.h>

struct paParameters;

constexpr int SAMPLE_RATE = 44100;
constexpr int FRAMES_PER_BUFFER = 64;

void showDevices();
int initPortAudio(PaStreamCallback audioCallback, PaStream* stream, paParameters* params);
int stopPortAudio(PaStream* stream);