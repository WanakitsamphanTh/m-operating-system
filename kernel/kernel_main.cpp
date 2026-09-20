#include <cstdint>
#include <cstring>
#include <kernel_core.hpp>
#include "kernel/dtb.hpp"
#include <kernel/io.hpp>
#include "kernel/mem/region.hpp"
#include "kernel/mem/page.hpp"
#include "kernel/mem/allocator.hpp"
#include "kernel/mem/manager.hpp"
#include "mstd/monadic/maybe.hpp"
#include "mstd/scope_guard.hpp"
#include "mstd/string.hpp"
#include "mstd/mem/alloc.hpp"

MK::PageManager<MK::Permanent> page_manager;
MK::KernelConsole console;

extern "C" [[noreturn]] 
void kernel_main(){
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