#pragma once
#include <cstddef>
#include <type_traits>
#include <cstdint>
#include "mstd/monadic/maybe.hpp"

namespace mstd{
    template<class T>
    concept allocator_t =
        requires (T& t, size_t size, size_t align, void* ptr) {
            { t.alloc(size,align) } -> std::same_as<maybe<void*>>;
            { t.dealloc(ptr) };
        };

    class allocator_interface {
    public:
        virtual maybe<void*> alloc(size_t size, size_t align = 0) = 0;
        virtual void dealloc(void*) = 0;
        virtual ~allocator_interface() = default;
    };

    template<typename T>
    class pool_allocator {
        T* pool;
        uint8_t* bitmap;
    public:
        pool_allocator(size_t pool_size);
        template<class... Args>
        maybe<T*> alloc_one(Args&&...);
        void dealloc(T*);
        ~pool_allocator();
    };

    class arena_allocator : public allocator_interface {
    public:
        virtual maybe<void*> alloc(size_t size, size_t align = 0) override;
        virtual void dealloc(void*) override;
        ~arena_allocator();
    };
    
    class free_list_allocator : public allocator_interface {
    public:
        virtual maybe<void*> alloc(size_t size, size_t align = 0) override;
        virtual void dealloc(void*) override;
        ~free_list_allocator();
    };

    allocator_interface& get_global_allocator();

    template<class T, allocator_t alloc_t, class... Args>
        requires std::is_constructible_v<T, Args...>
    maybe<T*> make_new(alloc_t& alloc, Args&&... args);
}