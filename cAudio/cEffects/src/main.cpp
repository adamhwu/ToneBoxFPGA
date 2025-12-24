#include <iostream>
#include <fstream>
#include <vector>
#include <cstdint>
#include <cstring>

#include "delay.h"
#include "distortion.h"
#include "reverb.h"
#include "headers.h"

int main() {
    std::ifstream file("resources/input.wav", std::ios::binary);
    if (!file) {
        std::cerr << "Failed to open file\n";
        return 1;
    }

    // ---- Read RIFF header ----
    RIFFHeader riff;
    file.read(reinterpret_cast<char*>(&riff), sizeof(riff));

    if (std::strncmp(riff.riff, "RIFF", 4) != 0 ||
        std::strncmp(riff.wave, "WAVE", 4) != 0) {
        std::cerr << "Not a WAV file\n";
        return 1;
    }
    WAVHeader wavHeader;
    wavHeader.chunkSize = riff.chunkSize;
    std::streampos dataPos;

    // ---- Scan chunks ----
    while (file && !file.eof()) {
        ChunkHeader chunk;
        file.read(reinterpret_cast<char*>(&chunk), sizeof(chunk));

        if (std::strncmp(chunk.id, "fmt ", 4) == 0) {
            file.read(reinterpret_cast<char*>(&wavHeader.audioFormat), sizeof(wavHeader.audioFormat));
            file.read(reinterpret_cast<char*>(&wavHeader.numChannels), sizeof(wavHeader.numChannels));
            file.read(reinterpret_cast<char*>(&wavHeader.sampleRate), sizeof(wavHeader.sampleRate));

            file.read(reinterpret_cast<char*>(&wavHeader.byteRate), sizeof(wavHeader.byteRate));
            file.read(reinterpret_cast<char*>(&wavHeader.blockAlign), sizeof(wavHeader.blockAlign));
            file.read(reinterpret_cast<char*>(&wavHeader.bitsPerSample), sizeof(wavHeader.bitsPerSample));

            // Skip any remaining fmt bytes
            file.seekg(chunk.size - 16, std::ios::cur);

            if (wavHeader.audioFormat != 1) {
                std::cerr << "Only PCM supported\n";
                return 1;
            }
        }
        else if (std::strncmp(chunk.id, "data", 4) == 0) {
            wavHeader.dataSize = chunk.size;
            dataPos = file.tellg();
            break;
        }
        else {
            // Skip unknown chunk
            file.seekg(chunk.size, std::ios::cur);
        }
    }

    if (wavHeader.dataSize == 0) {
        std::cerr << "No data chunk found\n";
        return 1;
    }

    // ---- Read samples ----
    file.seekg(dataPos);

    int bytesPerSample = wavHeader.bitsPerSample / 8;
    int numSamples = wavHeader.dataSize / bytesPerSample;

    std::vector<int16_t> samples(numSamples);
    file.read(reinterpret_cast<char*>(samples.data()), wavHeader.dataSize);
    std::vector<float> delayedSamples(numSamples);
    std::vector<int16_t> delayedSamplesInt(numSamples);
    for (int i = 0; i < numSamples; ++i) {
        delayedSamples[i] = static_cast<float>(samples[i]) / 32768.0f;
    }

    Delay delay(wavHeader.sampleRate, 0.08f, 0.3f);
    Distortion distortion(3.0f, 0.9f, 2);
    Reverb reverb(wavHeader.sampleRate, 0.85f, 1.5f);

    // effects chain
    std::vector<Effect*> effects = { &distortion, &delay };

    // iterate through, applying each effect to the sample
    for (int i = 0; i < numSamples; ++i) {
        for (auto& effect : effects) {
            delayedSamples[i] = effect->process(delayedSamples[i]);
        }
        delayedSamplesInt[i] = static_cast<int16_t>(delayedSamples[i] * 32768.0f);
    }

    std::ofstream MyFile("resources/output.wav", std::ios::binary);
    // Check if the file was successfully opened
    if (MyFile.is_open()) {
        MyFile.write(reinterpret_cast<char*>(&wavHeader), sizeof(wavHeader));
        MyFile.write(reinterpret_cast<char*>(delayedSamplesInt.data()), delayedSamplesInt.size() * sizeof(int16_t));
        MyFile.close();
    } else {
        std::cerr << "Error creating file!" << std::endl;
    }
    return 0;
}