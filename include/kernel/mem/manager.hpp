#pragma once
#include "kernel/mem/page.hpp"
#include "kernel/mem/allocator.hpp"
#include "mstd/monadic/maybe.hpp"
#include "kernel/mem/mem.hpp"
#include "kernel/common.hpp"
#include <cstdint>

namespace MK {
    
    extern "C" void enable_mmu(uint64_t mair, uint64_t tcr, PageDescriptor* l0_table);

    template<KernelSession session>
    class PageAllocator;

    template<KernelSession session>
    class PageTablesManager {
    protected:
        PageAllocator<session> allocator;
        PageTable l0_table;
    public:
        PageTablesManager();
        PageAllocator<session>& get_allocator();
        void map(uintptr_t, size_t, MapMode, uint64_t mem_type, uint64_t ap);
        void map(void*, size_t, uint64_t mem_type, uint64_t ap);
        void remap(void*, size_t, size_t, uint64_t mem_type, uint64_t ap);
        void unmap(uintptr_t, size_t);
    protected:
        void map_l0(uintptr_t, size_t, MapMode, uint64_t mem_type, uint64_t ap);
        void map_l1(PageDescriptor*, uintptr_t, size_t, MapMode, uint64_t mem_type, uint64_t ap);
        void map_l2(PageDescriptor*, uintptr_t, size_t, MapMode, uint64_t mem_type, uint64_t ap);
        void map_l3(PageDescriptor*, uintptr_t, size_t, MapMode, uint64_t mem_type, uint64_t ap);
    };

    template<KernelSession session>
    class PageManager;

    template<>
    class PageManager<Bootstrap>: public PageTablesManager<Bootstrap> {
        PageManager<Permanent> relocate() &&;
    public:
        void init(PageTable tab);
        PageManager<Permanent> enable_mmu_and_relocate() &&;
    };

    template<>
    class PageManager<Permanent>: public PageTablesManager<Permanent> {
        friend class PageManager<Bootstrap>;
    public:
        PageManager(){}
        PageManager(PageManager&& other){
            allocator = std::move(other.allocator); 
            l0_table = other.l0_table;
        }
        PageManager& operator=(PageManager&& other){
            allocator = std::move(other.allocator); 
            l0_table = other.l0_table;
            return *this;
        }
    };
}