#pragma once
#include <filesystem>
#include <string>
#include "trajectory.hpp"
namespace fc {
void initialize(bool remove_centre);
void shutdown();
bool ready();
void set_facility(void* facility, Limits limits);
void remove_facility(void* facility);
void reset();
std::string fingerprint(const std::filesystem::path& path);
void log(const std::string& message);
}
