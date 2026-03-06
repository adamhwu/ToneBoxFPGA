#include <iostream>
#include <stdint.h>
#include <fstream>
#include <iostream>
#include <filesystem>

namespace fs = std::filesystem;

struct RIFFHeader {
    char riff[4];        // "RIFF"
    uint32_t chunkSize;
    char wave[4];        // "WAVE"
};

struct ChunkHeader {
    char id[4];
    uint32_t size;
};

struct WAVHeader {
    char riff[4] = {'R','I','F','F'};
    uint32_t chunkSize;
    char wave[4] = {'W','A','V','E'};

    char fmt[4] = {'f','m','t',' '};
    uint32_t fmtSize = 16;
    uint16_t audioFormat = 1;
    uint16_t numChannels;
    uint32_t sampleRate;
    uint32_t byteRate;
    uint16_t blockAlign;
    uint16_t bitsPerSample;

    char data[4] = {'d','a','t','a'};
    uint32_t dataSize;
};

int readWAV(const fs::path& filename, WAVHeader& header, std::vector<int16_t>& out) {
    std::ifstream file(filename, std::ios::binary);
    if (!file) {
        std::cerr << "Failed to open file\n";
        return 1;
    }

    RIFFHeader riff;
    file.read(reinterpret_cast<char*>(&riff), sizeof(riff));

    header.chunkSize = riff.chunkSize;
    std::streampos dataPos;

    // ---- Scan chunks ----
    while (file && !file.eof()) {
        ChunkHeader chunk;
        file.read(reinterpret_cast<char*>(&chunk), sizeof(chunk));

        if (std::strncmp(chunk.id, "fmt ", 4) == 0) {
            file.read(reinterpret_cast<char*>(&header.audioFormat), sizeof(header.audioFormat));
            file.read(reinterpret_cast<char*>(&header.numChannels), sizeof(header.numChannels));
            file.read(reinterpret_cast<char*>(&header.sampleRate), sizeof(header.sampleRate));

            file.read(reinterpret_cast<char*>(&header.byteRate), sizeof(header.byteRate));
            file.read(reinterpret_cast<char*>(&header.blockAlign), sizeof(header.blockAlign));
            file.read(reinterpret_cast<char*>(&header.bitsPerSample), sizeof(header.bitsPerSample));

            // Skip any remaining fmt bytes
            file.seekg(chunk.size - 16, std::ios::cur);

            if (header.audioFormat != 1) {
                std::cerr << "Only PCM supported\n";
                return 1;
            }
        }
        else if (std::strncmp(chunk.id, "data", 4) == 0) {
            header.dataSize = chunk.size;
            dataPos = file.tellg();
            break;
        }
        else {
            // Skip unknown chunk
            file.seekg(chunk.size, std::ios::cur);
        }
    }

    if (header.dataSize == 0) {
        std::cerr << "No data chunk found\n";
        return 1;
    }

    // ---- Read samples ----
    file.seekg(dataPos);

    int bytesPerSample = header.bitsPerSample / 8;
    int numSamples = header.dataSize / bytesPerSample;

    out.resize(numSamples);
    if (!file.read(reinterpret_cast<char*>(out.data()), header.dataSize)) {
        std::cerr << "Failed to read audio data\n";
        return 1;
    }

    return 0;
}

#include <vector>
#include <cmath>
#include <stdexcept>

float normalizedRMS(const std::vector<float>& ref,
                     const std::vector<float>& model)
{
    if (ref.size() != model.size())
        throw std::runtime_error("Vector size mismatch");

    float errorSumSq = 0.0;
    float refSumSq   = 0.0;

    const size_t N = ref.size();

    for (size_t i = 0; i < N; ++i)
    {
        float e = ref[i] - model[i];
        errorSumSq += e * e;
        refSumSq   += ref[i] * ref[i];
    }

    if (refSumSq == 0.0)
        return 0.0;  // avoid divide by zero

    return std::sqrt(errorSumSq / refSumSq);
}

int main(void) {
    WAVHeader wavHeader;
    std::vector<int16_t> reference, modeled, error;
    std::vector<float> ref, model;
    readWAV("dirty3.wav", wavHeader, reference);
    readWAV("dirtyHW3.wav", wavHeader, modeled);


    if (reference.size() != modeled.size()) {
        std::cout << "different sized samples" << std::endl;
        return -1;
    }
    ref.resize(reference.size());
    model.resize(reference.size());

    for (size_t i = 0; i < reference.size(); i++) {
        ref[i] = static_cast<float>(reference[i])/32768.0f;
        model[i] = static_cast<float>(modeled[i])/32768.0f;
    }

    float rms = normalizedRMS(ref, model);

    std::cout << "total samples: " << reference.size() << std::endl;
    std::cout << "rms: " << rms << std::endl;

    return 0;
}