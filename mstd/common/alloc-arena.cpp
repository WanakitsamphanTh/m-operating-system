#include "mstd/mem/alloc.hpp"
#include "mstd/mem/mem.hpp"
#include <utility>

namespace mstd {
    arena_allocator::arena_allocator(){}
    arena_allocator::arena_allocator(heap_buffer&& buf): buffer(std::forward<heap_buffer>(buf)) {}
    arena_allocator(arena_allocator&& other){
        buffer = std::move<heap_buffer>(other.buffer);
        start = other.start;
        end = other.end;
        brk = other.brk;
        other.start = other.end = other.brk = nullptr;
    }
    maybe<void*> arena_allocator::alloc(size_t size, size_t align){
        uintptr_t ptr = reinterpret_cast<uintptr_t>(brk);
        if(align != 0)
            ptr = align_up(ptr, align);
        if(ptr + size > end) return nothing;
        brk = reinterpret_cast<void*>(ptr + size);
        return some<void*>(reinterpret_cast<void*>(ptr));
    }

    void arena_allocator::dealloc(void*){ /* do nothing */}
    arena_allocator::~arena_allocator(){}
}