#pragma once
#include <cstddef>
#include <cstdint>

namespace MK {
    struct MemRegion {
        uintptr_t base;
        size_t size;
    };

    struct Regions {
        MemRegion regions[8];
        size_t num;
        
        MemRegion& operator[](size_t);
    };

    extern Regions regions;
}