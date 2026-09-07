#include "kernel/mem/region.hpp"

namespace MK {
    using mstd::maybe;
    using mstd::nothing;
    MemRegion& Regions::operator[](size_t ind) { return this->regions[ind]; }
    maybe<uintptr_t> Regions::find_contiguous(size_t size, uintptr_t hint){
        return nothing;
    }
}