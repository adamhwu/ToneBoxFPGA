#pragma once

struct RingBuffer;


class FileWriter {
public:
    FileWriter(RingBuffer& rb);
    int startThread();
    bool isRecording() const { return recording_.load(); }
    int setRecording(bool rec);
    int stop();
private:
    SNDFILE* openNewWavFile();
    int ring_read(float* output, int maxCount);
    void writerThread();
    std::atomic<bool> recording_;
    std::atomic<bool> running_;
    std::thread writer_;
    RingBuffer& ringBuffer;
};