#pragma once

#include <cstdint>
#include <cstddef>
#include <utility>
#include "mstd/monadic/maybe.hpp"

template<size_t al, class Ptr>
inline Ptr align(Ptr _ptr) { 
    auto ptr = reinterpret_cast<const uint8_t*>(_ptr);
    return reinterpret_cast<Ptr>(size_t(ptr + (al - 1)) & ~(al - 1)); 
}

struct be_uint64_t {uint8_t bytes[8]; be_uint64_t() = delete; operator uint64_t() const; uint64_t to_le() const;} ;
struct be_uint32_t {uint8_t bytes[4]; be_uint32_t() = delete; operator uint32_t() const; uint32_t to_le() const; } ;

namespace MK {
    enum class FDTNodeType: uint32_t {
        BeginNode = 0x00000001, EndNode = 0x00000002, Prop = 0x00000003, Nop = 0x00000004, End = 0x00000009
    };

    struct FDT;
    struct FDTEntry;
    struct FDTHeader;
    struct FDTProperty;

    struct FDTEntry {
        be_uint64_t addr;
        be_uint64_t size;
    };

    struct FDTHeader {
        be_uint32_t tag;
        char name[];

        mstd::maybe<const FDTProperty&> search_property(const char*, FDT&) const;
    };

    struct FDTProperty {
        be_uint32_t tag;
        be_uint32_t len;
        be_uint32_t name_off;
        char data[];

        const char* get_name(FDT&) const;
        const char* as_string() const;
        mstd::maybe<uint32_t> u32_at(size_t) const;
        mstd::maybe<uint32_t> u64_at(size_t) const;
    };

    struct FDT {
        uint8_t* base_ptr;
        uint32_t total_size;
        uint32_t struct_off;
        uint32_t string_off;
        uint32_t rsv_map_off;
        uint32_t version;
        uint32_t last_comp_version;
        uint32_t phy_cpu_id;
        uint32_t string_size;
        uint32_t struct_size;

        static constexpr uint32_t magic_number = 0xd00dfeed;
        static mstd::maybe<FDT> try_read_fdt(uint8_t* fdt);
        mstd::maybe<const FDTHeader&> find_node_prefix(const char*) const;
    };
}

extern "C" uint32_t read_be_32(const uint8_t* bytes);
extern "C" uint64_t read_be_64(const uint8_t* bytes);
