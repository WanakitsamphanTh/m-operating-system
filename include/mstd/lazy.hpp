#pragma once
#include <tuple>
#include <functional>
#include <utility>
#include <type_traits>

namespace mstd {
    using std::forward_as_tuple;
    using std::move;
    using std::is_convertible_v;
    using std::is_constructible_v;

    template<class T, class Fn>
    class lazy {
        mutable union storage_t {
            T value;
            Fn fn;
            storage_t() {}
            ~storage_t(){}
        } storage;
        mutable bool initialized = false;

        void initialize() const {
            if (!initialized) {
                Fn fn = std::move(storage.fn);
                storage.fn.~Fn();
                fn(&storage.value);
                initialized = true;
            }
        }

    public:
        explicit lazy(Fn&& fn) : initialized(false) {
            new(&storage.fn) Fn(std::forward<Fn>(fn));
        }

        lazy(const lazy&) = delete;
        lazy(lazy&& other) noexcept : initialized(false) {
            if (other.initialized) {
                new(&storage.value) T(std::move(other.storage.value));
                initialized = true;
            } else {
                new(&storage.fn) Fn(std::move(other.storage.fn));
            }
        }

        bool is_ready() const {
            return initialized;
        }

        T& get() const {
            initialize();
            return storage.value;
        }


        
        template<class U> requires (is_constructible_v<U, T>)
        operator U() const {
            return get();
        }

        template<class U> requires (is_convertible_v<U&, T&>)
        operator U&() {
            return get();
        }

        template<class U> requires (is_convertible_v<U&, T&>)
        operator const T&() const {
            return get();
        }

        template<class U> requires (is_convertible_v<U&&, T&&>)
        operator U&&() && {
            return std::move(get());
        }

        const T& operator *() const {
            return get();
        }

        T& operator *() {
            return get();
        }

        const T* operator ->() const {
            return &get();
        }

        T* operator ->() {
            return &get();
        }

        ~lazy() {
            if (initialized) {
                storage.value.~T();
            } else {
                storage.fn.~Fn();
            }
        } 
    };

    template<class T, class Fn>
    class lazy<T&, Fn> {
        mutable union storage_t {
            T* ref;
            Fn fn;
            storage_t() {}
            ~storage_t(){}
        } storage;
        mutable bool initialized = false;

        void initialize() const {
            if (!initialized) {
                Fn fn = std::move(storage.fn);
                storage.fn.~Fn();
                fn(&storage.ref);
                initialized = true;
            }
        }

    public:
        explicit lazy(Fn&& fn) : initialized(false) {
            new(&storage.fn) Fn(std::forward<Fn>(fn));
        }

        lazy(const lazy&) = delete;
        lazy(lazy&& other) noexcept : initialized(false) {
            if (other.initialized) {
                storage.ref = other.storage.value;
                initialized = true;
            } else {
                new(&storage.fn) Fn(std::move(other.storage.fn));
            }
        }

        bool is_ready() const {
            return initialized;
        }

        T& get() const {
            initialize();
            return *storage.ref;
        }
        
        template<class U> requires (is_convertible_v<U&, T&>)
        operator U&() {
            return get();
        }

        template<class U> requires (is_convertible_v<const U&, const T&>)
        operator const T&() const {
            return get();
        }

        const T& operator *() const {
            return get();
        }

        T& operator *() {
            return get();
        }

        const T* operator ->() const {
            return &get();
        }

        T* operator ->() {
            return &get();
        }

        ~lazy() {
            if (!initialized) 
                storage.fn.~Fn();
        }
    };

    template<class T>
    class get_immediate;

    template<class T>
    class get_immediate {
        using type = T;
    };
    
    template<class T, class Fn>
    class get_immediate<lazy<T,Fn>> {
        using type = T;
    };

    template<class T>
    bool is_lazy = false;
    template<class T, class Fn>
    bool is_lazy<lazy<T,Fn>> = true;

    template<class T>
    using get_immediate_t = get_immediate<T>::type;

    template<class T, class... Args>
    auto make_lazy(Args&&... args) {
        auto fn = [...args = std::forward<Args>(args)](T* slot) {
            new(slot) T(std::move(args)...);
        };
        return lazy<T, decltype(fn)>(std::move(fn));
    }

    template<class Fn, class ...Args>
        requires (
            std::is_invocable_v<Fn, Args...>
            && !std::is_void_v<std::invoke_result_t<Fn, Args...>> 
            && !std::is_reference_v<std::invoke_result_t<Fn, Args...>>
        )
    auto make_lazy_call(Fn&& f, Args&&... args){
        using T = std::invoke_result_t<Fn, Args&&...>;
        auto fn 
        = [f = std::forward<Fn>(f), 
            ...args= std::forward<Args>(args)]
            (T* slot) mutable {
            new(slot) T(std::invoke(std::move(f), std::move(args)...));
        };
        return lazy<T, decltype(fn)>(std::move(fn));
    }
}