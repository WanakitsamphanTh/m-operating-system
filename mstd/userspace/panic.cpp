#include "kernel_core.hpp"

namespace mstd {
    extern "C" [[noreturn]] void panic(const char* msg) {
        kernel_panic();
    }
};