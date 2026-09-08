#include "kernel/mem/page.hpp"
#include <cstdint>

namespace MK {

    PageDescriptor::PageDescriptor(uintptr_t descriptor): descriptor(descriptor){}
    PageDescriptor::PageDescriptor(const PageDescriptor& p): descriptor(p.descriptor){}

    PageDescriptor& PageDescriptor::operator=(uint64_t descriptor){
        this->descriptor = descriptor;
        return *this;
    }
    PageDescriptor& PageDescriptor::operator=(const PageDescriptor& p){
        this->descriptor = p.descriptor;
        return *this;
    }
    PageDescriptor::operator uint64_t() const {
        return get_addr();
    }

    PageDescriptor PageDescriptor::make_table(PageDescriptor* descriptor) {
        return make_table(reinterpret_cast<uint64_t>(descriptor));
    }
    PageDescriptor PageDescriptor::make_table(uintptr_t addr){
        return (addr & 0x7ffffffff000) 
                | PageInfo::VALID 
                | PageInfo::TABLE;
    }
    PageDescriptor PageDescriptor::make_page(uintptr_t addr, uint64_t mem_type, uint64_t ap){
        return (addr & 0x7ffffffff000)
                | PageInfo::VALID
                | PageInfo::AF
                | (mem_type << 2)
                | (ap << 6);
    }

    uintptr_t PageDescriptor::get_addr() const { return descriptor; }
    bool PageDescriptor::is_valid() const { return descriptor & 0b11; }
    bool PageDescriptor::is_fault() const { return !(descriptor & 0b11); }
    bool PageDescriptor::is_table() const { return (descriptor & 0b11) == 0b11; }
    bool PageDescriptor::is_block() const { return (descriptor & 0b11) == 0b01; }
}