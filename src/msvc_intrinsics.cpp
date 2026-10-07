#include <cstring>
// Clang 18 does not lower MinHook's MSVC __movsb intrinsic on this target.
extern "C" void __movsb(unsigned char* destination, const unsigned char* source, size_t size) {
    std::memcpy(destination,source,size);
}
