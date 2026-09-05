#include "kernel/dtb.hpp"
#include "mstd/string.hpp"
#include "mstd/monadic/maybe.hpp"
#include "kernel_core.hpp"
#include <cstdint>
#include <cstring>

extern "C" uint32_t read_be_32(const uint8_t* bytes) {
    return (uint32_t(bytes[0]) << 24)
        | (uint32_t(bytes[1]) << 16)
        | (uint32_t(bytes[2]) << 8)
        | uint32_t(bytes[3]);
}

extern "C" uint64_t read_be_64(const uint8_t* bytes){
    return (uint64_t(bytes[0]) << 46)
        | (uint64_t(bytes[1]) << 48)
        | (uint64_t(bytes[2]) << 40)
        | (uint64_t(bytes[3]) << 32)
        | (uint64_t(bytes[4]) << 24)
        | (uint64_t(bytes[5]) << 16)
        | (uint64_t(bytes[6]) << 8)
        | uint64_t(bytes[7]);
}

be_uint64_t::operator uint64_t() const{
    return read_be_64(this->bytes);
} 

uint64_t be_uint64_t::to_le() const {
    return read_be_64(this->bytes);
}

be_uint32_t::operator uint32_t() const{
    return read_be_32(this->bytes);
}
uint32_t be_uint32_t::to_le() const{
    return read_be_32(this->bytes);
}

namespace MK {
    using mstd::memcmp;
    using mstd::strchr;

    const char* FDTProperty::get_name(FDT& fdt) const { 
        return reinterpret_cast<const char*>(fdt.base_ptr) 
                + fdt.string_off 
                + name_off; 
    }
    const char* FDTProperty::as_string() const { return data; }

    mstd::maybe<uint32_t> FDTProperty::u32_at(size_t off) const { 
        if(off + 4 > len) return mstd::nothing;
        return mstd::some<uint32_t>(read_be_32(reinterpret_cast<const uint8_t*>(data + off))); 
    } 

    mstd::maybe<uint32_t> FDTProperty::u64_at(size_t off) const { 
        if(off + 8 > len) return mstd::nothing;
        return mstd::some<uint32_t>(read_be_64(reinterpret_cast<const uint8_t*>(data + off))); 
    } 

    mstd::maybe<FDT> FDT::try_read_fdt(uint8_t* fdt){
        if(!fdt || read_be_32(fdt) != FDT::magic_number) 
            return mstd::nothing;
        return mstd::some<FDT>(
            FDT{
                .base_ptr = fdt,
                .total_size = read_be_32(fdt + 0x04),
                .struct_off = read_be_32(fdt + 0x08),
                .string_off = read_be_32(fdt + 0x0c),
                .rsv_map_off = read_be_32(fdt + 0x10),
                .version = read_be_32(fdt + 0x14),
                .last_comp_version = read_be_32(fdt + 0x18),
                .phy_cpu_id = read_be_32(fdt + 0x1c),
                .string_size = read_be_32(fdt + 0x20),
                .struct_size = read_be_32(fdt + 0x24)
            }
        );
    }

     mstd::maybe<const FDTNode&> FDT::find_node(const char* path_name) const {
        return find_node(path_name, false);
    }
    mstd::maybe<const FDTNode&> FDT::find_node_prefix(const char* path_name) const{
        return find_node(path_name, true);
    }

    mstd::maybe<const FDTNode&> FDT::find_node(const char* path_name, bool as_prefix) const {
        const auto& root = *reinterpret_cast<const FDTNode*>(this->struct_off + this->base_ptr);
        if(*path_name == '/') path_name++;
        if(*path_name == '\0') return mstd::some<const FDTNode&>(root);
        else return root.find_node(path_name, as_prefix);
    }

    mstd::maybe<const FDTNode&> FDTNode::find_node(const char* path_name) const {
        return this->find_node(path_name, false);
    }
    mstd::maybe<const FDTNode&> FDTNode::find_node_prefix(const char* path_name) const{
        return this->find_node(path_name, true);
    }

    mstd::maybe<const FDTNode&> FDTNode::find_node(const char* node_name, bool as_prefix) const {
        auto ptr = reinterpret_cast<const uint8_t*>(this);
        ptr += sizeof(FDTNode) + strlen(this->name) + 1;
        ptr = align<4>(ptr);    /* move towards the header*/

        auto next = strchr(node_name, '/');
        auto len = next ? next - node_name : strlen(node_name);
        bool terminal = next == nullptr;

        while(true){
            auto tag = static_cast<MK::FDTNodeType>(
                static_cast<uint32_t>(
                    *reinterpret_cast<const be_uint32_t*>(ptr)
                )
            );
            switch(tag){
                case FDTNodeType::BeginNode: {
                    const auto& node = *reinterpret_cast<const MK::FDTNode*>(ptr);
                    auto _len = mstd::strlen(node.name);

                    bool match;
                    if(terminal){
                        // terminal component
                        if(as_prefix)
                            match = _len >= len
                                    && memcmp(node_name, node.name, len) == 0
                                    && (node.name[len] == '\0' || node.name[len] == '@');
                        else match = _len == len && memcmp(node_name, node.name, len) == 0;
                    } else {
                        // non-terminal component
                        match = _len == len && memcmp(node_name, node.name, len) == 0;
                    }
                    if(match && terminal) return mstd::some<const FDTNode&>(node);

                    /* if the parent nodes match */
                    if(match) return node.find_node(next + 1, as_prefix);
                    else ptr = node.skip();

                    break;
                }
                case FDTNodeType::Nop:
                    ptr += 4;
                    break;
                case FDTNodeType::Prop: {
                    auto& node = *reinterpret_cast<const MK::FDTProperty*>(ptr);
                    ptr += sizeof(MK::FDTProperty) + node.len;
                    ptr = align<4>(ptr);
                    break;
                }
                default:
                    return mstd::nothing;
            }
        }
    }

    const uint8_t* FDTNode::skip() const {
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
                case FDTNodeType::BeginNode: {
                    const auto& node = *reinterpret_cast<const MK::FDTNode*>(ptr);
                    node.skip();
                    break;
                }
                case FDTNodeType::Nop:
                    ptr += 4;
                    break;
                case FDTNodeType::End:
                case FDTNodeType::EndNode:
                    ptr += 4;
                    return ptr;
                    break;
                case FDTNodeType::Prop:{
                    auto& node = *reinterpret_cast<const MK::FDTProperty*>(ptr);
                    ptr += sizeof(MK::FDTProperty) + node.len;
                    ptr = align<4>(ptr);
                    break;
                }
            }
        }
    }

    mstd::maybe<const FDTProperty&> FDTNode::find_property(const char* name, FDT& fdt) const {
        const uint8_t* strptr = fdt.base_ptr + fdt.string_off;
        MK::FDTNodeType tag = static_cast<MK::FDTNodeType>(static_cast<uint32_t>(this->tag));

        if(tag != FDTNodeType::BeginNode)
            return mstd::nothing;
        auto ptr = align<4>(reinterpret_cast<const uint8_t*>(this) + sizeof(FDTNode) + strlen(this->name) + 1);
        while(true){
            tag =  static_cast<MK::FDTNodeType>(
                static_cast<uint32_t>(
                    *reinterpret_cast<const be_uint32_t*>(ptr)
                )
            );

            switch(tag){
                case FDTNodeType::Prop: {
                    const auto& prop = *reinterpret_cast<const FDTProperty*>(ptr);
                    auto prop_name = reinterpret_cast<const char*>(strptr) + prop.name_off;
                    if(strcmp(name, prop_name) == 0)
                        return mstd::some<const FDTProperty&>(prop);
                    ptr = align<4>(ptr + sizeof(FDTProperty) + prop.len);
                    break;
                }
                case FDTNodeType::Nop:
                    ptr += 4;
                    break;
                default:
                    return mstd::nothing;
            }
        }
    }
}