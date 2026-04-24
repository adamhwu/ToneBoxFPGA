#pragma once

#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include "layers.h"

#define LM_SIZE 20
#define NL_BREAKPOINTS 30

namespace fs = std::filesystem;

#define DEBUG 1

class Layer {
    public:
        virtual void load(const fs::path& filename) = 0;
        virtual float process(float in) = 0;
};

class PiecewiseLinear : public Layer {
    public:
        PiecewiseLinear(fs::path filename);
        void load(const fs::path& filename) override;
        float process(float in) override;
        void showVectors(void);
    private:
        std::vector<float> bps;
        std::vector<float> values;
        std::vector<float> slopes;
};

class LinearModel : public Layer {
    public:
        LinearModel(fs::path filename);
        void load(const fs::path& filename) override;
        float process(float in) override;
        void showVectors(void);

    private:
        std::vector<float> B;
        std::vector<float> F;
        std::vector<float> x = {0};
        std::vector<float> y = {0};
};
