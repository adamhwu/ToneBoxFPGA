#include <fstream>
#include <iostream>

#include "readWav.h"
#include "headers.h"

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
