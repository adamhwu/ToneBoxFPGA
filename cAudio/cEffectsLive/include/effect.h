#pragma once

class Effect {
public:
    virtual ~Effect() = default;
    virtual float process(float input) = 0;
}; 