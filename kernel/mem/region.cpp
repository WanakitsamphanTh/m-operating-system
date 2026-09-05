#include "kernel/mem/region.hpp"

namespace MK {
    MemRegion& Regions::operator[](size_t ind) { return this->regions[ind]; }
}