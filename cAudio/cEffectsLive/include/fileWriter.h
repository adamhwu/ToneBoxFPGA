#pragma once

struct RingBuffer;

#include <sndfile.h>
#include <atomic>
#include <thread>

class FileWriter {
public:
    FileWriter(RingBuffer& cleanbuf, RingBuffer& dirtybuf);
    int startThread();
    bool isRecording() const { return recording_.load(); }
    int setRecording(bool rec);
    int stop();
private:
    SNDFILE* openNewWavFile(const std::string& name, const int fileindex);
    int ring_read(float* output, int maxCount, bool clean);
    void writerThread();
    std::atomic<bool> recording_;
    std::atomic<bool> running_;
    std::thread writer_;
    RingBuffer& cleanBuffer;
    RingBuffer& dirtyBuffer;
};