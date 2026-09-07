#pragma once
#include <cstddef>
#include <cstdint>
#include <mstd/monadic/maybe.hpp>

namespace MK {
    using mstd::maybe;
    struct MemRegion {
        uintptr_t base;
        size_t size;
    };

    struct Regions {
        MemRegion regions[8];
        size_t num;
        MemRegion& operator[](size_t);
        maybe<uintptr_t> find_contiguous(size_t size, uintptr_t hint);
    };

    extern Regions regions;
}