#pragma once

#include <type_traits>
#include <utility>
#include <new>
#include <functional>
#include <mstd/lazy.hpp>


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

    template<class T, class... Args>
        requires (!is_reference_v<T> && is_constructible_v<T, Args&&...>)
    maybe<T> some(Args&&... args);

    template<class TRef, class URef> 
        requires (is_reference_v<TRef> 
            && is_convertible_v<URef, remove_reference_t<TRef>&>)
    maybe<TRef> some(URef ref);

    template<class T> class some_t;
    class nothing_t{};
    constexpr nothing_t nothing;

    /* ===================== concrete maybe ===================== */
    template<class T> requires (!is_reference_v<T>)
    class maybe<T> {
        alignas(T) char buffer[sizeof(T)];
        bool valid;
        maybe(bool valid): valid(valid){}
        
        T&& move_value() {
            return move(*reinterpret_cast<T*>(buffer));
        }
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
        T&& take() {
            return move_value();
        }

        const T& borrow_const() const {
            return *reinterpret_cast<const T*>(buffer);
        }

        T& borrow() {
            return *reinterpret_cast<T*>(buffer);
        }

        /* operation */
        template<class Fn, class Result = invoke_result_t<Fn, T&&>>
            requires (!is_void_v<Result>)
        maybe<Result> then(Fn&& fn) {
            if(valid) return maybe<Result>(invoke(forward<Fn>(fn), move_value()));
            else return nothing;
        }

        template<class Fn, class Result = invoke_result_t<Fn, T&&>>
            requires (is_void_v<Result>)
        void then(Fn&& fn) {
            if(valid) invoke(forward<Fn>(fn), move_value());
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
            if(valid) return move_value();
            else return T(forward<U>(val));
        }
        
        template<class Fn, class U, class Result = invoke_result_t<Fn, T&&>>
            requires (!is_void_v<Result> && is_convertible_v<U, Result>)
        Result then_or_default(Fn&& fn, U&& val) {
            if(valid) return invoke(forward<Fn>(fn), move_value());
            else return Result(forward<U>(val));
        }

        template<class Fn, class Result = invoke_result_t<Fn>>
            requires (!is_void_v<Result> && is_convertible_v<Result, T>)
        T or_else(Fn&& fn) {
            if(valid) return move_value();
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
            if(valid) return invoke(forward<Fn>(fn), move_value());
            else return invoke(forward<Gn>(gn));
        }

        template<class Fn, class Gn, class Result = invoke_result_t<Fn, T&&>>
            requires (is_void_v<Result> && is_void_v<invoke_result_t<Gn>>)
        void then_or_else(Fn&& fn, Gn&& gn) {
            if(valid) invoke(forward<Fn>(fn), move_value());
            else invoke(forward<Gn>(gn));
        }

        template<class Fn, class Result = invoke_result_t<Fn, const T&>>
            requires (!is_void_v<Result>)
        maybe<Result> inspect(Fn&& fn) const {
            if(valid) return maybe<Result>(invoke(forward<Fn>(fn), borrow_const()));
            else return nothing;
        }

        template<class Fn, class Result = invoke_result_t<Fn, const T&>>
            requires (is_void_v<Result>)
        void inspect(Fn&& fn) const {
            if(valid) invoke(forward<Fn>(fn), borrow_const());
        }

        template<class Fn>
            requires (is_invocable_v<Fn, T&>)
        maybe apply(Fn&& fn) {
            if(valid)
                invoke(forward<Fn>(fn), borrow());
            return *this;
        }

        template<class Fn, class Result = invoke_result_t<Fn>>
            requires (is_convertible_v<Result, T>)
        maybe apply_when_nothing(Fn&& fn) {
            if(!valid){
                auto storage = reinterpret_cast<T*>(buffer);
                new(storage) T(invoke(forward<Fn>(fn)));
                valid = true;
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
        template<class URef>
            requires (is_convertible_v<URef, T&>)
        maybe(URef ref): maybe(&ref){}
        maybe(nothing_t): maybe(nullptr){}
        maybe& operator=(nothing_t nothing) {
            release();
        }
        
        /* move */
        template<class URef> requires (is_convertible_v<URef, TRef>)
        maybe(maybe<URef>&& other): maybe(other.ref){}

        template<class URef> requires (is_convertible_v<URef, TRef>)
        maybe& operator=(maybe<URef>&& other){
            ptr = other.ptr;
        }

        /* copy */
        template<class URef> requires (is_convertible_v<URef, TRef>)
        maybe(const maybe<URef>& other): maybe(other.ref){}

        template<class URef> requires (is_convertible_v<URef, TRef>)
        maybe& operator=(const maybe<URef>& other){
            ptr = other.ptr;
        }

        template<class URef> requires (is_convertible_v<TRef, URef>)
        maybe& operator=(URef ref){
            this->ptr = ref;
        }

        /* dereference */
        template<class VRef> requires(is_convertible_v<TRef, VRef>)
        operator VRef() { return ptr; }
        template<class VRef> requires(is_convertible_v<const TRef, VRef>)
        operator const VRef() const { return ptr; }

        template<typename U = T>
            requires (!is_const_v<U>)
        T* operator->() { 
            return ptr;
        }
        const T* operator->() const { 
            return ptr;
        }
        T& operator*() { 
            return *ptr;
        }
        const T& operator*() const { 
            return *ptr; 
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
        TRef take() {
            return *ptr;
        }

        const T& borrow_const() const {
            return *ptr;
        }

        template<typename U = T>
            requires (!is_const_v<U>)
        T& borrow() {
            return *ptr;
        }

        /* operation*/
        template<class Fn, class Result = invoke_result_t<Fn, T&>>
            requires (!is_void_v<Result>)
        maybe<Result> then(Fn&& fn) {
            if(is_valid()) return maybe<Result>(invoke(forward<Fn>(fn), borrow()));
            else return nothing;
        }

        template<class Fn, class Result = invoke_result_t<Fn, TRef>>
            requires (is_void_v<Result>)
        void then(Fn&& fn) {
            if(is_valid()) invoke(forward<Fn>(fn), take());
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
            if(is_valid()) return take();
            else return ref;
        }
        
        template<class Fn, class URef, class Result = invoke_result_t<Fn, TRef>>
            requires (!is_void_v<Result> && is_convertible_v<URef, Result>)
        Result then_or_default(Fn&& fn, URef ref) {
            if(is_valid()) return invoke(forward<Fn>(fn), take());
            else return ref;
        }

        template<class Fn, class Result = invoke_result_t<Fn>>
            requires (!is_void_v<Result> && is_convertible_v<Result, TRef>)
        TRef or_else(Fn&& fn) {
            if(is_valid()) return take();
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
            if(is_valid()) return invoke(forward<Fn>(fn), take());
            else return invoke(forward<Gn>(gn));
        }

        template<class Fn, class Gn, class Result = invoke_result_t<Fn, TRef>>
            requires (is_void_v<Result> && is_void_v<invoke_result_t<Gn>>)
        void then_or_else(Fn&& fn, Gn&& gn) {
            if(is_valid()) invoke(forward<Fn>(fn), take());
            else invoke(forward<Gn>(gn));
        }

        template<class Fn, class Result = invoke_result_t<Fn, const T&>>
            requires (!is_void_v<Result>)
        maybe<Result> inspect(Fn&& fn) const {
            if(is_valid()) return maybe<Result>(invoke(forward<Fn>(fn), borrow_const()));
            else return nothing;
        }

        template<class Fn, class Result = invoke_result_t<Fn, const T&>>
            requires (is_void_v<Result>)
        void inspect(Fn&& fn) const {
            if(is_valid()) invoke(forward<Fn>(fn), borrow_const());
        }

        template<class Fn>
            requires (is_invocable_v<Fn, T&>)
        maybe apply(Fn&& fn) {
            if(is_valid())
                invoke(forward<Fn>(fn), borrow());
            return *this;
        }
    };

    /* ===================== some() definitions ===================== */

    template<class T, class... Args>
        requires (!is_reference_v<T> && is_constructible_v<T, Args&&...>)
    maybe<T> some(Args&&... args){
        maybe<T> something(true);
        auto storage = reinterpret_cast<T*>(something.buffer);
        new (storage) T(forward<Args>(args)...);
        return something;
    }

    template<class TRef, class URef> 
        requires (is_reference_v<TRef> && is_convertible_v<TRef, URef&&>)
    maybe<TRef> some(URef&& ref){
        return maybe<TRef>(ref);
    }
}