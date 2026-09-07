#include "kernel/mem/allocator.hpp"
#include "kernel/mem/page.hpp"
#include "mstd/monadic/maybe.hpp"
#include "kernel_core.hpp"
#include <cstdint>
#include <cstring>

inline uint64_t ceil_div(uint64_t x, uint64_t y){
    return x/y + ((x % y)) ? 1 : 0;
}

namespace MK {
    using mstd::maybe;
    using mstd::some;
    using mstd::nothing;
    using mstd::panic;

    Allocator::Allocator(): bitmap(nullptr), ram_start(nullptr), bitmap_size(0){}

    void Allocator::init(uintptr_t kernel_start, uintptr_t kernel_size, Regions& regions){
        auto kernel_end = kernel_start + kernel_size;
        uintptr_t ram_start, ram_end;
        for(auto i = 0; i < regions.num; i++){
            auto region_base = regions[i].base;
            ram_start = ram_start < region_base ? ram_start : region_base;
            auto region_end = regions[i].base + regions[i].size;
            ram_end = ram_end < region_end ? region_end : ram_end;
        }
        this->ram_start = reinterpret_cast<uint8_t*>(ram_start);

        // calculate bitmap size
        uint64_t ram_size = ram_end - ram_start;
        this->page_count = ram_size / page_size;
        this->bitmap_size = ceil_div(this->page_count, 8);
        
        // find the enough space for bitmap
        auto bitmap_addr = regions.find_contiguous(this->bitmap_size, kernel_end).take("cannot allocate the bitmap");
        auto bitmap_end = bitmap_addr + this->bitmap_size;
        this->bitmap = reinterpret_cast<uint8_t*>(bitmap_addr);

        // place bitmap and mask everything as reserved
        memset(this->bitmap, 0xff, this->bitmap_size);

        // remask region space as free
        for(auto i = 0; i < regions.num; i++){
            for(size_t off = 0; off < regions[i].size; off++){
                uintptr_t page = regions[i].base + off;
                if((page >= kernel_start && page < kernel_end) 
                    || page >= reinterpret_cast<uintptr_t>(this->bitmap) 
                    && page < reinterpret_cast<uintptr_t>(bitmap_end)) 
                        continue;
                size_t page_ind = page / 4096;
                size_t byte_ind = page_ind / 8;
                size_t bit_ind = page_ind % 8;
                this->bitmap[byte_ind] &= ~(1 << bit_ind);
            }
        }
    }

    maybe<Page> Allocator::alloc_page(){
        for(size_t ind = 0; ind < bitmap_size; ind++){
            uint8_t& byte = bitmap[ind];
            for(int i = 0; i < 8; i++){
                if((byte >> i) & 0x01) continue;
                size_t page_ind = ind * 8 + i;
                if(page_ind >= page_count)
                    return nothing;
                uint8_t mask = 1 << i;
                byte |= mask;
                Page page = reinterpret_cast<Page>(ram_start + (page_ind) * page_size);
                return some<Page>(page);
            }
        }
        return nothing;   
    }

    void Allocator::free_page(Page page){
        if(page == 0) return;
        uintptr_t ind = (page - reinterpret_cast<uintptr_t>(this->ram_start)) / page_size;
        uintptr_t byte_ind = ind / 8;
        uintptr_t bit_ind = ind % 8;
        bitmap[byte_ind] &= ~(1 << bit_ind);
    }
}