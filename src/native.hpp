#pragma once
#include <filesystem>
#include <array>
#include <cstdint>
#include <string>
#include "trajectory.hpp"
namespace fc {
void initialize(bool remove_centre);
void shutdown();
bool ready();
uint64_t load_generation();
void set_facility(void* facility, Limits limits, uint32_t key, void* actor);
std::optional<Limits> limits_for_key(uint32_t key);
struct Probe { Vec origin; bool vanilla, modified; std::optional<Flight> flight; };
Probe probe(void* facility, Vec relative);
Vec catch_point(void* receiver);
std::array<uint64_t,3> flight_stats();
void set_save_root(std::filesystem::path root);
void remove_facility(void* facility);
void reset();
std::string fingerprint(const std::filesystem::path& path);
void log(const std::string& message);
}
