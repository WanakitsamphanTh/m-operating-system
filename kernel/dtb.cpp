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

    mstd::maybe<const FDTHeader&> FDT::find_node_prefix(const char* node_name) const {
        auto ptr = this->base_ptr + this->struct_off;
        auto len = strlen(node_name);
        while(true){
            auto tag = static_cast<MK::FDTNodeType>(
                static_cast<uint32_t>(
                    *reinterpret_cast<be_uint32_t*>(ptr)
                )
            );
            switch(tag){
                case FDTNodeType::BeginNode:{
                    const auto& node = *reinterpret_cast<MK::FDTHeader*>(ptr);
                    const char* name = node.name;
                    auto _len = mstd::strlen(name);

                    if(memcmp(node_name, name, len) == 0 
                        && (name[len] == '\0' || name[len] == '@'))
                            return mstd::some<const FDTHeader&>(node);

                    ptr += sizeof(MK::FDTHeader) + _len + 1;
                    ptr = align<4>(ptr);
                    break;
                }
                case FDTNodeType::EndNode:
                    ptr += 4;
                    break;
                case FDTNodeType::Nop:
                    ptr += 4;
                    break;
                case FDTNodeType::Prop: {
                    auto& node = *reinterpret_cast<MK::FDTProperty*>(ptr);
                    ptr += sizeof(MK::FDTProperty) + node.len;
                    ptr = align<4>(ptr);
                    break;
                }
                case FDTNodeType::End:
                    return mstd::nothing;
                default:
                    return mstd::nothing;
            }
        }
    }

    mstd::maybe<const FDTProperty&> FDTHeader::search_property(const char* name, FDT& fdt) const {
        const uint8_t* strptr = fdt.base_ptr + fdt.string_off;
        MK::FDTNodeType tag = static_cast<MK::FDTNodeType>(static_cast<uint32_t>(this->tag));

        if(tag != FDTNodeType::BeginNode)
            return mstd::nothing;
        auto ptr = align<4>(reinterpret_cast<const uint8_t*>(this) + sizeof(FDTHeader) + strlen(this->name) + 1);
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