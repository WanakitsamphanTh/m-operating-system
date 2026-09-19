#include <cstdint>
#include <cstring>
#include <kernel_core.hpp>
#include <kernel/io.hpp>
#include "kernel/mem/region.hpp"
#include "kernel/mem/page.hpp"
#include "kernel/mem/allocator.hpp"
#include "mstd/monadic/maybe.hpp"
#include "mstd/scope_guard.hpp"
#include "mstd/string.hpp"
#include "mstd/mem/alloc.hpp"

extern "C" [[noreturn]] 
void kernel_main(MK::KernelSystem* hi_kernel_ptr, MK::PhySpan* hi_unmap_spans, size_t unmap_count){
    auto& console = hi_kernel_ptr->console;
    auto& regions = hi_kernel_ptr->regions;
    auto& page_allocator = hi_kernel_ptr->page_allocator;
    auto& page_manager = hi_kernel_ptr->page_manager;
    
    uintptr_t sp, pic;
    __asm__ __volatile__(
        "mov %0, sp\n"
        "adr %1, .\n"
        : "=r"(sp), "=r"(pic)
    );

    console.writeln("kernel now works at the higher half space");
    console.writef("stack pointer: {}\n", mstd::as_ptr(sp));
    console.writef("instruction pointer: {}\n", mstd::as_ptr(pic));
    
    kernel_halt();
}