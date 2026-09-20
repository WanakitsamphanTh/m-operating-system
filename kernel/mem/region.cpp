#include "kernel/mem/region.hpp"
#include "mstd/string.hpp"
#include "kernel/mem/page.hpp"

namespace MK {
    using mstd::maybe;
    using mstd::nothing;
    
    uintptr_t PhySpan::end() const { return base + size; }
    
    bool PhySpan::overlap(const PhySpan& other) const { 
        if(base < other.base) return end() > other.base;
        if(base > other.base) return other.end() > base;
        return true;
    }

    bool PhySpan::is_part_of(const PhySpan& parent) const {
        return base >= parent.base && end() <= parent.end();
    }

    const PhySpan& MemRegion::get_span() const { return span; }
    PhySpan& MemRegion::get_span() { return span; }

    maybe<Page> MemRegion::alloc_page(){
        if(this->bitmap == nullptr) return nothing;
        for(size_t i = 0; i < this->bitmap_size; i++){
            auto& byte = this->bitmap[i];
            if(byte == 0xff) continue;
            for(auto j = 0; j < 8; j++){
                if(!((byte >> j) & 0x01)){
                    byte |= (1 << j);
                    return mstd::some<Page>(reinterpret_cast<Page>(this->span.base + (i * 8 + j) * page_size));
                }
            }
        }
        return nothing;
    }

    maybe<Page> MemRegion::alloc_page(uintptr_t addr) {
        if(this->bitmap == nullptr) return nothing;
        if(!PhySpan{addr, page_size}.is_part_of(this->span)) return nothing;
        uintptr_t page_ind = (addr - this->span.base) / page_size;
        auto byte_ind = page_ind / 8;
        auto bit_ind = page_ind % 8;
        if((this->bitmap[byte_ind] >> bit_ind) & 0x01)
            return nothing;
        this->bitmap[byte_ind] |= 1 << bit_ind;
        return mstd::some<Page>(static_cast<Page>(addr));
    }
    
    void MemRegion::free_page(Page pg){
        if(this->bitmap == nullptr) return;
        if(!this->include(pg)) return;
        uintptr_t page_ind = (pg - this->span.base) / page_size;
        auto byte_ind = page_ind / 8;
        auto bit_ind = page_ind % 8;
        this->bitmap[byte_ind] &= ~(1 << bit_ind);
    }

    void Region<Bootstrap>::init_bitmap(const PhySpan& span){
        this->bitmap = reinterpret_cast<uint8_t*>(span.base);
        this->bitmap_size = span.size;
        mstd::memset(this->bitmap, 0x00, this->bitmap_size);
        reserve_pages(span);
    }

    Region<Permanent> Region<Bootstrap>::relocate() &&{
        bitmap = phy2kvirt_as<uint8_t*>(reinterpret_cast<uintptr_t>(bitmap));
        return std::move(*reinterpret_cast<Region<Permanent>*>(this));
    }

    bool MemRegion::reserve_page_at(uintptr_t addr){
        if(this->bitmap == nullptr) return false;
        if(!PhySpan{addr, page_size}.is_part_of(this->span)) return false;
        uintptr_t page_ind = (addr - this->span.base) / page_size;
        auto byte_ind = page_ind / 8;
        auto bit_ind = page_ind % 8;
        if((this->bitmap[byte_ind] >> bit_ind) & 0x01)
            return false;
        this->bitmap[byte_ind] |= 1 << bit_ind;
        return true;
    }

    void MemRegion::reserve_pages(const PhySpan& span){
        if(!span.is_part_of(this->span)) return;
        for(auto page = (span.base / page_size) * page_size; page < align<page_size>(span.end()); page += page_size){
            uintptr_t page_ind = (page - this->span.base) / page_size;
            auto byte_ind = page_ind / 8;
            auto bit_ind = page_ind % 8;
            this->bitmap[byte_ind] |= (1 << bit_ind);
        }
    }

    bool MemRegion::include(Page pg){
        return pg >= this->span.base && pg < this->span.end();
    }
}