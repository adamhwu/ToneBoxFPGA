#pragma once
#include <atomic>

class Effect {
public:
    std::atomic<bool> enabled{true};
    virtual ~Effect() = default;
    virtual float process(float input) = 0;
}; 