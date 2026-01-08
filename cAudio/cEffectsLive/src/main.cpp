#include <portaudio.h>
#include <iostream>

#include "PortAudioWrapper.h"
#include "delay.h"
#include "distortion.h"
#include "reverb.h"
#include "effect.h"
#include "audioCallback.h"

int main() {
    PaStream* stream = nullptr;
    Delay delay(SAMPLE_RATE/4, 0.08f, 0.1f);
    Distortion distortion(9.0f, 0.5f, "scream");
    Reverb reverb(SAMPLE_RATE, 0.85f, 1.5f);
    std::vector<Effect*> effects{ &distortion, &delay, &reverb };

    int err = initPortAudio(audioCallback, stream, &effects);

    if (err != paNoError) {
        std::cerr << "Failed to initialize PortAudio\n";
        return -1;
    }

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

        std::cout << "distortion: " << (distortion.enabled.load() ? "on" : "off") << ", "
                  << "delay: " << (delay.enabled.load() ? "on" : "off") << ", "
                  << "reverb: " << (reverb.enabled.load() ? "on" : "off") << "\n";
    }

    stopPortAudio(stream);

    return 0;
}