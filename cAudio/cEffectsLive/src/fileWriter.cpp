#include <sndfile.h>
#include <string>
#include <cstdio>
#include <atomic>
#include <thread>
#include "fileWriter.h"
#include "ringBuffer.cpp"

FileWriter::FileWriter(RingBuffer& rb)
    : recording_(false), running_(true), ringBuffer(rb) {}

int FileWriter::startThread() {
    writer_ = std::thread(&FileWriter::writerThread, this);
    return 0;
}

void FileWriter::writerThread() {
    SNDFILE* file = nullptr;
    float temp[512];
    while (this->running_.load()) {
        // If not recording, close file and wait 
        if (!recording_.load()) {
            if (file) {
                sf_close(file);
                file = nullptr;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            continue;
        } 
        // if recording and file not opened, open new file
        else if (!file) {
            file = openNewWavFile();
            ringBuffer.flush();
            if (!file) continue;
        }

        // Consume audio
        int frames = ring_read(temp, 512);
        if (frames > 0) {
            sf_write_float(file, temp, frames);
        } else {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    }
    // Cleanup
    if (file)
        sf_close(file);
}

SNDFILE* FileWriter::openNewWavFile() {
    static int fileIndex = 0;
    char filename[64];
    snprintf(filename, sizeof(filename), "recording_%03d.wav", fileIndex++);

    SF_INFO sfinfo{};
    sfinfo.channels = 1;                   // mono, change if needed
    sfinfo.samplerate = 44100;             // same as your audio stream
    sfinfo.format = SF_FORMAT_WAV | SF_FORMAT_PCM_16;

    SNDFILE* file = sf_open(filename, SFM_WRITE, &sfinfo);
    if (!file) {
        printf("Error opening file '%s': %s\n", filename, sf_strerror(nullptr));
    }
    return file;
}

int FileWriter::ring_read(float* output, int maxCount) {
    int read = 0;
    while (read < maxCount && ringBuffer.readIndex != ringBuffer.writeIndex) {
        ringBuffer.read(&output[read]);
        read++;
    }
    return read;  // number of samples actually read
}
int FileWriter::setRecording(bool rec) {
    recording_.store(rec);
    return 0;
}

int FileWriter::stop() {
    running_.store(false);
    if (writer_.joinable()) {
        writer_.join();
    }
    return 0;
}
