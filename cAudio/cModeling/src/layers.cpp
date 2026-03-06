#include "layers.h"

PiecewiseLinear::PiecewiseLinear(fs::path filename) {
    load(filename);
    std::cout << "NL Loaded" << std::endl;
    if (DEBUG) {
        showVectors();
    }
}

void PiecewiseLinear::load(const fs::path& filename) {
    std::ifstream file(filename);

    if (!file.is_open()) {
        std::cout << filename << std::endl;
        throw std::runtime_error("Could not open PWL CSV file");
    }

    std::string line;
    std::vector<std::vector<float>> rows;
    while (std::getline(file, line))
    {
        std::stringstream ss(line);
        std::string cell;
        std::vector<float> row;

        while (std::getline(ss, cell, ','))
            row.push_back(std::stod(cell));

        rows.push_back(row);
    }

    if (rows.size() != 2)
        throw std::runtime_error("Expected 2 rows in CSV");

    bps = rows[0];
    values = rows[1];

    if (bps.size() != values.size())
        throw std::runtime_error("Breakpoint/value size mismatch");

    file.close();

    slopes.reserve(bps.size()-1);
    
    for (int i = 0; i < bps.size()-1; i++) {
        slopes[i] = values[i+1]-values[i];
        slopes[i] /= bps[i+1]-bps[i];
    }
}

float PiecewiseLinear::process(float in) {
    float out, dx;
    if (in < bps[0]) return values[0];
    else if (in > bps[bps.size()-1]) return values[bps.size()-1];

    for (int i = 0; i < bps.size()-1; i++) {
        if ((in > bps[i] && (in < bps[i+1]))) {
            dx = in-bps[i];
            out = dx * slopes[i] + values[i];
            return out;
        }
    }
    return 0.0;
}

void PiecewiseLinear::showVectors(void) {
    std::cout << "bps: ";
    for (int i = 0; i < bps.size(); i++) {
        std::cout << bps[i] << ", ";
    }
    std::cout << "\nvalues: ";
    for (int i = 0; i < bps.size(); i++) {
        std::cout << values[i] << ", ";
    }
    std::cout << "\nslopes: ";
    for (int i = 0; i < bps.size(); i++) {
        std::cout << slopes[i] << ", ";
    }
    std::cout << std::endl;
}

LinearModel::LinearModel(fs::path filename) {
    load(filename);
    x.reserve(LM_SIZE);
    y.reserve(LM_SIZE);
    for (int i = 0; i < LM_SIZE; i++) {
        x[i] = 0;
        y[i] = 0;
    }

    if (DEBUG) {
        std::cout << "Linear Model Loaded, coeffs: " << std::endl;
        showVectors();
    }
}

void LinearModel::load(const fs::path& filename) {
    std::ifstream file(filename);

    if (!file.is_open()) {
        throw std::runtime_error("Could not open linear model CSV file");
    }

    std::string line;
    std::string token;

    {
        std::getline(file, line);
        std::stringstream ss(line);
        while (std::getline(ss, token, ',')) {
            B.push_back(static_cast<float>(std::stof(token)));
        }
    }

    {
        std::getline(file, line);
        std::stringstream ss(line);
        while (std::getline(ss, token, ',')) {
            F.push_back(static_cast<float>(std::stof(token)));
        }
    }
}

float LinearModel::process(float in)
{
    int i;
    // shift input history
    for(i=LM_SIZE-1;i>0;i--)
        x[i] = x[i-1];
    x[0] = in;

    double out = 0.0;

    for(i=0;i<LM_SIZE;i++)
        out += B[i] * x[i] - F[i] * y[i];

    // shift output history
    for(i=LM_SIZE-1;i>0;i--)
        y[i] = y[i-1];
    y[0] = out;

    return out;
}

void LinearModel::showVectors(void) {
    std::cout << "B: ";
    for (int i = 0; i < B.size(); i++) {
        std::cout << B[i] << ", ";
    }
    std::cout << std::endl <<"F: ";
    for (int i = 0; i < F.size(); i++) {
        std::cout << F[i] << ", ";
    }
    std::cout << std::endl;
    std::cout << F[19] << std::endl;
    std::cout << B[19] << std::endl;
}
