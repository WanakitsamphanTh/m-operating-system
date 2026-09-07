#pragma once
#include <cstdint>
#include <type_traits>

namespace MK {

    using Page = uintptr_t;

    struct MemPage{
        uintptr_t start;
        uintptr_t end;
    };

    class PageDescriptor {
        uint64_t descriptor;
    public:
        PageDescriptor(uintptr_t addr);
        PageDescriptor(const PageDescriptor&);
        PageDescriptor& operator=(uint64_t);
        PageDescriptor& operator=(const PageDescriptor&);
        operator uint64_t() const;

        static PageDescriptor make_table(PageDescriptor* addr);
        static PageDescriptor make_table(uintptr_t addr);
        static PageDescriptor make_block(uintptr_t addr, uint64_t mem_type, uint64_t ap);

        uintptr_t get_addr() const;
        bool is_valid() const;
        bool is_fault() const;
        bool is_table() const;
        bool is_block() const;
    public:
        static constexpr uint64_t VALID = 0x01;
        static constexpr uint64_t TABLE = 0x10;
        static constexpr uint64_t BLOCK = 0x00;
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

    extern "C" void enable_mmu(PageDescriptor*);
    extern "C" void set_page_table(PageDescriptor*);

}