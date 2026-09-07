#pragma once

#include <cstdint>
#include "kernel/mem/page.hpp"
#include "kernel/mem/region.hpp"
#include "mstd/monadic/maybe.hpp"

namespace MK {
    using mstd::maybe;

    class Allocator {
        constexpr static size_t page_size = 4096;
        size_t page_count;
        uint8_t* bitmap;
        size_t bitmap_size;
        uint8_t* ram_start;
    public:
        Allocator();
        void init(uintptr_t kernel_start, uintptr_t kernel_size, Regions& regions);
        maybe<Page> alloc_page();
        void free_page(Page);
    };
}