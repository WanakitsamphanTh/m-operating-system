#pragma once
#include <cstddef>
#include <cstdint>
#include <mstd/monadic/maybe.hpp>
#include <type_traits>
#include "kernel/mem/mem.hpp"
#include "kernel/common.hpp"

namespace MK {
    using mstd::maybe;

    struct PhySpan {
        uintptr_t base;
        size_t size;

        uintptr_t end() const;
        bool overlap(const PhySpan& other) const;
        bool is_part_of(const PhySpan& parent) const;
    };

    class MemRegion {
    protected:
        PhySpan span;
        uint8_t* bitmap;
        size_t bitmap_size;
    public:
        const PhySpan& get_span() const;
        PhySpan& get_span();
        maybe<Page> alloc_page();
        maybe<Page> alloc_page(uintptr_t);
        void free_page(Page pg);
        bool reserve_page_at(uintptr_t);
        void reserve_pages(const PhySpan& span);
        bool include(Page);
    };

    template<KernelSession session>
    class Region {};

    template<>
    class Region<Bootstrap>: public MemRegion {
        void init_bitmap(const PhySpan& span);
    public:
        template<typename... Span>
        void init(Span&&... reserved_span);
        Region<Permanent> relocate() &&;
    };

    template<>
    class Region<Permanent>: public MemRegion {
        friend class Region<Bootstrap>;
    public:
        Region(Region&& other) {
            span = other.span; 
            bitmap = other.bitmap; 
            bitmap_size = other.bitmap_size;
        }
        Region& operator=(Region&& other){
            span = other.span;
            bitmap = other.bitmap;
            bitmap_size = other.bitmap_size;
            return *this;
        }
    };

    struct MemRegions {
        MemRegion regions[8];
        size_t num;
    };

    template<KernelSession session>
    struct Regions : public MemRegions {
        Regions() = default;
        Region<session>& operator[](size_t index) {
            return *reinterpret_cast<Region<session>*>(&regions[index]);
        }
        template<KernelSession s = session>
            requires std::is_same_v<s, Bootstrap>
        Regions<Permanent> relocate() && {
            for(auto i = 0; i < num; i++)
                std::move((*this)[i]).relocate();
            return std::move(*reinterpret_cast<Regions<Permanent>*>(this));
        }

        template<KernelSession s = session>
            requires std::is_same_v<s, Permanent>
        Regions(Regions&& other);
        template<KernelSession s = session>
            requires std::is_same_v<s, Permanent>
        Regions& operator=(Regions&& other);
    };

    template<typename... Span>
    void Region<Bootstrap>::init(Span&&... reserved_span){
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