#pragma once
#include <cstddef>
#include <type_traits>
#include <cstdint>
#include "mstd/monadic/maybe.hpp"

namespace mstd{
    struct heap_buffer {
        void* ptr;
        size_t size;
        enum class buffer_tag { static_buffer, extensible_os, non_extensible_os };
        buffer_tag tag;
    public:
        heap_buffer();
        heap_buffer(heap_buffer&&);
        static heap_buffer from_static(void* ptr, size_t size) {
            heap_buffer buf;
            buf.ptr = ptr;
            buf.size = size,
            buf.tag = buffer_tag::static_buffer;
            return buf;
        }
        static heap_buffer from_map(void*, size_t);
        void clear();
        bool extend();
        bool include();
        ~heap_buffer();
    };

    template<class T>
    class allocator_interface;

    template<class T>
    concept allocator_t =
        requires (T& t, size_t size, size_t align, void* ptr) {
            { t.alloc(size,align) } -> std::same_as<maybe<void*>>;
            { t.dealloc(ptr) };
        };

    template<class T>
    class allocator_interface {
    public:
        template<class U, class... Args>
        maybe<U*> alloc_as(size_t align, Args&&... args) {
            auto ptr = reinterpret_cast<U*>(static_cast<T*>(this)->alloc(sizeof(U), align));
            if(ptr == nullptr) 
                return nothing;
            new(ptr) U(std::forward<Args>(args)...);
            return some<U*>(ptr);
        }

        template<class U>
            requires std::is_default_constructible_v<U>
        maybe<U*> alloc_array(size_t len, size_t align) {
            auto arr = reinterpret_cast<U*>(static_cast<T*>(this)->alloc(sizeof(U) * len, align));
            if(arr == nullptr)
                return nothing;
            for(auto i = 0; i < len; i++)
                new(arr + i) U();
            return some<U*>(arr);
        }

        template<class U, class Fn>
        maybe<U*> alloc_array_fn(size_t len, size_t align, Fn&& fn) {
            auto arr = reinterpret_cast<U*>(static_cast<T*>(this)->alloc(sizeof(U) * len, align));
            if(arr == nullptr)
                return nothing;
            for(auto i = 0; i < len; i++)
                new(arr + i) U(fn());
            return some<U*>(arr);
        }

        template<class U>
        void dealloc_one(U* ptr){
            ptr->~U();
            static_cast<T*>(this)->dealloc(ptr);
        }

        template<class U, class Fn>
        void dealloc_one_with_deleter(U* ptr, Fn&& fn){
            fn(ptr);
            static_cast<T*>(this)->dealloc(ptr);
        }

        template<class U>
        void dealloc_array(U* arr, size_t len){
            for(auto i = 0; i < len; i++)
                arr[i].~U();
            static_cast<T*>(this)->dealloc(arr);
        }

        template<class U, class Fn>
        void dealloc_array_with_deleter(U* arr, Fn&& fn){
            fn(arr);
            static_cast<T*>(this)->dealloc(arr);
        }
    };

    class dyn_allocator;

    template<class T>
    concept concrete_allocator 
        = allocator_t<T> && !std::is_same_v<dyn_allocator, std::remove_cvref_t<T>>;

    class dyn_allocator: public allocator_interface<dyn_allocator> {
        using any = void*;
        struct vtable {
            maybe<void*> (*alloc)(any, size_t, size_t);
            void (*dealloc)(any, void*) = 0;
        };

        template<class T>
        static maybe<void*> impl_alloc(any allocator, size_t size, size_t align){
            return reinterpret_cast<T*>(allocator)->alloc(size, align);
        }
        template<class T>
        static void impl_dealloc(any allocator, void* ptr){
            return reinterpret_cast<T*>(allocator)->dealloc(ptr);
        }

        template<concrete_allocator T>
        static const vtable* get_vtable(){
            static const vtable vt {
                .alloc = &dyn_allocator::impl_alloc<T>,
                .dealloc = &dyn_allocator::impl_dealloc<T>
            };
            return &vt;
        }

        any allocator;
        const vtable* vptr;
    public:
        dyn_allocator(): vptr(nullptr), allocator(nullptr){}
        template<concrete_allocator alloc_t>
        dyn_allocator(alloc_t& alloc)
            : vptr(get_vtable<alloc_t>()),
            allocator(&alloc){}
        dyn_allocator(const dyn_allocator& al)
            : vptr(al.vptr), allocator(al.allocator){}
        
        dyn_allocator& operator=(const dyn_allocator& other){
            vptr = other.vptr;
            allocator = other.allocator;
            return *this;
        }
        template<concrete_allocator alloc_t>
        dyn_allocator& operator=(alloc_t& al){
            vptr = get_vtable<alloc_t>();
            allocator = &al;
        }

        maybe<void*> alloc(size_t size, size_t align = 0){
            if(vptr == nullptr || allocator == nullptr)
                return nothing;
            return vptr->alloc(allocator, size, align);
        }

        void dealloc(void* ptr) {
            if(vptr == nullptr || allocator == nullptr)
                return;
            vptr->dealloc(allocator, ptr);
        }

        maybe<void*> alloc_unchecked(size_t size, size_t align = 0){
            return vptr->alloc(allocator, size, align);
        }

        void dealloc_unchecked(void* ptr) {
            vptr->dealloc(allocator, ptr);
        }

        /*
        caution: type identity is valid only within the same linkage unit
                and downcasting is usually discouraged.
        */
        template<concrete_allocator T>
        bool is() const { return this->vptr == get_vtable<T>(); }
        template<concrete_allocator T>
        maybe<T&> downcast() { 
            if(is<T>()) return some<T&>(*reinterpret_cast<T*>(allocator));
            else return nothing;
        }

    };
    dyn_allocator& get_global_allocator();

    class arena_allocator: public allocator_interface<arena_allocator>{
        heap_buffer buffer;
        uint8_t* start;
        uint8_t* end;
        uint8_t* brk;
        uint32_t prev_size = 0;
    public:
        arena_allocator();
        arena_allocator(heap_buffer&&);
        arena_allocator(arena_allocator&&);
        maybe<void*> alloc(size_t, size_t);
        void dealloc(void*);
        ~arena_allocator();
    };

    struct mem_header_t {
        uint32_t size;      // 4 bytes
        uint32_t prev_size; // 4 bytes
        bool occupied;      // 1 bytes

        void set_header(uint32_t size, uint32_t prev_size, bool occupied);
        void set_occupied();
        void set_unoccupied();
        static mem_header_t* from_payload(void* ptr);
    };
    static_assert(sizeof(mem_header_t) == 12, "sizeof(mem_header_t) is not what you expected");

    struct free_list_node {
        mem_header_t header;
        free_list_node* next;
    };

    struct tree_link {
        tree_link* l;
        tree_link* r;
    };

    struct free_tree_node {
        mem_header_t header;
        tree_link* by_addr;
        tree_link* by_size;
    };

    struct free_cache_t {
        free_list_node* free_list[256];
        free_tree_node* large_root_addr;
        free_tree_node* large_root_size;
    };

    /*
    free_list_allocator: 
        memory block:   | header | pad | header_offset (uint32_t) | payload |
        size: sizeof(header) + sizeof(uint32_t) + pad size + payload size
    */

    class free_list_allocator: public allocator_interface<free_list_allocator> {
        heap_buffer buffer;
        free_cache_t freed;
        void* brk;
        void* start;
        void* end;

        maybe<void*> acquire_freed(size_t, size_t);
    public:
        free_list_allocator();
        free_list_allocator(heap_buffer&&);
        free_list_allocator(free_list_allocator&&);
        maybe<void*> alloc(size_t, size_t);
        void dealloc(void*); 
        ~free_list_allocator();
    };

    template<class T>
    class object_pool {
        heap_buffer buffer;
        T* start;
        struct bitmap_t {
            bitmap_t* next;
            size_t size;
            uint8_t bitmap[];
        }* bitmap;
        maybe<T*> acquire(){
            auto bitmap = this->bitmap;
            while(bitmap){
                uint8_t* ptr = bitmap->bitmap;
                uint8_t* end = bitmap->bitmap + bitmap->size;
                while(ptr){
                    if(*ptr == 0xff) {
                        ptr++;
                        continue;
                    }
                    for(auto i = 0; i < 8; i++){
                        auto bit = ((*ptr) >> i) & 1;
                        if(bit == 0) {
                            *ptr = *ptr | (1 << i);
                            return some<T*>(start + i);
                        }
                    }
                }
                bitmap = bitmap->next;
            }
            return nothing;
        }
        maybe<T*> acquire(size_t len){
            return nothing;
        }
    public:
        object_pool();
        object_pool(heap_buffer&&);
        object_pool(object_pool&&);
        template<class... Args>
        maybe<T*> alloc_one(Args&&... args){
            return acquire().then([&args...](T* ptr) mutable {
                new(ptr) T(std::forward<Args>(args)...);
                return ptr;
            });
        }
        template<class U = T>
            requires std::is_default_constructible_v<T>
        maybe<T*> alloc_array(size_t len){
            return acquire(len).then([len](T* ptr) mutable {
                for(size_t i = 0; i < len; i++)
                    new(ptr) T;
                return ptr;
            });
        }
        template<class Fn>
        maybe<T*> alloc_array_fn(size_t len, Fn&& fn){
            return acquire(len).then([len, &fn](T* ptr) mutable {
                for(size_t i = 0; i < len; i++)
                    new(ptr) T(fn());
                return ptr;
            });
        }
        void dealloc(T* ptr){
            ptr->~T();
        }
        void dealloc_array(T* arr, size_t len){
            for(auto i = 0; i < len; i++)
                arr[i].~T();

        }
        ~object_pool(){

        }
    };

    template<class T, allocator_t alloc_t, class... Args>
        requires std::is_constructible_v<T, Args...>
    maybe<T*> make_new(alloc_t& alloc, Args&&... args){}
    template<class T, class... Args>
        requires std::is_constructible_v<T, Args...>
    maybe<T*> make_new(object_pool<T>& alloc, Args&&... args){}
}