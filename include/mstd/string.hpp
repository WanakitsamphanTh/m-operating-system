#pragma once
#include <cstddef>

namespace mstd {
    extern "C" size_t strlen(const char*);
    extern "C" int memcmp(const void*, const void*, size_t);
    extern "C" void* memset(void*, int, size_t);
    extern "C" const char* strchr(const char*, char);
}