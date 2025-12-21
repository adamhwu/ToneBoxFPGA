#include <vector>
#include <iostream>

class Delay {
public:
    Delay(int delaySamples, float feedback, float mix = 0.5f)
        : buffer(delaySamples, 0.0f),
          feedback(feedback),
          mix(mix),
          writeIndex(0) {}

    int16_t process(float input) {
        float delayed = buffer[writeIndex];
        float output = (1-mix) * input + mix * delayed;

        buffer[writeIndex] = input + delayed * feedback;
        writeIndex = (writeIndex + 1) % buffer.size();
        output = tanh(output);
        return static_cast<int16_t>(output * 32768.0f);
    }

private:
    std::vector<float> buffer;
    float feedback;
    float mix;
    int writeIndex;
};