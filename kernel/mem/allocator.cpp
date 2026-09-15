#include "kernel/mem/allocator.hpp"
#include "kernel/mem/page.hpp"
#include "kernel/mem/mem.hpp"
#include "mstd/monadic/maybe.hpp"
#include "kernel_core.hpp"
#include <cstdint>
#include <cstring>

inline uint64_t ceil_div(uint64_t x, uint64_t y){
    return x/y + (((x % y)) ? 1 : 0);
}

namespace MK {
    using mstd::maybe;
    using mstd::some;
    using mstd::nothing;
    using mstd::panic;

    PageAlloc::PageAlloc(){}

    maybe<Page> PageAlloc::alloc_page(){
        for(auto i = 0; i < regions->num; i++)
            if(auto page = (*regions)[i].alloc_page(); page.is_valid()) 
                return page;
        return nothing;   
    }

    maybe<Page> PageAlloc::alloc_page(uintptr_t addr){
        addr = align_down<page_size>(addr);
        for(auto i = 0; i < regions->num; i++)
            if(auto page = (*regions)[i].alloc_page(addr); page.is_valid()) 
                return page;
        return nothing;   
    }

    void PageAlloc::reserve_page_at(uintptr_t addr){
        addr = align_down<page_size>(addr);
        for(auto i = 0; i < regions->num; i++)
            if((*regions)[i].reserve_page_at(addr)) 
                break;
    }

    void PageAlloc::free_page(Page page){
        for(size_t i = 0; i < regions->num; i++)
            if(auto region = (*regions)[i]; region.include(page))
                region.free_page(page);
    }

    void PageAlloc::relink(Regions& reg){
        this->regions = &reg;
    }
}