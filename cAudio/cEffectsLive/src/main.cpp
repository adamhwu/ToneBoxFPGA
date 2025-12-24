#include <portaudio.h>
#include <iostream>

#include "delay.h"
#include "reverb.h"
#include "distortion.h"

constexpr int SAMPLE_RATE = 96000;
constexpr int FRAMES_PER_BUFFER = 64;


Delay delay(SAMPLE_RATE, 0.08f, 0.3f);
Distortion distortion(3.0f, 0.9f, 2);
Reverb reverb(SAMPLE_RATE, 0.85f, 1.5f);

// effects chain
std::vector<Effect*> effects = { &distortion , &delay, &reverb };


static int audioCallback(
    const void* inputBuffer,
    void* outputBuffer,
    unsigned long framesPerBuffer,
    const PaStreamCallbackTimeInfo*,
    PaStreamCallbackFlags,
    void*
) {
    const float* in  = (const float*)inputBuffer;
    float* out = (float*)outputBuffer;

    for (unsigned int i = 0; i < framesPerBuffer; ++i) {
        float x = in ? in[i] : 0.0f;
        // iterate through, applying each effect to the sample
        float y = x;
        for (auto& effect : effects) {
            y = effect->process(y);
        }
        out[i] = y;
    }
    return paContinue;
}

int main() {
    Pa_Initialize();

    PaStream* stream;
    Pa_OpenDefaultStream(
        &stream,
        1,                // input channels
        1,                // output channels
        paFloat32,        // sample format
        SAMPLE_RATE,
        FRAMES_PER_BUFFER,
        audioCallback,
        nullptr
    );

    Pa_StartStream(stream);

    std::cout << "Running... press Enter to quit\n";
    std::cin.get();

    Pa_StopStream(stream);
    Pa_CloseStream(stream);
    Pa_Terminate();
}
