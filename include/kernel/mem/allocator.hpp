#pragma once

#include <cstdint>
#include "kernel/common.hpp"
#include "kernel/mem/page.hpp"
#include "kernel/mem/region.hpp"
#include "mstd/monadic/maybe.hpp"
#include "kernel/mem/mem.hpp"
#include "kernel/mem/region.hpp"

inline uint64_t ceil_div(uint64_t x, uint64_t y){
    return x/y + (((x % y)) ? 1 : 0);
}

namespace MK {
    using mstd::maybe;

    
    template<KernelSession session>
    class PageAlloc {
    protected:
        Regions<session> regions;
    public:
        PageAlloc(){}
        Regions<session>& get_regions();
        maybe<Page> alloc_page();
        maybe<Page> alloc_page(uintptr_t);
        void reserve_page_at(uintptr_t);
        template<typename T>
        maybe<T*> alloc_page_as();
        void free_page(Page);
    };

    template<KernelSession session>
    class PageAllocator;

    template<>
    class PageAllocator<Bootstrap>: public PageAlloc<Bootstrap> {
    public:
        template<typename... PhySpan>
        void init(PhySpan&&... span);
        PageAllocator<Permanent> relocate() &&;
    };

    template<>
    class PageAllocator<Permanent>: public PageAlloc<Permanent> {
    public:
        friend class PageAllocator<Bootstrap>;
        PageAllocator(){}
        PageAllocator(PageAllocator&& other){
            regions = std::move(other.regions);
        }
        PageAllocator& operator=(PageAllocator<Permanent>&& other){
            regions = std::move(other.regions);
            return *this;
        }
    };

    
    template<typename... Span>
    void PageAllocator<Bootstrap>::init(Span&&... reserved_span){
        for(auto i = 0; i < regions.num; i++){
            regions[i].init(reserved_span...);
        }
    }

    template<KernelSession session>
    template<typename T>
    maybe<T*> PageAlloc<session>::alloc_page_as() {
        return this->alloc_page().then([](Page&& page){ return reinterpret_cast<T*>(page); });
    }

    template<KernelSession session>
    maybe<Page> PageAlloc<session>::alloc_page(){
        for(auto i = 0; i < regions.num; i++)
            if(auto page = regions[i].alloc_page(); page.is_valid()) 
                return page;
        return mstd::nothing;   
    }

    template<KernelSession session>
    maybe<Page> PageAlloc<session>::alloc_page(uintptr_t addr){
        addr = align_down<page_size>(addr);
        for(auto i = 0; i < regions.num; i++)
            if(auto page = regions[i].alloc_page(addr); page.is_valid()) 
                return page;
        return mstd::nothing;   
    }

    template<KernelSession session>
    void PageAlloc<session>::reserve_page_at(uintptr_t addr){
        addr = align_down<page_size>(addr);
        for(auto i = 0; i < regions.num; i++)
            if(regions[i].reserve_page_at(addr)) 
                break;
    }

    template<KernelSession session>
    void PageAlloc<session>::free_page(Page page){
        for(size_t i = 0; i < regions.num; i++)
            if(auto& region = regions[i]; region.include(page))
                region.free_page(page);
    }

    template<KernelSession session>
    Regions<session>& PageAlloc<session>::get_regions(){
        return regions;
    }
}