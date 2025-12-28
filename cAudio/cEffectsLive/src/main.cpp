#include <portaudio.h>
#include <iostream>

#include "PortAudioWrapper.h"
#include "audioCallback.h"


Delay delay(SAMPLE_RATE/2, 0.08f, 0.1f);
Distortion distortion(4.0f, 0.9f, 2);
Reverb reverb(SAMPLE_RATE, 0.85f, 1.5f);

int main() {
    PaStream* stream = nullptr;
    AudioEngine gEngine;
    int err = initPortAudio(audioCallback, stream, &gEngine);

    if (err != paNoError) {
        std::cerr << "Failed to initialize PortAudio\n";
        return -1;
    }

    char c = '1';
    bool delayOn = true;
    bool reverbOn = true;
    bool distortionOn = true;

    while (true) {
        // Keep running until 'q' is pressed
        std::cin >> c;
        if (c == 'q') break;
        if (c == 'd') {
            std::cout << "Toggling delay effect\n";
            delayOn = !delayOn;
            if (delayOn) {
                std::cout << "Enabling delay\n";

            } else {
                std::cout << "Disabling delay\n";
            }
        }
    }

    stopPortAudio(stream);

    return 0;
}