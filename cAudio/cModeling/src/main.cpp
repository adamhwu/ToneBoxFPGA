#include <iostream>
#include <vector>

#include "headers.h"
#include "layers.h"
#include "readWav.h"

int main() {
    WAVHeader wavHeader;
    std::vector<int16_t> samples;
    std::vector<double> floatSamples; // try fixed point representation
    // read .wav file into samples buffer
    readWAV("resources/clean_0.wav", wavHeader, samples);
    int numSamples = samples.size();
    std::cout << numSamples << std::endl;

    if (DEBUG) {
        // output integers as binary
        std::ofstream header("samples.h");
        header << "int16_t input_data[" << numSamples << "] = {\n";
        for (int i = 0; i < numSamples; i++) {
            header << samples[i];
            if (i != numSamples - 1)
                header << ",";
            header << "\n";
        }
        header << "};\n";
        header.close();
    }

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

    // create layer objects
    PiecewiseLinear* inl = new PiecewiseLinear("weights/inl_coeffsTSL.csv");
    PiecewiseLinear* onl = new PiecewiseLinear("weights/onl_coeffsTSL.csv");
    LinearModel* lm = new LinearModel("weights/lm_coeffs_TSL.csv");

    // iterate through, applying each layer to the sample
    double temp;
    for (int i = 0; i < numSamples; i++) {
        temp = floatSamples[i];
        temp = inl->process(temp);
        temp = lm->process(temp);
        temp = onl->process(temp);
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