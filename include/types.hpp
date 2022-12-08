#pragma once

#include <cinttypes>
#include <chrono>
#include <filesystem>

using FileId = uint64_t;
using Path = std::filesystem::path;
using Hash = std::size_t;
using Timestamp = std::chrono::steady_clock::time_point;
