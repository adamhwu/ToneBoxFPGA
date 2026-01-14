#include <stdio.h>
#include <stdlib.h>
#include <portaudio.h>
#include <sndfile.h>  // For writing WAV files

#define NUM_SAMPLES  (44100 * 5) // 5 seconds of audio at 44100 Hz  

struct RingBuffer {
    float buffer[NUM_SAMPLES];
    int writeIndex = 0;
    int readIndex = 0;

    int write(float sample) {
        buffer[writeIndex] = sample;
        writeIndex = (writeIndex + 1) % NUM_SAMPLES;
        return writeIndex;
    }

    int read(float* outSample) {
        *outSample = buffer[readIndex];
        readIndex = (readIndex + 1) % NUM_SAMPLES;
        return readIndex;
    }

    void flush() {
        readIndex = writeIndex;
    }
};