#include "kernel/mem/page.hpp"
#include "kernel/mem/allocator.hpp"
#include "kernel/mem/mem.hpp"
#include "kernel_core.hpp"

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <malloc.h>

namespace MK {
    template<size_t entry_size>
    bool is_mappable(uintptr_t base){
        return is_align<entry_size>(base);
    };

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
                | PageInfo::PAGE
                | PageInfo::AF
                | (mem_type << 2)
                | (ap << 6);
    }

    PageDescriptor PageDescriptor::make_block(uintptr_t addr, uint64_t mem_type, uint64_t ap){
        return (addr & 0x7ffffffff000)
                | PageInfo::VALID
                | PageInfo::BLOCK
                | PageInfo::AF
                | (mem_type << 2)
                | (ap << 6);
    }

    uintptr_t PageDescriptor::get_addr() const { return descriptor & 0x7ffffffff000; }
    bool PageDescriptor::is_valid() const { return descriptor & PageInfo::VALID; }
    bool PageDescriptor::is_fault() const { return !(descriptor & 0b11); }
    bool PageDescriptor::is_table() const { return (descriptor & 0b11) == 0b11; }
    bool PageDescriptor::is_block() const { return (descriptor & 0b11) == 0b01; }

    PageTablesManager::PageTablesManager(): mmu_enabled(false){}

    void PageTablesManager::init(PageAlloc& alloc){
        mmu_enabled = false;
        allocator = &alloc;
        memset(this->l0_table, 0, page_size);
        //map(reinterpret_cast<uintptr_t>(this), sizeof(PageTablesManager), MapMode::Id, PageInfo::MemoryType::Normal, PageInfo::AP::PRW);
    }

    void PageTablesManager::relink(PageAlloc& alloc){
        allocator = &alloc;
    }

    void PageTablesManager::map(uintptr_t base, size_t size, MapMode mode, uint64_t mem_type, uint64_t ap){
        auto end = align<page_size>(base + size);
        base = align_down<page_size>(base);
        size = end - base;
        if(mode == MapMode::Id){
            for(auto off = 0ull; off < size; off += page_size)
                this->allocator->reserve_page_at(base + off);
        }
        map_l0(base, size, mode, mem_type, ap);
    }

    void PageTablesManager::map_l0(uintptr_t base, size_t size, MapMode md, uint64_t mem_type, uint64_t ap){
        uintptr_t end = base + size;
        auto tb = this->l0_table;
        while(base < end){
            auto ind = (base >> 39) & 0x1ff;
            uintptr_t chunk_end = std::min(align<l0_entry_size>(base + 1), end);
            size_t chunk_size = chunk_end - base;
            PageDescriptor* l1_tb;
            if(!tb[ind].is_valid()) { 
                // create a new table
                auto l1_pa = this->allocator->alloc_page().take();
                l1_tb = mmu_enabled ? 
                    phy2virt_as<PageDescriptor*>(l1_pa) 
                    : reinterpret_cast<PageDescriptor*>(l1_pa);
                memset(l1_tb, 0, page_size);
                tb[ind] = PageDescriptor::make_table(l1_pa);
            } else {
                auto l1_pa = tb[ind].get_addr();
                l1_tb = mmu_enabled ? 
                    phy2virt_as<PageDescriptor*>(l1_pa) 
                    : reinterpret_cast<PageDescriptor*>(l1_pa);
            }
            // map page
            map_l1(l1_tb, base, chunk_size, md, mem_type, ap);
            base = chunk_end;
        }
    }

    __attribute__((optimize("no-jump-tables")))
    void PageTablesManager::map_l1(PageDescriptor* tb, uintptr_t base, size_t size, MapMode md, uint64_t mem_type, uint64_t ap){
        uintptr_t end = base + size;
        while(base < end){
            auto ind = (base >> 30) & 0x1ff;
            uintptr_t chunk_end = std::min(align<l1_block_size>(base + 1), end);
            size_t chunk_size = chunk_end - base;
            if(md != MapMode::New && chunk_size == l1_block_size 
                && is_align<l1_block_size>(base) && !tb[ind].is_valid()){
                    uintptr_t addr;
                    switch(md){
                        using enum MapMode;
                        case Direct:
                            addr = virt2phy(reinterpret_cast<void*>(base));
                            break;
                        case DirectKernel:
                            addr = kvirt2phy(reinterpret_cast<void*>(base));
                            break;
                        default:
                            addr = base;
                    }
                    tb[ind] = PageDescriptor::make_block(addr, mem_type, ap);
            } else {
                PageDescriptor* l2_tb;
                 if(!tb[ind].is_valid()) { 
                // create a new table
                    auto l2_pa = this->allocator->alloc_page().take();
                    l2_tb = mmu_enabled ? 
                        phy2virt_as<PageDescriptor*>(l2_pa) 
                        : reinterpret_cast<PageDescriptor*>(l2_pa);
                    memset(l2_tb, 0, page_size);
                    tb[ind] = PageDescriptor::make_table(l2_pa);
                } else {
                    auto l2_pa = tb[ind].get_addr();
                    l2_tb = mmu_enabled ? 
                        phy2virt_as<PageDescriptor*>(l2_pa) 
                        : reinterpret_cast<PageDescriptor*>(l2_pa);
                }
                // map page
                map_l2(l2_tb, base, chunk_size, md, mem_type, ap);
            }
            base = chunk_end;
        }
    }

    __attribute__((optimize("no-jump-tables")))
    void PageTablesManager::map_l2(PageDescriptor* tb, uintptr_t base, size_t size, MapMode md, uint64_t mem_type, uint64_t ap){
        uintptr_t end = base + size;
        while(base < end){
            auto ind = (base >> 21) & 0x1ff;
            uintptr_t chunk_end = std::min(align<l2_block_size>(base + 1), end);
            size_t chunk_size = chunk_end - base;
            if(md != MapMode::New && chunk_size == l2_block_size 
                && is_align<l2_block_size>(base) && !tb[ind].is_valid()){
                    uintptr_t addr;
                    switch(md){
                        using enum MapMode;
                        case Direct:
                            addr = virt2phy(reinterpret_cast<void*>(base));
                            break;
                        case DirectKernel:
                            addr = kvirt2phy(reinterpret_cast<void*>(base));
                            break;
                        default:
                            addr = base;
                    }
                    tb[ind] = PageDescriptor::make_block(addr, mem_type, ap);
            } else {
                PageDescriptor* l3_tb;
                if(!tb[ind].is_valid()) { 
                // create a new table
                    auto l3_pa = this->allocator->alloc_page().take();
                    l3_tb = mmu_enabled ? 
                        phy2virt_as<PageDescriptor*>(l3_pa) 
                        : reinterpret_cast<PageDescriptor*>(l3_pa);
                    memset(l3_tb, 0, page_size);
                    tb[ind] = PageDescriptor::make_table(l3_pa);
                } else {
                    auto l3_pa = tb[ind].get_addr();
                    l3_tb = mmu_enabled ? 
                        phy2virt_as<PageDescriptor*>(l3_pa) 
                        : reinterpret_cast<PageDescriptor*>(l3_pa);
                }
                // map page
                map_l3(l3_tb, base, chunk_size, md, mem_type, ap);
            }
            base = chunk_end;
        }
    }

    __attribute__((optimize("no-jump-tables")))
    void PageTablesManager::map_l3(PageDescriptor* tb, uintptr_t base, size_t size, MapMode md, uint64_t mem_type, uint64_t ap){
        uintptr_t end = base + size;
        while(base < end){
            auto ind = (base >> 12) & 0x1ff;
            uintptr_t chunk_end =align<page_size>(base + 1);
            //if(tb[ind].is_valid()) mstd::panic("the entry is already mapped!");
            uintptr_t addr;
            switch(md){
                using enum MapMode;
                case Id:
                    addr = base;
                    break;
                case Direct:
                    addr = virt2phy(reinterpret_cast<void*>(base));
                    break;
                case DirectKernel:
                    addr = kvirt2phy(reinterpret_cast<void*>(base));
                    break;
                default:
                    addr = this->allocator->alloc_page().take();
            }
            tb[ind] = PageDescriptor::make_page(addr, mem_type, ap);
            base = chunk_end;
        }
    }

    void PageTablesManager::enable_mmu(){
        mmu_enabled = true;
        MK::enable_mmu(MK::MAIR::value, MK::TCR::value, this->l0_table);
    }

    void PageTablesManager::map(void* addr, size_t size, uint64_t mem_type, uint64_t ap){
        
    }
    void PageTablesManager::remap(void* addr, size_t old_size, size_t size, uint64_t mem_type, uint64_t ap){

    }
    void PageTablesManager::unmap(uintptr_t addr, size_t size){

    }
}