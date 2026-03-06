#include <iostream>
#include <vector>
#include <fstream>

#include "headers.h"
#include "readWav.h"

int main() {
    WAVHeader wavHeader;
    std::vector<int16_t> samples;
    std::vector<double> floatSamples; // try fixed point representation

    // read .wav file into samples buffer
    readWAV("resources/clean3.wav", wavHeader, samples);
    int numSamples = samples.size();
    std::cout << numSamples << std::endl;

    floatSamples.resize(numSamples);
    double max = 0;

    // convert to normalized float values
    for (int i = 0; i < numSamples; i++) {
        double sample = static_cast<double>(samples[i]) / 32768.0;
        floatSamples[i] = sample;
        max = (abs(sample) > max) ? abs(sample) : max;
    }

    for (int i = 0; i < numSamples; ++i)
        floatSamples[i] /= max;

    // iterate through, applying each layer to the sample
    double temp;
    for (int i = 0; i < numSamples; i++) {
        temp = floatSamples[i];
        floatSamples[i] = temp;
        samples[i] = static_cast<int16_t>(temp * 32768.0f);
    }

    std::ofstream refFile("references/output.csv");
    for (const auto& value : floatSamples)
        refFile << value << "\n";
    refFile.close();

    std::ofstream MyFile("resources/output.wav", std::ios::binary);

    // Check if the file was successfully opened
    if (MyFile.is_open()) {
        MyFile.write(reinterpret_cast<char*>(&wavHeader), sizeof(wavHeader));
        MyFile.write(reinterpret_cast<char*>(samples.data()), samples.size() * sizeof(int16_t));
        MyFile.close();
    } else {
        std::cerr << "Error creating file!" << std::endl;
    }
    return 0;
}