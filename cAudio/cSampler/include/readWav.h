#pragma once
#include <filesystem>

struct WAVHeader;
namespace fs = std::filesystem;

int readWAV(const fs::path& filename, std::vector<int16_t>& out);