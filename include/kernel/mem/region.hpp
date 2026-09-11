#pragma once
#include <cstddef>
#include <cstdint>
#include <mstd/monadic/maybe.hpp>
#include <kernel/mem/page.hpp>
#include "kernel/mem/mem.hpp"

namespace MK {
    using mstd::maybe;

    struct PhySpan {
        uintptr_t base;
        size_t size;

        uintptr_t end() const;
        bool overlap(const PhySpan& other) const;
        bool is_part_of(const PhySpan& parent) const;
    };

    struct MemRegion {
        PhySpan span;
        uint8_t* bitmap;
        size_t bitmap_size;

        template<typename... Span>
        void init(Span&&... reserved_span);
        maybe<Page> alloc_page();
        maybe<Page> alloc_page(uintptr_t);
        void free_page(Page pg);
        bool reserve_page_at(uintptr_t);
        void reserve_pages(const PhySpan& span);
        bool include(Page);
    private:
        void init_bitmap(const PhySpan& span);
    };

    struct Regions {
        MemRegion regions[8];
        size_t num;
        MemRegion& operator[](size_t);
    };

    extern Regions regions;

    template<typename... Span>
    void MemRegion::init(Span&&... reserved_span){
        uintptr_t bitmap_addr = this->span.base;
        uintptr_t bitmap_size = this->span.size / (page_size * 8);
        while (true){
            PhySpan bitmap_span{bitmap_addr, bitmap_size}; 
            if(!bitmap_span.is_part_of(this->span)) {
                break;
            }
            if(!(bitmap_span.overlap(reserved_span) || ...)){
                init_bitmap(bitmap_span);
                (this->reserve_pages(reserved_span), ...);
                return;
            }
            bitmap_addr += page_size;
        }
        this->bitmap = nullptr;
    }
}