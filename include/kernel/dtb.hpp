#pragma once

#include <cstdint>
#include <cstddef>
#include <utility>
#include "mstd/monadic/maybe.hpp"
#include "mstd/fmt/io_core.hpp"

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
    struct FDTNode;
    struct FDTProperty;

    struct FDTEntry {
        be_uint64_t addr;
        be_uint64_t size;
    };

    struct FDTNode {
        be_uint32_t tag;
        char name[];

        mstd::maybe<const FDTNode&> find_node(const char*) const;
        mstd::maybe<const FDTNode&> find_node_prefix(const char*) const;
        mstd::maybe<const FDTNode&> find_node(const char*, bool) const;
        mstd::maybe<const FDTProperty&> find_property(const char*, const FDT&) const;
        const uint8_t* skip() const;
        
        template<class concrete_writer, mstd::fmt_buffer fmt_buf>
        const uint8_t* print(mstd::writer_core<concrete_writer, fmt_buf>& writer, const FDT& fdt, size_t depth) const;
    };

    struct FDTProperty {
        be_uint32_t tag;
        be_uint32_t len;
        be_uint32_t name_off;
        char data[];

        const char* get_name(const FDT&) const;
        const char* as_string() const;
        mstd::maybe<uint32_t> u32_at(size_t) const;
        mstd::maybe<uint32_t> u64_at(size_t) const;

        template<class concrete_writer, mstd::fmt_buffer fmt_buf>
        const uint8_t* print(mstd::writer_core<concrete_writer, fmt_buf>& writer, const FDT& fdt, size_t depth) const;
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
        mstd::maybe<const FDTNode&> find_node(const char*) const;
        mstd::maybe<const FDTNode&> find_node_prefix(const char*) const;
        mstd::maybe<const FDTNode&> find_node(const char*, bool) const;

        template<class concrete_writer, mstd::fmt_buffer fmt_buf>
        void print(mstd::writer_core<concrete_writer, fmt_buf>& writer) const;
    };

    template<class concrete_writer, mstd::fmt_buffer fmt_buf>
    void FDT::print(mstd::writer_core<concrete_writer, fmt_buf>& writer) const{
        const auto& root = *reinterpret_cast<const FDTNode*>(this->struct_off + this->base_ptr);
        root.print<concrete_writer, fmt_buf>(writer, *this, 0);
    }

    template<class concrete_writer, mstd::fmt_buffer fmt_buf>
    const uint8_t* FDTProperty::print(mstd::writer_core<concrete_writer, fmt_buf>& writer, const FDT& fdt, size_t depth) const {
        for(auto i = 0; i < depth; i++)
            writer.writec('\t');
        writer.writef("{} :", reinterpret_cast<const char*>(fdt.base_ptr + fdt.string_off + this->name_off));
        for(auto i = 0; i < this->len; i++){
            writer.writef(" {02h}", uint64_t(this->data[i]));
        }
        writer.writec('\n');
        return align<4>(reinterpret_cast<const uint8_t*>(this) + sizeof(FDTProperty) + this->len);
    }

    template<class concrete_writer, mstd::fmt_buffer fmt_buf>
    const uint8_t* FDTNode::print(mstd::writer_core<concrete_writer, fmt_buf>& writer, const FDT& fdt, size_t depth) const {
        for(auto i = 0; i < depth; i++)
            writer.writec('\t');
        writer.writef("{}\n", this->name);
        auto ptr = reinterpret_cast<const uint8_t*>(this);
        ptr += sizeof(FDTNode) + strlen(this->name) + 1;
        ptr = align<4>(ptr);    /* move towards the header*/
        while(true){
            auto tag = static_cast<MK::FDTNodeType>(
                static_cast<uint32_t>(
                    *reinterpret_cast<const be_uint32_t*>(ptr)
                )
            );
            switch(tag){
                case MK::FDTNodeType::Nop:
                    ptr += 4;
                    break;
                case MK::FDTNodeType::BeginNode:{
                    const auto& node = *reinterpret_cast<const FDTNode*>(ptr);
                    ptr = node.print<concrete_writer, fmt_buf>(writer, fdt, depth + 1);
                    break;
                }
                case MK::FDTNodeType::Prop: {
                    const auto& prop = *reinterpret_cast<const FDTProperty*>(ptr);
                    ptr = prop.print<concrete_writer, fmt_buf>(writer, fdt, depth + 1);
                    break;
                }
                case MK::FDTNodeType::EndNode:
                    return align<4>(ptr + 4);
                case MK::FDTNodeType::End:
                    return align<4>(ptr + 4);
            }
        }
    }
}

extern "C" uint32_t read_be_32(const uint8_t* bytes);
extern "C" uint64_t read_be_64(const uint8_t* bytes);
