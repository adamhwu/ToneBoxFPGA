#pragma once
#include <filesystem>

struct WAVHeader;
namespace fs = std::filesystem;


int readWAV(const fs::path& filename, WAVHeader& header, std::vector<int16_t>& out);