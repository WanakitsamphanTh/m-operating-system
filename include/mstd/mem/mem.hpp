#pragma once
#include <cstdint>
#include <cstddef>
#include "mstd/monadic/maybe.hpp"

namespace mstd {
    struct protection {
        static constexpr uint64_t read = 0;
        static constexpr uint64_t write = 0;
        static constexpr uint64_t exec = 0;
    };
    

    maybe<void*> map(void* addr, size_t size, uint64_t prot);
    maybe<void*> remap(void* addr, size_t size, size_t new_size, uint64_t prot);
    void unmap(void* addr, size_t size);

    template<class T>
    T align_up(T t, size_t alignment){
        uintptr_t ui = reinterpret_cast<uintptr_t>(t);
        ui = (ui + alignment - 1) & ~(alignment - 1);
        return reinterpret_cast<T>(ui);
    }

    template<class T>
    T align_down(T t, size_t alignment){
        uintptr_t ui = reinterpret_cast<uintptr_t>(t);
        ui -= ui % alignment;
        return reinterpret_cast<T>(ui);
    }

}