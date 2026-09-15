#pragma once
#include <cstddef>
#include <cstdint>

namespace MK {

    constexpr uint64_t userspace_vaddr = 0ull;
    constexpr uint64_t kernel_vaddr = 0xffffull << 48;
    constexpr uint64_t vaddr_max = 0xffffffffffff;

    constexpr size_t page_size = 4096;  // 4 KiB
    constexpr size_t l2_block_size = 512 * page_size; // 2MB
    constexpr size_t l1_block_size = 512 * l2_block_size; // 1 GiB
    constexpr size_t l0_entry_size = 512 * l1_block_size; // 512 GiB

    template<size_t al, class Ptr>
    inline Ptr align(Ptr _ptr) { 
        auto ptr = reinterpret_cast<const uint8_t*>(_ptr);
        return reinterpret_cast<Ptr>(size_t(ptr + (al - 1)) & ~(al - 1)); 
    }

    template<size_t al, class Ptr>
    inline Ptr align_down(Ptr _ptr) { 
        auto ptr = reinterpret_cast<uint64_t>(_ptr);
        return reinterpret_cast<Ptr>((_ptr / al) * al); 
    }

    template<size_t al, class Ptr>
    inline bool is_align(Ptr ptr) {
        return reinterpret_cast<uintptr_t>(ptr) % al == 0;
    }

    using Page = uintptr_t;
    using Block = uintptr_t;

    using Stack = char[page_size];
}