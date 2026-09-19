#pragma once

#include <cstdint>
#include <type_traits>
#include <utility>
#include "mstd/monadic/maybe.hpp"

namespace mstd {
    template<uint32_t index, class... Ts>
    union polymorph_storage;

    template<uint32_t index, class... Ts>
    class indexed_type_list;

    template<class Ty, class... Ts>
    struct is_in;

    template<class Ty, class T0, class T1, class... Ts>
    struct is_in <Ty, T0, T1, Ts...> {
        inline static constexpr bool value = std::is_same_v<Ty, T0> || is_in<Ty, T1, Ts...>::value;
    };

    template<class Ty, class T0>
    struct is_in <Ty, T0> {
        inline static constexpr bool value = std::is_same_v<Ty, T0>;
    };

    template<class Ty, class... Ts>
    inline constexpr bool is_in_v = is_in<Ty, Ts...>::value;

    typedef void (*destructor_fn_t)(void*);
    template<class T>
    void type_erased_destructor(void* ptr){
        reinterpret_cast<T*>(ptr)->~T();
    }

    typedef void (*move_fn_t)(void*, void*);
    template<class T>
    void type_erased_move(void* dst, void* src){
        new(dst) T(std::move(*reinterpret_cast<T*>(src)));
    }

    typedef void (*copy_fn_t)(void*, void*);
    template<class T>
        requires std::is_copy_constructible_v<T>
    void type_erased_copy(void* dst, const void* src){
        new(dst) T(*reinterpret_cast<const T*>(src));
    }

    template<class T, class... Ts>
    class match_result;

    template<class... Ts>
    class choice {
        using storage_t = polymorph_storage<0, Ts...>;
        inline static constexpr destructor_fn_t destroy_fn[] = {
            type_erased_destructor<Ts>...
        };
        inline static constexpr copy_fn_t copy_fn[] = {
            type_erased_copy<Ts>...
        };
        inline static constexpr move_fn_t move_fn[] = {
            type_erased_move<Ts>...
        };
        storage_t storage;
        uint32_t discriminator;
        void destroy(){
            destroy_fn[discriminator](&storage);
        }
    public:
        using type_list_t = indexed_type_list<0, Ts...>;
        choice(): discriminator(0) {
            using T = typename type_list_t::type_at<0>;
            new(&storage) T();
        }    

        template<class T>
            requires is_in_v<T, Ts...>
        choice(const T& t){
            new(&this->storage) T(t);
            this->discriminator = type_list_t::template id_of<T>();
        }

        template<class T>
            requires is_in_v<T, Ts...>
        choice(T&& t){
            new(&this->storage) T(std::forward<T>(t));
            this->discriminator = type_list_t::template id_of<T>();
        }   

        choice(const choice& other) {
            copy_fn[other.discriminator](&this->storage, &other.storage);
            this->discriminator = other.discriminator;
        }
        choice& operator=(const choice& other){
            if(this == &other) return *this;
            move_fn[other.discriminator](&this->storage, &other.storage);
            this->discriminator = other.discriminator;
            return *this;
        }
        template<class U>
        choice& operator=(const U& src) { 
            // only works on the first matched type
            discriminator = type_list_t::template id_of<U>();
            new(&storage) U(src);
        }

        choice(choice&& other){
            move_fn[other.discriminator](&this->storage, &other.storage);
            this->discriminator = other.discriminator;
        }
        choice& operator=(choice&& other){
            if(this == &other) return this;
            destroy();
            move_fn[other.discriminator](&this->storage, &other.storage);
            this->discriminator = other.discriminator;
            return this;
        }
        template<class U>
        choice& operator=(U&& src)  { 
            // only works on the first matched type
            discriminator = type_list_t::template id_of<U>();
            new(&storage) U(std::forward<U>(src));
            return *this;
        }

        ~choice() { destroy(); }

        template<class T>
        bool is() const {
            return discriminator == type_list_t::template id_of<T>();
        }
        
        template<class T, class U>
            requires std::is_constructible_v<T, U>
        choice& set(U&& src){
            destroy();
            using Ut = std::decay_t<U>;
            auto index = type_list_t::template id_of<Ut>();
            if constexpr(std::is_rvalue_reference_v<U>)
                new(&storage) T(std::forward<Ut>(src));
            else
                new(&storage) T(src);
            discriminator = index;
            return *this;
        }

        uint32_t get_id() const {
            return discriminator;
        }

        template<uint32_t index, class U, class V = typename type_list_t::type_at<index>>
            requires std::is_constructible_v<V, U>
        choice& set_at(U&& src){
            destroy();
            using Ut = std::decay_t<U>;
            if constexpr(std::is_rvalue_reference_v<U>)
                new(&storage) V(std::forward<Ut>(src));
            else
                new(&storage) V(src);
            discriminator = index;
            return *this;
        }

        template<class U, class Fn>
        auto match(Fn&& fn){
            if constexpr(!std::is_void_v<std::invoke_result_t<Fn, U&>>){
                if(discriminator == type_list_t::template id_of<U>())
                    return match_result<U, Ts...>(
                        std::invoke(
                            std::forward<Fn>(fn),
                            *reinterpret_cast<U*>(&storage)
                        ), *this);
                else
                    return match_result<U, Ts...>(*this);
            }
            else {
                if(discriminator == type_list_t::template id_of<U>()){
                    std::invoke(
                        std::forward<Fn>(fn),
                        *reinterpret_cast<U*>(&storage)
                    );
                    return match_result<void, Ts...>(true, *this);
                }
                else
                    return match_result<void, Ts...>(false, *this);
            }
        }

        template<uint32_t index, class Fn>
        auto match_index(Fn&& fn){
            using U = type_list_t::template type_at<index>;
            if constexpr(std::is_void_v<std::invoke_result_t<Fn, U>>){
                return match_result<U, Ts...>(U{}, *this);
            }
            else {
                return match_result<void>(*this);
            }
        }

        // take
        template<class T>
        maybe<T> take_if(){
            if(discriminator == type_list_t::template id_of<T>())
                return some<T>(std::move(*reinterpret_cast<T*>(&storage)));
            else return nothing;
        }

        template<uint32_t index>
        auto take_if_at() -> maybe<typename type_list_t::type_at<index>>{
            using T = typename type_list_t::type_at<index>;
            if(discriminator == index)
                return some<T>(std::move(*reinterpret_cast<T*>(&storage)));
            else return nothing;
        }

        template<class T>
        T take_unchecked(){
            return std::move(*reinterpret_cast<T*>(&storage));
        }

        template<uint32_t index>
        auto take_unchecked_at() -> typename type_list_t::type_at<index> {
            using T = typename type_list_t::type_at<index>;
            return std::move(*reinterpret_cast<T*>(&storage));
        }

        // borrow
        template<class T>
        maybe<T&> borrow_if(){
            if(discriminator == type_list_t::template id_of<T>())
                return some<T>(*reinterpret_cast<T*>(&storage));
            else return nothing;
        }

        template<uint32_t index>
        auto borrow_if_at() -> maybe<typename type_list_t::type_at<index>&>{
            using T = typename type_list_t::type_at<index>;
            if(discriminator == index)
                return some<T&>(*reinterpret_cast<T*>(&storage));
            else return nothing;
        }

        template<class T>
        T& borrow_unchecked(){
            return *reinterpret_cast<T*>(&storage);
        }

        template<uint32_t index>
        auto borrow_unchecked_at() -> typename type_list_t::type_at<index>& {
            using T = typename type_list_t::type_at<index>;
            return *reinterpret_cast<T*>(&storage);
        }

        template<class U, class... Us> friend class match_result;
    };

    template<class T, class... Ts>
    class match_result {
        using choice = choice<Ts...>;
        using type_list_t = choice::type_list_t;
        maybe<T> result;
        choice& base;
    public:
        match_result(choice* base): base(base), result(nothing){}
        match_result(T&& result, choice& base): base(base), result(std::forward<T>(result)){}
        template<class U, class Fn>
        match_result& match(Fn&& fn){
            if(!result.is_valid() && base.discriminator == type_list_t::template id_of<U>()){
                result = std::invoke(
                    std::forward<Fn>(fn),
                    *reinterpret_cast<U*>(&base.storage)
                );
            }
            return *this;
        }
        template<class Fn>
        T otherwise(Fn&& fn){
            if(!result.is_valid())
               return std::invoke(std::forward<Fn>(fn));
            return result.take_unchecked();
        }

        template<class U>
        T or_default(U&& default_val){
            return static_cast<T>(default_val);
        }
    };

    template<class... Ts>
    class match_result<void, Ts...> {
        using choice = choice<Ts...>;
        using type_list_t = choice::type_list_t;
        bool done;
        choice& base;
    public:
        match_result(bool done, choice& base): base(base), done(done){}
        template<class U, class Fn>
        match_result& match(Fn&& fn){
            if(!done && base.discriminator == type_list_t::template id_of<U>()){
                std::invoke(
                    std::forward<Fn>(fn),
                    *reinterpret_cast<U*>(&base.storage)
                );
                done = true;
            }
            return *this;
        }
        template<class Fn>
        void otherwise(Fn&& fn){
            if(!done) std::invoke(std::forward<Fn>(fn));
        }
    };

    template<uint32_t index, class T, class... Ts>
    union polymorph_storage<index, T, Ts...>{
        inline static constexpr auto id = index;
        T val;
        using next_t = polymorph_storage<index + 1, Ts...>;
        next_t next;
    public:
        polymorph_storage(){}
        ~polymorph_storage(){}
        template<uint32_t ind, class U>
        void assign_with_index(U src) {
            using Ut = std::decay_t<U>;
            if constexpr(std::is_rvalue_reference_v<U>){
                if constexpr(ind == index) {
                    static_assert(std::is_constructible_v<T, U>, "cannot construct the object");
                    new(&this->val) T(std::forward<Ut>(src));
                } else next.template assign_with_index<ind, U>(std::forward<Ut>(src));
            } else {
                if constexpr(ind == index) {
                    static_assert(std::is_constructible_v<T, U>, "cannot construct the object");
                    new(&this->val) T(src);
                } else next.template assign_with_index<ind, U>(src);
            }
        }

        template<typename U>
        void assign_with_type(U src) {
            using Ut = std::decay_t<U>;
            if constexpr(std::is_same_v<T, Ut>) {
                if constexpr(std::is_rvalue_reference_v<U>)
                    new(&this->val) T(std::forward<Ut>(src));
                else new(&this->val) T(src);
            } else next.template assign_with_type<U>(std::forward<U>(src));
        }

        template<class U>
        static constexpr bool is_same() { return std::is_same_v<T, U>; }
        static consteval uint32_t get_id() { return index; }

    };

    template<uint32_t index, class T>
    union polymorph_storage<index, T>{
        inline static constexpr auto id = index;
        T val;
    public:
        template<uint32_t ind, class U>
        void assign_with_index(U src) {
            using Ut = std::decay_t<U>;
            static_assert(ind == index, "type index out of range");
            static_assert(std::is_constructible_v<T, U>, "cannot construct the object");
            new(&this->val) T(std::forward(src));
            if constexpr(std::is_rvalue_reference_v<U>){
                new(&this->val) T(std::forward(src));
            } else {
                new(&this->val) T(src);
            }
        }

        template<typename U>
        void assign_with_type(U src) {
            using Ut = std::decay_t<U>;
            static_assert(std::is_same_v<T, U>, "the given type is not in the list");
            if constexpr(std::is_rvalue_reference_v<U>){
                new(&this->val) T(std::forward(src));
            } else {
                new(&this->val) T(src);
            }
        }
        
        template<class U>
        static constexpr bool is_same() { return std::is_same_v<T, U>; }
        static consteval uint32_t get_id() { return index; }
    };

    template<uint32_t ind, class... Ts> struct _type_at;
    template<class T, class... Ts>
    struct _type_at<0, T, Ts...> {
        using type = T;
    };
    template<uint32_t index, class T, class... Ts>
    struct _type_at<index, T, Ts...> {
        using type = typename _type_at<index - 1, Ts...>::type;
    };

    template<uint32_t index, class T, class... Ts>
    struct indexed_type_list<index, T, Ts...> {
        inline static constexpr auto id = index;
        using next = indexed_type_list<index + 1, Ts...>;

        template<class U>
        static consteval uint32_t id_of(){ 
            if constexpr(std::is_same_v<T,U>)
                return index;
            else 
                return next::template id_of<U>(); 
        }

        template<uint32_t ind>
        using type_at = typename _type_at<ind, T, Ts...>::type;        
    };

    template<uint32_t index, class T>
    struct indexed_type_list<index, T> {
        inline static constexpr auto id = index;

        template<class U>
        static consteval uint32_t id_of(){ 
            static_assert(std::is_same_v<U, T>, "the given type is not in the list");
            return index; 
        }

        template<uint32_t ind>
        using type_at = typename _type_at<ind, T>::type;
    };

    template<class T>
    using types_of = T::type_list_t;

    template<class T, class... Ts>
    consteval uint32_t id_of(choice<Ts...>&){
        return choice<Ts...>::type_list_t::template id_of<T>();
    }
}