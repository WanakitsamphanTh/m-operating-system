#pragma once

#include <type_traits>
#include <utility>
#include <new>
#include <functional>
#include <mstd/core.hpp>

namespace mstd {
    
    using std::is_reference_v;
    using std::is_constructible_v;
    using std::is_convertible_v;
    using std::remove_reference_t;
    using std::forward;
    using std::move;
    using std::invoke;
    using std::invoke_result_t;
    using std::is_void_v;
    using std::is_same_v;
    using std::is_invocable_v;
    using std::is_array_v;
    using std::is_const_v;

    template<class T> class maybe;
    template<class T, class validation> class firm;

    template<class T> class some_t;
    class nothing_t{};
    constexpr nothing_t nothing;

    template<class T>
    constexpr bool is_firm_v = false;

    template<class T, class validation>
    constexpr bool is_firm_v<firm<T, validation>> = true;

    /* ===================== concrete maybe ===================== */
    template<class T> requires (!is_reference_v<T> && !is_firm_v<T>)
    class maybe<T> {
        alignas(T) char buffer[sizeof(T)];
        bool valid;
        maybe(bool valid): valid(valid){}
        
    public:
        template<class U, class... Args>
        requires (!std::is_reference_v<U> &&
                  std::is_constructible_v<U, Args&&...>)
        friend maybe<U> some(Args&&... args);

        /* constructors */
        maybe() noexcept: maybe(false){}
        template<class Arg, class... Args>
            requires (is_constructible_v<T, Arg, Args...>)
        maybe(Arg&& arg, Args&&... args) : maybe(true){
            auto storage = reinterpret_cast<T*>(buffer);
            new(storage) T(forward<Arg>(arg), forward<Args>(args)...);
        }
        maybe(nothing_t nothing): maybe(false){}
        maybe& operator=(nothing_t nothing) {
            try_remove_value();
            return *this;
        }

        /* move*/
        template<class U> requires (is_constructible_v<T, U&&>)
        maybe(maybe<U>&& other): valid(false){
            if(other.valid){
                auto dst = reinterpret_cast<T*>(buffer);
                auto src = reinterpret_cast<U*>(other.buffer);
                new(dst) T(move(*src));
                valid = true;
                other.try_remove_value();
            }
        }

        template<class U> requires (is_constructible_v<T, U&&>)
        maybe& operator=(maybe<U>&& other){
            if(this == &other) return *this;
            try_remove_value();
            if(other.valid){
                auto dst = reinterpret_cast<T*>(buffer);
                auto src = reinterpret_cast<U*>(other.buffer);
                new(dst) T(move(*src));
                valid = true;
                other.try_remove_value();
            } 
            return *this;
        }
        template<class U> requires (is_constructible_v<T, U&&>)
        maybe& operator=(U&& val){
            if(reinterpret_cast<void*>(this->buffer) == reinterpret_cast<void*>(&val)) 
                return *this;
            try_remove_value();
            auto storage = reinterpret_cast<T*>(buffer);
            new(storage) T(forward<U>(val));
            valid = true;
            return *this;
        }

        /* copy */
        template<class U> requires (is_constructible_v<T, const U&>)
        maybe(const maybe<U>& other): valid(false){
            if(other.valid){
                auto dst = reinterpret_cast<T*>(buffer);
                auto src = reinterpret_cast<const U*>(other.buffer);
                new(dst) T(*src);
            }
            valid = other.valid;
        }
        template<class U> requires (is_constructible_v<T, const U&>)
        maybe& operator=(const maybe<U>& other){
            if(this == &other) return *this;
            try_remove_value();
            if(other.valid){
                auto dst = reinterpret_cast<T*>(buffer);
                auto src = reinterpret_cast<const U*>(other.buffer);
                new(dst) T(*src);
            }
            valid = other.valid;
            return *this;
        }
        template<class U> requires (is_constructible_v<T, const U&>)
        maybe& operator=(const U& val){
            if(reinterpret_cast<void*>(this->buffer) == reinterpret_cast<void*>(&val)) 
                return *this;
            try_remove_value();
            auto storage = reinterpret_cast<T*>(buffer);
            new(storage) T(val);
            valid = true;
            return *this;
        }

        /* destructors */
        ~maybe() {
            if(valid) reinterpret_cast<T*>(buffer)->~T();
        }

        /* destroy*/
        void try_remove_value() {
            if(valid) reinterpret_cast<T*>(buffer)->~T();
            valid = false;
        }

        /* validity */
        operator bool() const noexcept { return valid; }
        bool operator!() const noexcept { return !valid; }
        bool is_valid() const noexcept { return valid; }

        /* dereference : unsafe */
        T* operator->() { 
            return reinterpret_cast<T*>(buffer); 
        }
        const T* operator->() const { 
            return reinterpret_cast<const T*>(buffer);
        }
        T& operator*() { 
            return *reinterpret_cast<T*>(buffer);
        }
        const T& operator*() const { 
            return *reinterpret_cast<const T*>(buffer); 
        }

        /* ownership*/
        T&& take_unchecked() {
            return move(*reinterpret_cast<T*>(buffer));
        }

        T& borrow_unchecked() {
            return *reinterpret_cast<T*>(buffer);
        }

        const T& borrow_const_unchecked() const {
            return *reinterpret_cast<T*>(buffer);
        }
        
        T&& take() {
            if(is_valid())
                return take_unchecked();
            panic("Empty maybe<T> cannot be taken as value");
        }

        const T& borrow_const() const {
            if(is_valid())
                return *reinterpret_cast<const T*>(buffer);
            panic("Empty maybe<T> cannot be borrowed as value");
        }

        T& borrow() {
            if(is_valid())
                return *reinterpret_cast<T*>(buffer);
            panic("Empty maybe<T> cannot be borrowed as value");
        }

        T&& take(const char* msg) {
            if(is_valid())
                return take_unchecked();
            panic(msg);
        }

        const T& borrow_const(const char* msg) const {
            if(is_valid())
                return *reinterpret_cast<const T*>(buffer);
            panic(msg);
        }

        T& borrow(const char* msg) {
            if(is_valid())
                return *reinterpret_cast<T*>(buffer);
            panic(msg);
        }
        

        /* operation */
        template<class Fn, class Result = invoke_result_t<Fn, T&&>>
            requires (!is_void_v<Result>)
        maybe<Result> then(Fn&& fn) {
            if(valid) return maybe<Result>(invoke(forward<Fn>(fn), take_unchecked()));
            else return nothing;
        }

        template<class Fn, class Result = invoke_result_t<Fn, T&&>>
            requires (is_void_v<Result>)
        void then(Fn&& fn) {
            if(valid) invoke(forward<Fn>(fn), take_unchecked());
        }

        template<class Fn, class Result = invoke_result_t<Fn>>
            requires (!is_void_v<Result>)
        maybe<Result> when_nothing(Fn&& fn) {
            if(!valid) return maybe<Result>(invoke(forward<Fn>(fn)));
            else return nothing;
        }

        template<class Fn, class Result = invoke_result_t<Fn>>
            requires (is_void_v<Result>)
        void when_nothing(Fn&& fn) {
            if(!valid) invoke(forward<Fn>(fn));
        }

        template<class U> requires (is_convertible_v<U, T>)
        T or_default(U&& val) {
            if(valid) return take_unchecked();
            else return T(forward<U>(val));
        }
        
        template<class Fn, class U, class Result = invoke_result_t<Fn, T&&>>
            requires (!is_void_v<Result> && is_convertible_v<U, Result>)
        Result then_or_default(Fn&& fn, U&& val) {
            if(valid) return invoke(forward<Fn>(fn), take_unchecked());
            else return Result(forward<U>(val));
        }

        template<class Fn, class Result = invoke_result_t<Fn>>
            requires (!is_void_v<Result> && is_convertible_v<Result, T>)
        T or_else(Fn&& fn) {
            if(valid) return take_unchecked();
            else return invoke(forward<Fn>(fn));
        }

        template<class Fn, class Result = invoke_result_t<Fn>>
            requires (is_void_v<Result>)
        void or_else(Fn&& fn) {
            if(!valid) invoke(forward<Fn>(fn));
        }

        template<class Fn, class Gn, class Result = invoke_result_t<Fn, T&&>>
            requires (!is_void_v<Result> && is_constructible_v<invoke_result_t<Gn>, Result>)
        Result then_or_else(Fn&& fn, Gn&& gn) {
            if(valid) return invoke(forward<Fn>(fn), take_unchecked());
            else return invoke(forward<Gn>(gn));
        }

        template<class Fn, class Gn, class Result = invoke_result_t<Fn, T&&>>
            requires (is_void_v<Result> && is_void_v<invoke_result_t<Gn>>)
        void then_or_else(Fn&& fn, Gn&& gn) {
            if(valid) invoke(forward<Fn>(fn), take_unchecked());
            else invoke(forward<Gn>(gn));
        }

        template<class Fn, class Result = invoke_result_t<Fn, const T&>>
            requires (!is_void_v<Result>)
        maybe<Result> inspect(Fn&& fn) const {
            if(valid) return maybe<Result>(invoke(forward<Fn>(fn), borrow_const_unchecked()));
            else return nothing;
        }

        template<class Fn, class Result = invoke_result_t<Fn, const T&>>
            requires (is_void_v<Result>)
        void inspect(Fn&& fn) const {
            if(valid) invoke(forward<Fn>(fn), borrow_const_unchecked());
        }

        template<class Fn>
            requires (is_invocable_v<Fn, T&>)
        maybe apply(Fn&& fn) {
            if(valid)
                invoke(forward<Fn>(fn), borrow_unchecked());
            return *this;
        }

        template<class Fn, class Result = invoke_result_t<Fn>>
        maybe apply_when_nothing(Fn&& fn) {
            if(!valid){
                if constexpr(is_same_v<Result, maybe<T>>){
                    *this = invoke(forward<Fn>(fn));
                } else {
                    auto storage = reinterpret_cast<T*>(buffer);
                    new(storage) T(invoke(forward<Fn>(fn)));
                    valid = true;
                }
            }
            return *this;
        }

    };

    /* ===================== reference maybe ===================== */
    template<class TRef>
        requires (is_reference_v<TRef>)
    class maybe<TRef> {
        using T = std::remove_reference_t<TRef>;
        T* ptr;
        maybe(T* ref): ptr(ref){}
        T& borrow_as_if() { return *ptr; }
    public:
        maybe(): maybe(nullptr){}
        template<class U>
            requires (is_convertible_v<U&, T&>)
        maybe(U& ref): maybe(&ref){}
        maybe(nothing_t): maybe(nullptr){}
        maybe& operator=(nothing_t nothing) {
            release();
        }
        
        /* move */
        template<class U> requires (is_convertible_v<U&, TRef>)
        maybe(maybe<U&>&& other): maybe(other.ref){}

        template<class U> requires (is_convertible_v<U&, TRef>)
        maybe& operator=(maybe<U&>&& other){
            ptr = other.ptr;
        }

        /* copy */
        template<class U> requires (is_convertible_v<U&, TRef>)
        maybe(const maybe<U&>& other): maybe(other.ref){}

        template<class U> requires (is_convertible_v<U&, TRef>)
        maybe& operator=(const maybe<U&>& other){
            ptr = other.ptr;
        }

        template<class U> requires (is_convertible_v<U&, TRef>)
        maybe& operator=(U& ref){
            this->ptr = ref;
        }

        /* release*/
        void release() {
            ptr = nullptr;
        }

        /* validity */
        operator bool() const noexcept { return ptr != nullptr; }
        bool operator!() const noexcept { return ptr == nullptr; }
        bool is_valid() const noexcept { return ptr != nullptr; }

        /* ownership */
        TRef take_unchecked() { return *ptr; }

        template<class U = std::remove_const_t<T>>
        const U& borrow_const_unchecked() const { return *ptr; }

        template<typename U = T> requires (!is_const_v<U>)
        T& borrow_unchecked() {
            return *ptr;
        }

        TRef take() {
            if(is_valid())
                return *ptr;
            panic("Empty maybe<T> cannot be borrowed as value");
        }

        template<class U = std::remove_const_t<T>>
        const U& borrow_const() const {
            if(is_valid())
                return *ptr;
            panic("Empty maybe<T> cannot be borrowed as value");
        }

        template<typename U = T>
            requires (!is_const_v<U>)
        T& borrow() {
            if(is_valid())
                return *ptr;
            panic("Empty maybe<T> cannot be borrowed as value");
        }

        TRef take(const char* msg) {
            if(is_valid())
                return *ptr;
            panic(msg);
        }

        template<class U = std::remove_const_t<T>>
        const U& borrow_const(const char* msg) const {
            if(is_valid())
                return *ptr;
            panic(msg);
        }

        template<typename U = T>
            requires (!is_const_v<U>)
        T& borrow(const char* msg) {
            if(is_valid())
                return *ptr;
            panic(msg);
        }

        /* operation*/
        template<class Fn, class Result = invoke_result_t<Fn, T&>>
            requires (!is_void_v<Result>)
        maybe<Result> then(Fn&& fn) {
            if(is_valid()) return maybe<Result>(invoke(forward<Fn>(fn), borrow_unchecked()));
            else return nothing;
        }

        template<class Fn, class Result = invoke_result_t<Fn, TRef>>
            requires (is_void_v<Result>)
        void then(Fn&& fn) {
            if(is_valid()) invoke(forward<Fn>(fn), take_unchecked());
        }

        template<class Fn, class Result = invoke_result_t<Fn>>
            requires (!is_void_v<Result>)
        maybe<Result> when_nothing(Fn&& fn) {
            if(!is_valid()) return maybe<Result>(invoke(forward<Fn>(fn)));
            else return nothing;
        }

        template<class Fn, class Result = invoke_result_t<Fn>>
            requires (is_void_v<Result>)
        void when_nothing(Fn&& fn) {
            if(!is_valid()) invoke(forward<Fn>(fn));
        }

        template<class URef> requires (is_convertible_v<URef,TRef>)
        TRef or_default(URef ref) {
            if(is_valid()) return take_unchecked();
            else return ref;
        }
        
        template<class Fn, class URef, class Result = invoke_result_t<Fn, TRef>>
            requires (!is_void_v<Result> && is_convertible_v<URef, Result>)
        Result then_or_default(Fn&& fn, URef ref) {
            if(is_valid()) return invoke(forward<Fn>(fn), take_unchecked());
            else return ref;
        }

        template<class Fn, class Result = invoke_result_t<Fn>>
            requires (!is_void_v<Result> && is_convertible_v<Result, TRef>)
        TRef or_else(Fn&& fn) {
            if(is_valid()) return take_unchecked();
            else return invoke(forward<Fn>(fn));
        }

        template<class Fn, class Result = invoke_result_t<Fn>>
            requires (is_void_v<Result>)
        void or_else(Fn&& fn) {
            if(!is_valid()) invoke(forward<Fn>(fn));
        }

        template<class Fn, class Gn, class Result = invoke_result_t<Fn, TRef>>
            requires (!is_void_v<Result> && is_constructible_v<invoke_result_t<Gn>, Result>)
        Result then_or_else(Fn&& fn, Gn&& gn) {
            if(is_valid()) return invoke(forward<Fn>(fn), take_unchecked());
            else return invoke(forward<Gn>(gn));
        }

        template<class Fn, class Gn, class Result = invoke_result_t<Fn, TRef>>
            requires (is_void_v<Result> && is_void_v<invoke_result_t<Gn>>)
        void then_or_else(Fn&& fn, Gn&& gn) {
            if(is_valid()) invoke(forward<Fn>(fn), take_unchecked());
            else invoke(forward<Gn>(gn));
        }

        template<class Fn, class Result = invoke_result_t<Fn, const T&>>
            requires (!is_void_v<Result>)
        maybe<Result> inspect(Fn&& fn) const {
            if(is_valid()) return maybe<Result>(invoke(forward<Fn>(fn), borrow_const_unchecked()));
            else return nothing;
        }

        template<class Fn, class Result = invoke_result_t<Fn, const T&>>
            requires (is_void_v<Result>)
        void inspect(Fn&& fn) const {
            if(is_valid()) invoke(forward<Fn>(fn), borrow_const_unchecked());
        }

        template<class Fn>
            requires (is_invocable_v<Fn, T&>)
        maybe apply(Fn&& fn) {
            if(is_valid())
                invoke(forward<Fn>(fn), borrow_unchecked());
            return *this;
        }

        template<class Fn, class Result = invoke_result_t<Fn>>
        maybe apply_when_nothing(Fn&& fn) {
            if(!is_valid()){
                if constexpr(is_same_v<Result, maybe<TRef>>){
                    *this = invoke(forward<Fn>(fn));
                } else {
                    ptr = &invoke(forward<Fn>(fn));
                }
            }
            return *this;
        }
    };

    /* ========================= non_null type ======================== */
    template<class T>
    struct has_invalid;

    template<class T>
    concept has_invalid_value
        = requires(T& t) { 
            { has_invalid<T>::test(t) } -> std::same_as<bool>;
            { has_invalid<T>::sentinel }; 
        };

    template<class T>
    struct has_invalid<T*>{
        static constexpr T* sentinel = nullptr;
        static bool test(T*& ptr){ return ptr != nullptr; }
    };

    template<class T>
    using has_null = has_invalid<T*>;

    template<class T, class validation = has_invalid<T>>
    class firm {
        T value;
        firm(T&& val): value(forward<T>(val)){}
    public:
        static maybe<firm<T>> test(T&& v){
            if(!validation::test(v))
                return nothing;
            else
                return maybe<firm<T>>({forward<T>(v)});
        }
        
        static firm<T> assume(T&& val){
            return firm<T>(forward<T>(val));
        }

        operator T() { return value; }

        template<class U = T> requires std::is_pointer_v<U>
        auto operator->() {
            return value;
        }

        template<class U = T> requires std::is_pointer_v<U>
        auto operator*() {
            return *value;
        }
    };

    template<class T>
    using non_null = firm<T*>;
    
    /* ===================== specialization for maybe<firm<T>> ==============*/
    template<class T, class validation>
    class maybe<firm<T, validation>> {
        static constexpr auto sentinel = validation::sentinel;
        T val;
    public:
        maybe(): val(sentinel){}
        template<class U> requires is_constructible_v<T, U>
        maybe(const U& _val): maybe(){
            T tmp(_val);
            if(validation::test(tmp)) val = move(tmp);
        }
        template<class U> requires std::is_assignable_v<T, U>
        maybe(U&& val): maybe(){
            if(validation::test(val)) val = forward<U>(val);
        }
        maybe(nothing_t): maybe(){}

        /* copy */
        maybe(const maybe& other): val(other.val){}
        maybe& operator=(const maybe& other){
            val = other.val;
            return *this;
        }
        
        /* move */
        maybe(maybe&& other): val(move(other.val)){}
        maybe& operator=(maybe&& other){
            val = move(other.val);
            return *this;
        }

        /* validity */
        operator bool() const noexcept { return validation::test(val); }
        bool operator!() const noexcept { return !validation::test(val); }
        bool is_valid() const noexcept { return validation::test(val); }

        /* ownership */
        firm<T>&& take_unchecked() { return move(*reinterpret_cast<firm<T>*>(val)); }
        const firm<T>& borrow_const_unchecked() const { return *reinterpret_cast<const firm<T>*>(val); }
        firm<T>& borrow_unchecked() { return *reinterpret_cast<firm<T>*>(val); }

        firm<T>&& take() {
            if(is_valid())
                return take_unchecked();
            panic("Empty maybe<T> cannot be borrowed as value");
        }

        const firm<T>& borrow_const() const {
            if(is_valid())
                return borrow_const_unchecked();
            panic("Empty maybe<T> cannot be borrowed as value");
        }

        firm<T>& borrow() {
            if(is_valid())
                return borrow_unchecked();
            panic("Empty maybe<T> cannot be borrowed as value");
        }
    };

    /* ===================== some() definitions ===================== */

    template<class T>
    maybe<T> some(T&& value){
        maybe<T> something(forward<T>(value));
        return something;
    }

    template<class T, class... Args>
        requires (!is_reference_v<T> && is_constructible_v<T, Args&&...>)
    maybe<T> some(Args&&... args){
        maybe<T> something(true);
        auto storage = reinterpret_cast<T*>(something.buffer);
        new (storage) T(forward<Args>(args)...);
        return something;
    }

    template<class TRef, class U> 
        requires (is_reference_v<TRef> 
            && is_convertible_v<U&, TRef>)
    maybe<TRef> some(U& ref){
        return maybe<TRef>(ref);
    }
}