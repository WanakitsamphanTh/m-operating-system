#if OK

#include "mstd/mem/alloc.hpp"
#include "mstd/mem/mem.hpp"
#include <cstdint>

namespace mstd {

    void mem_header_t::set_header(uint32_t size, uint32_t prev_size, bool occupied){
        this->size = size + align_gap;
        this->prev_size = prev_size;
        this->occupied = occupied;
    }
    
    mem_header_t* mem_header_t::from_payload(void* ptr) {
        auto byte_ptr = reinterpret_cast<uint8_t*>(ptr);
        if(byte_ptr == nullptr) return nullptr;
        auto offset = *reinterpret_cast<uint32_t*>(ptr - sizeof(uint32_t));
        return reinterpret_cast<mem_header_t*>(byte_ptr - offset);

    }
    void mem_header_t::set_occupied() { occupied = true; }
    void mem_header_t::set_unoccupied() { occupied = false; }


    free_list_allocator::free_list_allocator(){}
    free_list_allocator::free_list_allocator(heap_buffer&& buf): buffer(buf){}
    free_list_allocator::free_list_allocator(free_list_allocator&& other)
        : buffer(std::move<heap_buffer>(other.buffer)),
        freed(std::move<free_cache_t>(other.freed)) {}

    maybe<void*> free_list_allocator::alloc(size_t size, size_t align){
        uintptr_t ptr;
        size_t total_size = size + sizeof(mem_header_t) + 4;

        /* try bump */
        ptr = brk + sizeof(mem_header_t) + 4;
        uintptr_t aligned_ptr = align ? align_up(ptr, align) : ptr;

        if(aligned_ptr <= end) {
            auto align_size = aligned_ptr - ptr;
            total_size += align_size;
            auto& header = *reinterpret_cast<mem_header_t*>(brk);
            header.set_header(total_size, prev_size, align_size, true);
            *reinterpret_cast<uint32_t*>(aligned_ptr - 4) = align_size;
            prev_size = total_size;
            brk += total_size;
            return some<void*>(aligned_ptr);
        }

        /* check from free list */
        return acquire_freed(total_size, align)
                .apply_when_nothing([this, size, align]() -> maybe<void*> {
                    if(this->buffer.extend())
                        return this->alloc(size, align);
                    return nothing;
                }
            );
    }

    void free_list_allocator::dealloc(void* ptr){
        if(ptr == nullptr) return;
        if(!buffer.include(ptr)) return;
        auto& header = mem_header_t::from_payload(ptr);
        header.set_unoccupied();
        /* add to free list or tree */
    }

    maybe<void*> free_list_allocator::acquire_freed(size_t size, size_t align){
        return nothing;
    }

    free_list_allocator::~free_list_allocator(){}
}


#endif