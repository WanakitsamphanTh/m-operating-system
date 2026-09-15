#include <cstdint>
#include <cstring>
#include <kernel_core.hpp>

extern "C" [[noreturn]] void kernel_main(MK::KernelSystem* hi_kernel_ptr,MK::PhySpan* hi_unmap_spans, uintptr_t hi_sp){
    __asm__ __volatile__(
        "mov %%sp, %0"
        :
        : "r"(hi_sp)
        : "sp"
    );
    auto& console = hi_kernel_ptr->console;
    auto& regions = hi_kernel_ptr->regions;
    auto& page_allocator = hi_kernel_ptr->page_alloc;
    auto& page_manager = hi_kernel_ptr->page_manager;


    kernel_hault();    
}