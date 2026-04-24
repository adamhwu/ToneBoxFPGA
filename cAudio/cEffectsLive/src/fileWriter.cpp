#include <iostream>
#include <string>
#include <cstdio>
#include "fileWriter.h"
#include "ringBuffer.cpp"

FileWriter::FileWriter(RingBuffer& cleanbuf, RingBuffer& dirtybuf)
    : recording_(false), running_(true), cleanBuffer(cleanbuf), dirtyBuffer(dirtybuf) {}

int FileWriter::startThread() {
    writer_ = std::thread(&FileWriter::writerThread, this);
    return 0;
}

void FileWriter::writerThread() {
    static int fileIndex = 0;
    SNDFILE* cleanFile = nullptr;
    SNDFILE* dirtyFile = nullptr;

    float dirtyBuf[512];
    float cleanBuf[512];
    while (this->running_.load()) {
        // If not recording, close file and wait 
        if (!recording_.load()) {
            if (cleanFile) {
                sf_close(cleanFile);
                sf_close(dirtyFile);
                cleanFile = nullptr;
                dirtyFile = nullptr;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            continue;
        } 

        // if recording and file not opened, open new file
        else if (!cleanFile) {
            cleanFile = openNewWavFile("clean", fileIndex);
            dirtyFile = openNewWavFile("dirty", fileIndex);
            fileIndex++;
            cleanBuffer.flush();
            dirtyBuffer.flush();
            if (!cleanFile) continue;
            if (!dirtyFile) continue;
        }

        // Consume audio
        int cleanFramesCount = ring_read(cleanBuf, 512, true);
        int dirtyFramesCount = ring_read(dirtyBuf, 512, false);
        if (cleanFramesCount > 0) {
            sf_write_float(cleanFile, cleanBuf, cleanFramesCount);
            sf_write_float(dirtyFile, dirtyBuf, dirtyFramesCount);
        } else {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    }
    // Cleanup
    if (cleanFile) sf_close(cleanFile);
    if (dirtyFile) sf_close(dirtyFile);
}

SNDFILE* FileWriter::openNewWavFile(const std::string& name, const int fileindex) {
    std::string filename = name + "_" + std::to_string(fileindex) + ".wav";

    SF_INFO sfinfo{};
    sfinfo.channels = 1;                   // mono, change if needed
    sfinfo.samplerate = 44100;             // same as your audio stream
    sfinfo.format = SF_FORMAT_WAV | SF_FORMAT_PCM_16;

    SNDFILE* file = sf_open(filename.c_str(), SFM_WRITE, &sfinfo);
    if (!file) {
        printf("Error opening file '%s': %s\n", filename.c_str(), sf_strerror(nullptr));
    }
    return file;
}

int FileWriter::ring_read(float* output, int maxCount, bool clean) {
    int read = 0;
    RingBuffer& buf = (clean) ? cleanBuffer : dirtyBuffer;

    while (read < maxCount && buf.readIndex != buf.writeIndex) {
        buf.read(&output[read]);
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
