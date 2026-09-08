#include "kernel/mem/region.hpp"
#include "mstd/string.hpp"

template<size_t al, class Ptr>
inline Ptr align(Ptr _ptr) { 
    auto ptr = reinterpret_cast<const uint8_t*>(_ptr);
    return reinterpret_cast<Ptr>(size_t(ptr + (al - 1)) & ~(al - 1)); 
}


namespace MK {
    using mstd::maybe;
    using mstd::nothing;

    uintptr_t PhySpan::end() const { return base + size; }
    
    bool PhySpan::overlap(const PhySpan& other) const { 
        if(base == other.base) return false;
        if(base < other.base) return end() <= other.base;
        if(base > other.base) return other.end() <= base;
        return false;
    }

    bool PhySpan::is_part_of(const PhySpan& parent) const {
        return base >= parent.base && end() <= parent.end();
    }

    MemRegion& Regions::operator[](size_t ind) { return this->regions[ind]; }

    maybe<Page> MemRegion::alloc_page(){
        if(this->bitmap == nullptr) return nothing;
        for(size_t i = 0; i < this->bitmap_size; i++){
            auto& byte = this->bitmap[i];
            if(byte == 0xff) continue;
            for(auto j = 0; j < 8; j++){
                if((byte >> j) & 0x01){
                    byte |= (1 << j);
                    return mstd::some<Page>(reinterpret_cast<Page>(this->span.base + (i * 8 + j) * 4096));
                }
            }
        }
        return nothing;
    }
    
    void MemRegion::free_page(Page pg){
        if(this->bitmap == nullptr) return;
        if(!this->include(pg)) return;
        uintptr_t page_ind = pg / 4096;
        auto byte_ind = page_ind / 8;
        auto bit_ind = page_ind % 8;
        this->bitmap[byte_ind] &= ~(1 << bit_ind);
    }

    void MemRegion::init_bitmap(const PhySpan& span){
        this->bitmap = reinterpret_cast<uint8_t*>(span.base);
        this->bitmap_size = span.size;
        mstd::memset(this->bitmap, 0x00, this->bitmap_size);
        reserve_pages(span);
    }

    void MemRegion::reserve_pages(const PhySpan& span){
        if(!span.is_part_of(this->span)) return;
        for(auto page = (span.base / 4096) * 4096; page < align<4096>(span.end()); page += 4096){
            uintptr_t page_ind = page / 4096;
            auto byte_ind = page_ind / 8;
            auto bit_ind = page_ind % 8;
            this->bitmap[byte_ind] |= (1 << bit_ind);
        }
    }

    bool MemRegion::include(Page pg){
        return pg >= this->span.base && pg < this->span.end();
    }
}