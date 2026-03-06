#pragma once

#include <stdint.h>

#pragma pack(push, 1)

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

#pragma pack(pop)