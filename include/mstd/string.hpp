#pragma once
#include <cstddef>

namespace mstd {
    extern "C" size_t strlen(const char*);
    extern "C" int memcmp(const char*, const char*, size_t);
}