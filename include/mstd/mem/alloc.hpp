#pragma once
#include <cstddef>
#include <type_traits>
#include "mstd/monadic/maybe.hpp>"

namespace mstd{
    template<class T>
    concept allocator_t =
        requires (T t, size_t size) {
            { t.alloc(size) } -> std::same_as<maybe<void*>>;
            { t.free() };
        };

    class allocator_interface {
    protected:
        void* mem;
        size_t size;
        bool extend(size_t);
    public:
        virtual maybe<void*> alloc(size_t) = 0;
        virtual void free(void*) = 0;
        virtual ~allocator_interface();
    };

    template<typename T>
    class pool_allocator : public allocator_interface {
    public:
        virtual maybe<void*> alloc(size_t) override;
        virtual maybe<T*> alloc_one();
        virtual void free(void*) override;
        ~pool_allocator();
    };

    class arena_allocator : public allocator_interface {
    public:
        ~arena_allocator();
    };
    
    class FreeListAllocator : public arena_allocator {
    public:
        ~FreeListAllocator();
    };

    class global_allocator {
        static global_allocator& alloc;
    public:
        maybe<void*> alloc(size_t) = 0;
        void free(void*) = 0;
    };

    template<class T, allocator_t alloc_t, class Args...>
        requires std::is_constructible_v<T, Args...>
    T* make_new(alloc_t& alloc, Args... args);
}