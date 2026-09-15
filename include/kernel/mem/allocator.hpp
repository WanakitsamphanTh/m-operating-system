#pragma once

#include <cstdint>
#include "kernel/mem/page.hpp"
#include "kernel/mem/region.hpp"
#include "mstd/monadic/maybe.hpp"
#include "kernel/mem/mem.hpp"

namespace MK {
    using mstd::maybe;

    class PageAlloc {
        Regions* regions;
    public:
        PageAlloc();
        template<typename... PhySpan>
        void init(Regions& regions, PhySpan&&... span);
        void relink(Regions& regions);
        maybe<Page> alloc_page();
        maybe<Page> alloc_page(uintptr_t);
        void reserve_page_at(uintptr_t);
        template<typename T>
        maybe<T*> alloc_page_as();
        void free_page(Page);
    };

    template<typename... Span>
    void PageAlloc::init(Regions& regions, Span&&... reserved_span){
        this->regions = &regions;
        for(auto i = 0; i < regions.num; i++){
            regions[i].init(reserved_span...);
        }
    }
    template<typename T>
    maybe<T*> PageAlloc::alloc_page_as() {
        return this->alloc_page().then([](Page&& page){ return reinterpret_cast<T*>(page); });
    }
}