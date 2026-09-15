#pragma once
#include <cstdint>
#include <cstddef>
#include "kernel/mem/mem.hpp"
#include "mem.hpp"


namespace MK {

    struct MemPage{
        uintptr_t start;
        uintptr_t end;
    };

    class PageDescriptor {
        uint64_t descriptor;
    public:
        PageDescriptor(): descriptor(0){}
        PageDescriptor(uintptr_t addr);
        PageDescriptor(const PageDescriptor&);
        PageDescriptor& operator=(uint64_t);
        PageDescriptor& operator=(const PageDescriptor&);
        operator uint64_t() const;

        static PageDescriptor make_table(PageDescriptor* addr);
        static PageDescriptor make_table(uintptr_t addr);
        static PageDescriptor make_page(uintptr_t addr, uint64_t mem_type, uint64_t ap);
        static PageDescriptor make_block(uintptr_t addr, uint64_t mem_type, uint64_t ap);

        uintptr_t get_addr() const;
        bool is_valid() const;
        bool is_fault() const;
        bool is_table() const;
        bool is_block() const;
    };

    class PageInfo {
    public:
        static constexpr uint64_t VALID = 0b01;
        static constexpr uint64_t TABLE = 0b10;
        static constexpr uint64_t PAGE = 0b10;
        static constexpr uint64_t BLOCK = 0b00;
        static constexpr uint64_t AF = 1ull << 10;
        static constexpr uint64_t AP_MASK = 0b11ull << 6;
        static constexpr uint64_t SH_MASK = 0b11ull << 8;
        static constexpr uint64_t ATTR_MASK = 0b111ull << 2;
        static constexpr uint64_t PXN = 1ull << 53;
        static constexpr uint64_t UXN = 1ull << 54;
    
        struct MemoryType {
            static constexpr uint64_t Normal = 0ull;
            static constexpr uint64_t Device = 1ull;
        };

        struct AP {
            static constexpr uint64_t PRW = 0x00;  // Read/Write for EL1-3, no access for EL0
            static constexpr uint64_t RW = 0x01;   // Read/Write for all
            static constexpr uint64_t PRO = 0x10;  // Read only for EL1-3, no access for EL0
            static constexpr uint64_t RO = 0x11;   // Read only for all
        };

        struct SH {
            static constexpr uint64_t None = 0x00;
            static constexpr uint64_t Outer = 0x10;
            static constexpr uint64_t Inner = 0x11;
        };
    };

    static_assert(sizeof(PageDescriptor) == 8, "sizeof(PageDescriptor) must be 64-bit");

    using PageTable = PageDescriptor*;

    enum class MapMode {
        Id, Direct, DirectKernel, New
    };

    class PageAlloc;

    class PageTablesManager {
        bool mmu_enabled;
        PageAlloc* allocator;
        alignas(page_size) PageDescriptor l0_table[512];
    public:
        PageTablesManager();
        void init(PageAlloc& alloc);
        void relink(PageAlloc& alloc);
        void map(uintptr_t, size_t, MapMode, uint64_t mem_type, uint64_t ap);
        void map(void*, size_t, uint64_t mem_type, uint64_t ap);
        void remap(void*, size_t, size_t, uint64_t mem_type, uint64_t ap);
        void unmap(uintptr_t, size_t);
        void enable_mmu();
    private:
        void map_l0(uintptr_t, size_t, MapMode, uint64_t mem_type, uint64_t ap);
        void map_l1(PageDescriptor*, uintptr_t, size_t, MapMode, uint64_t mem_type, uint64_t ap);
        void map_l2(PageDescriptor*, uintptr_t, size_t, MapMode, uint64_t mem_type, uint64_t ap);
        void map_l3(PageDescriptor*, uintptr_t, size_t, MapMode, uint64_t mem_type, uint64_t ap);
    };

    static constexpr uintptr_t max_virtual_addr = 0xffffffffffffffff;
    static constexpr uintptr_t phy_base = 1ull << 47 | kernel_vaddr;
    static constexpr uintptr_t kernel_base = kernel_vaddr;

    // MAIR_EL1: one attribute byte per PageInfo::MemoryType index.
    struct MAIR {
        static constexpr uint64_t DeviceNGNRNE = 0x00; // Device-nGnRnE, for MMIO
        static constexpr uint64_t NormalWBWA = 0xff;   // Normal, Inner/Outer Write-Back Write-Allocate

        static constexpr uint64_t value =
            (NormalWBWA << (PageInfo::MemoryType::Normal * 8)) |
            (DeviceNGNRNE << (PageInfo::MemoryType::Device * 8));
    };

    struct TCR {
        static constexpr uint64_t T0SZ = 64 - 48;            // bits[5:0]:  48-bit input address via TTBR0
        static constexpr uint64_t T1SZ = (64 - 48) << 16;   // bits[21:16]:  48-bit input address via TTBR1
        
        static constexpr uint64_t IRGN0_WBWA = 0b01ull << 8;  // bits[9:8]:  inner WB write-allocate
        static constexpr uint64_t IRGN1_WBWA = 0b01ull << 24;  // bits[25:24]:  inner WB write-allocate
        
        static constexpr uint64_t ORGN0_WBWA = 0b01ull << 10; // bits[11:10]: outer WB write-allocate
        static constexpr uint64_t ORGN1_WBWA = 0b01ull << 26; // bits[27:26]: outer WB write-allocate
        
        static constexpr uint64_t SH0_INNER = 0b11ull << 12;  // bits[13:12]: inner shareable
        static constexpr uint64_t SH1_INNER = 0b11ull << 28;  // bits[29:28]: inner shareable
        
        static constexpr uint64_t TG0_4KB = 0b00ull << 14;    // bits[15:14]: 4KB granule
        static constexpr uint64_t TG1_4KB = 0b10ull << 30;    // bits[31:30]: 4KB granule
        
        static constexpr uint64_t EPD0 = 1ull << 7;         // disable TTBR1_EL0 walks
        static constexpr uint64_t EPD1 = 1ull << 23;          // disable TTBR1_EL1 walks

        static constexpr uint64_t value =
            T0SZ | IRGN0_WBWA | ORGN0_WBWA | SH0_INNER | TG0_4KB
            | T1SZ | IRGN1_WBWA | ORGN1_WBWA | SH1_INNER | TG1_4KB;
    };

    inline void* phy2virt(uintptr_t ptr) { return reinterpret_cast<void*>(MK::phy_base + ptr); }

    template<class T>
    T phy2virt_as(uintptr_t ptr){ return reinterpret_cast<T>(phy2virt(ptr)); }

    template<class T>
    uintptr_t virt2phy(T ptr){ return reinterpret_cast<uintptr_t>(ptr) & ~phy_base; }

    inline void* phy2kvirt(uintptr_t ptr){ return reinterpret_cast<void*>(kernel_base + ptr); }

    template<class T>
    T phy2kvirt_as(uintptr_t ptr){ return reinterpret_cast<T>(phy2kvirt(ptr)); }

    template<class T>
    uintptr_t kvirt2phy(T ptr){ return reinterpret_cast<uintptr_t>(ptr) & ~kernel_base; }

    extern "C" void enable_mmu(uint64_t mair, uint64_t tcr, PageDescriptor* l0_table);
}