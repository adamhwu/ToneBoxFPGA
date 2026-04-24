#include </opt/homebrew/include/portaudio.h>
#include <iostream>

#include "paWrapper.h"
#include "delay.h"
#include "distortion.h"
#include "reverb.h"
#include "effect.h"
#include "audioCallback.h"
#include "ringBuffer.cpp"
#include "fileWriter.h"


int main() {
    // initialize effects
    Delay delay(SAMPLE_RATE/4, 0.08f, 0.1f);
    Distortion distortion(9.0f, 0.5f, "scream");
    Reverb reverb(SAMPLE_RATE, 0.85f, 1.5f);
    std::vector<Effect*> effects{ &distortion, &delay, &reverb };

    RingBuffer cleanBuf;
    RingBuffer dirtyBuf;
    std::vector<float> looperBuffer;
    looperBuffer.resize(44100*30, 1.0f); // 30 seconds of audio at 44100 Hz
    std::atomic<bool> recording_;
    std::atomic<bool> paused_;
    paused_.store(true);
    FileWriter fileWriter(cleanBuf, dirtyBuf);
    PaStream* stream = nullptr;
    paParameters paParams(&effects, &cleanBuf, &dirtyBuf, &recording_, &paused_, &looperBuffer);

    int err = initPortAudio(audioCallback, stream, &paParams);
    if (err != paNoError) {
        std::cerr << "Failed to initialize PortAudio\n";
        return -1;
    }
    fileWriter.startThread();

    // CLI loop
    char c;
    while (true) {
        // Keep running until 'q' is pressed
        std::cin >> c;
        if (c == 'q') break;
        else if (c == 'd') {
            distortion.enabled.store(!distortion.enabled.load());
            std::cout << "Distortion " << (distortion.enabled.load() ? "enabled" : "disabled") << "\n";
        }
        else if (c == 'l') {
            delay.enabled.store(!delay.enabled.load());
            std::cout << "Delay " << (delay.enabled.load() ? "enabled" : "disabled") << "\n";
        }
        else if (c == 'r') {
            reverb.enabled.store(!reverb.enabled.load());
            std::cout << "reverb " << (reverb.enabled.load() ? "enabled" : "disabled") << "\n";
        }
        else if (c == 'k') {
            distortion.drive += 1.0f;
            if (distortion.drive > 20.0f) distortion.drive = 1.0f;
            std::cout << "Distortion drive set to " << distortion.drive << "\n";
        }
        else if (c == 'j') {
            distortion.drive -= 1.0f;
            if (distortion.drive < 1.0f) distortion.drive = 20.0f;
            std::cout << "Distortion drive set to " << distortion.drive << "\n";
        }
        else if (c == 's') {
            recording_.store(!recording_.load());
            std::cout << "recording " << (recording_ ? "started" : "stopped") << "\n";
        }
        else if (c == 'p') {
            paused_.store(!paused_);
            std::cout << "Looping " << (!paused_ ? "started" : "stopped") << "\n";
        }

        std::cout << "distortion: " << (distortion.enabled.load() ? "on" : "off") << ", "
                  << "delay: " << (delay.enabled.load() ? "on" : "off") << ", "
                  << "reverb: " << (reverb.enabled.load() ? "on" : "off") << "\n";
    }
    stopPortAudio(stream);
    fileWriter.stop();
    return 0;
}