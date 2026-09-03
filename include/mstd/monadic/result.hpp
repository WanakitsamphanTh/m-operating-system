#pragma once
#include <type_traits>
#include <utility>
#include <functional>
#include "mstd/monadic/maybe.hpp"

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
    using std::decay;
    using std::is_const_v;

    template<class T, class E> class result;

    template<class T>
    inline constexpr bool is_result_t = false;
    template<class T, class E> 
    inline constexpr bool is_result_t<result<T, E>> = true;

    template<class T>
    struct ok_t;

    template<>
    struct ok_t<void>{};
    constexpr ok_t<void> Ok;

    template<class E>
    struct err_t;

    /* ========== result<T,E> ========== */
    template<class T, class E>
        requires (!is_reference_v<T> && !is_void_v<T>)
    class result<T,E>{
        union { T val; E err; } storage;
        bool _ok;
        void reset() {
            if(_ok) storage.val.~T();
            else storage.val.~E();
        }
    public:
        using value_type = T;
        using error_type = E;

        /* friend */
        template<class Ut, class Et>
        friend class result;

        /* factory */
        template<class U>
            requires (is_constructible_v<T, U>)
        result(ok_t<U>&& ok): _ok(true){
            new(&storage.val) T(*ok);
        }

        template<class U>
            requires (is_constructible_v<E, U>)
        result(err_t<U>&& err): _ok(true){
            new(&storage.err) E(*err);
        }

        /* copy */
        template<class Ut, class Et>
            requires (is_constructible_v<T, const Ut&> && is_constructible_v<E, const Et&>)
        result(const result<Ut, Et>& other): _ok(false){
            if(other._ok){
                new(&storage.val) E(other.borrow_value());
                _ok = true;
            } else {
                new(&storage.err) E(other.borrow_error());
                _ok = false;
            }
        }

        template<class Ut, class Et>
            requires (is_constructible_v<T, const Ut&> && is_constructible_v<E, const Et&>)
        result& operator=(const result<Ut, Et>& other){
            if(this == &other) return *this;
            reset();
            if(other._ok){
                new(&storage.val) E(other.borrow_value);
                _ok = true;
            } else {
                new(&storage.err) E(other.borrow_error);
                _ok = false;
            }
            return *this;
        }

        /* move */
        template<class Ut, class Et>
            requires (is_constructible_v<T, Ut&&> && is_constructible_v<E, Et&&>)
        result(result<Ut, Et>&& other): _ok(false){
            if(other._ok){
                new(&storage.val) E(other.take_value());
                _ok = true;
            } else {
                new(&storage.err) E(other.take_error());
                _ok = false;
            }
        }

        template<class Ut, class Et>
            requires (is_constructible_v<T, Ut&&> && is_constructible_v<E, Et&&>)
        result& operator=(result<Ut, Et>&& other){
            if(this == &other) return *this;
            reset();
            if(other._ok){
                new(&storage.val) E(other.take_value());
                _ok = true;
            } else {
                new(&storage.err) E(other.take_error());
                _ok = false;
            }
            return *this;
        }

        /* validity */
        bool is_ok() const { return _ok; }
        bool is_error() const { return !_ok; }
        operator bool() const { return is_ok(); }
        bool operator!() const { return is_error(); }

        /* forwarding */
        err_t<E> propogate() {
            return err_t<E>(this->take_error());
        }

        /* take */
        T&& take_value() {
            return move(storage.val);
        }
        E&& take_error() {
            return move(storage.err);
        }

        maybe<T> safe_take_value() {
            if(is_ok()) return maybe<T>(take_value());
            else return nothing;
        }
        maybe<E> safe_take_error() {
            if(is_ok()) return maybe<T>(take_error());
            else return nothing;
        }        

        /* borrow */
        T& borrow_value() { 
            return storage.val;
        }
        E& borrow_error() { 
            return storage.err;
        }
        const T& borrow_value_const() const { 
            return storage.val;
        }
        const E& borrow_error_const() const { 
            return storage.err;
        }

        /* operation */
        template<class Fn, class Result = invoke_result_t<Fn, T&&>>
            requires (is_result_t<Result>)
        Result then(Fn&& fn){
            using value_type = Result::value_type;
            using err_type = Result::error_type;

            if(is_ok()) {
                if constexpr(is_void_v<value_type>){
                    invoke(forward<Fn>(fn), take_value());
                    return Ok;
                } else if constexpr(is_result_t<Result>) {
                    using res_err = typename Result::error_type;
                    static_assert(
                        is_constructible_v<res_err, E>, 
                        "the two result types are not compatible"
                    );
                    return invoke(forward<Fn>(fn), take_value());
                }
                else
                    return ok<Result>(invoke(forward<Fn>(fn), take_value()));
            } else return propogate();
        }

        template<class Fn, class Gn,
            class FResult = invoke_result_t<Fn, T&&>,
            class GResult = invoke_result_t<Gn, E&&>
        > requires (is_same_v<FResult, GResult>)
        FResult then_or_else(Fn&& fn, Gn&& gn) {
            if(is_ok())
                return invoke(forward<Fn>(fn), take_value());
            else
                return invoke(forward<Gn>(gn), take_error());
        }

        template<class Fn, class G,
            class Result = invoke_result_t<Fn, T&&>
        > requires (is_constructible_v<Result, G>)
        Result then_or_default(Fn&& fn, G&& g) {
            if(is_ok()) {
                if constexpr(is_result_t<Result>){
                    using val_type = typename Result::value_type;
                    static_assert(
                        is_same_v<G, val_type>, 
                        "the result type and the default type are not compatible"
                    );
                    return invoke(forward<Fn>(fn), take_value()).take_or_default(forward<G>(g));
                } else {
                    return invoke(forward<Fn>(fn), take_value());
                }
            }
            else return forward<G>(g);  
        }

        template<class Fn, class G> 
            requires (is_convertible_v<T&&, G&&>)
        T take_or_default(Fn&& fn, G&& g) {
            if(is_ok()) return take_value();
            else return forward<G>(g);  
        }

        template<class Fn, class Result = invoke_result_t<Fn, E&&>> 
            requires (is_same_v<T, Result>)
        T take_or_else(Fn&& fn) {
            if(is_ok()) return take_value();
            else return invoke(forward<Fn>(fn), take_error());  
        }

        template<class Fn>
        result& then_apply(Fn&& fn) {
            if(is_ok()) invoke(forward<Fn>(fn), borrow_value());
            return *this;
        }

        template<class Fn>
        result& or_apply(Fn&& fn) {
            if(is_error()) invoke(forward<Fn>(fn), borrow_error());
            return *this;
        }

        template<class Fn, class Gn>
        result& either_apply(Fn&& fn, Gn&& gn) {
            if(is_ok()) invoke(forward<Fn>(fn), borrow_value());
            else invoke(forward<Gn>(gn), borrow_error());
            return *this;
        }

        template<class Fn>
        result& inspect(Fn&& fn)  {
            if(is_ok()) invoke(forward<Fn>(fn), borrow_value());
            return *this;
        }

        template<class Fn>
        result& inspect_error(Fn&& fn)  {
            if(is_error()) invoke(forward<Fn>(fn), borrow_error());
            return *this;
        }

        template<class Fn>
        result& inspect(Fn&& fn) const {
            if(is_ok()) invoke(forward<Fn>(fn), borrow_value_const());
            return *this;
        }

        template<class Fn>
        result& inspect_error(Fn&& fn) const {
            if(is_error()) invoke(forward<Fn>(fn), borrow_error_const());
            return *this;
        }

        template<class Fn, class Gn>
        result& either_inspect(Fn&& fn, Gn&& gn) const  {
            if(is_ok()) invoke(forward<Fn>(fn), borrow_value_const());
            else invoke(forward<Gn>(fn), borrow_error());
            return *this;
        }

        template<class... Args>
            requires (is_invocable_v<T, Args...>)
        auto then_invoke(Args&&... args){
            using Result = invoke_result_t<T, Args...>;
            if(is_ok()){
                if constexpr (is_result_t<Result>) {
                    using err_type = typename Result::error_type;
                    static_assert(
                        is_constructible_v<err_type, E>, 
                        "invoke results are not compatible"
                    );
                    return invoke(take_value(), forward<Args>(args)...);
                    /* F: T -> result<U, E'> . returns result<U,E'>*/
                } else if constexpr(is_void_v<Result>) {
                    invoke(take_value(), forward<Args>(args)...);
                    return Ok;
                    /* F: T -> void . returns results<void, E>*/
                } else {
                    return ok<Result>(invoke(take_value(), forward<Args>(args)...));
                    /* F: T -> U . returns results<U, E'>*/
                }
            } else return propogate();
        }
    };

    /* ========== result<T&,E> ========== */
    template<class TRef, class E>
        requires (is_reference_v<TRef>)
    class result<TRef, E>{
        using T = remove_reference_t<TRef>;
        union {
            T* ref;
            E err;
        } storage;
        bool _ok;
    public:
        template<class URef, class Et>
        friend class result;

        template<class URef>
            requires (is_reference_v<URef> && is_convertible_v<URef, TRef>)
        result(ok_t<URef>&& o): _ok(true){
            storage.ref = &*o;
        }

        template<class Et>
            requires (is_constructible_v<E, Et&&>)
        result(ok_t<Et>&& e): _ok(false) {
            new(&storage.err) E(*e);
        }

        /* move*/
        template<class URef, class Et>
            requires (is_reference_v<URef> && is_convertible_v<URef, TRef> && is_constructible_v<E, Et&&>)
        result(result<URef, Et>&& other): _ok(false) {
            if(other.is_ok()) {
                storage.ref = other.storage.ref;
                _ok = true;
            } else {
                new(&storage.err) E(move(other.storage.ref));
                _ok = false;
            }
        }
        template<class URef, class Et>
            requires (is_reference_v<URef> && is_convertible_v<URef, TRef> && is_constructible_v<E, Et&&>)
        result& operator=(result<URef, Et>&& other) {
            if(this == &other) return *this;
            if(is_error())
                storage.err.~E();
            if(other.is_ok()) {
                storage.ref = other.storage.ref;
                _ok = true;
            } else {
                new(&storage.err) E(move(other.storage.ref));
                _ok = false;
            }
            return *this;
        }

        /* copy */
        template<class URef, class Et>
            requires (is_reference_v<URef> && is_convertible_v<URef, TRef> && is_constructible_v<E, const Et&>)
        result(const result<URef, Et>& other): _ok(false) {
            if(other.is_ok()) {
                storage.ref = other.storage.ref;
                _ok = true;
            } else {
                new(&storage.err) E(other.storage.ref);
                _ok = false;
            }
        }
        template<class URef, class Et>
            requires (is_reference_v<URef> && is_convertible_v<URef, TRef> && is_constructible_v<E, const Et&>)
        result& operator=(result<URef, Et>&& other) {
            if(this == &other) return *this;
            if(is_error())
                storage.err.~E();
            if(other.is_ok()) {
                storage.ref = other.storage.ref;
                _ok = true;
            } else {
                new(&storage.err) E(other.storage.ref);
                _ok = false;
            }
            return *this;
        }

        /* validity */
        bool is_ok() const { return _ok; }
        bool is_error() const { return !_ok; }
        operator bool() const { return is_ok(); }
        bool operator!() const { return is_error(); }

        /* forwarding */
        err_t<E> propogate() {
            return err_t<E>(this->take_error());
        }

        /* take */
        TRef take_value() {
            return *storage.ref;
        }
        
        E&& take_error() {
            return storage.err;
        }

        maybe<TRef> safe_take_value() {
            if(is_ok()) return maybe<TRef>(take_value());
            else return nothing;
        }
        maybe<E> safe_take_error() {
            if(is_ok()) return maybe<T>(take_error());
            else return nothing;
        }        

        /* borrow */
        template<class U = T>
            requires (!is_const_v<U>)
        T& borrow_value() { 
            return *storage.ref;
        }
        E& borrow_error() { 
            return storage.err;
        }

        const T& borrow_value_const() const { 
            return *storage.ref;
        }
        const E& borrow_error_const() const { 
            return storage.err;
        }

        /* operation */
        template<class Fn, class Result = invoke_result_t<Fn, TRef>>
            requires (is_result_t<Result>)
        Result then(Fn&& fn){
            using value_type = Result::value_type;
            using err_type = Result::error_type;

            if(is_ok()) {
                if constexpr(is_void_v<value_type>){
                    invoke(forward<Fn>(fn), take_value());
                    return Ok;
                } else if constexpr(is_result_t<Result>) {
                    using res_err = typename Result::error_type;
                    static_assert(
                        is_constructible_v<res_err, E>, 
                        "the two result types are not compatible"
                    );
                    return invoke(forward<Fn>(fn), take_value());
                }
                else
                    return ok<Result>(invoke(forward<Fn>(fn), take_value()));
            } else return propogate();
        }

        template<class Fn, class Gn,
            class FResult = invoke_result_t<Fn, T&>,
            class GResult = invoke_result_t<Gn, E&&>
        > requires (is_same_v<FResult, GResult>)
        FResult then_or_else(Fn&& fn, Gn&& gn) {
            if(is_ok())
                return invoke(forward<Fn>(fn), take_value());
            else
                return invoke(forward<Gn>(gn), take_error());
        }

        template<class Fn, class GRef,
            class Result = invoke_result_t<Fn, T&&>
        > requires (is_convertible_v<Result, GRef>)
        Result then_or_default(Fn&& fn, GRef&& g) {
            if(is_ok()) {
                if constexpr(is_result_t<Result>){
                    using val_type = typename Result::value_type;
                    static_assert(
                        is_same_v<GRef, val_type>, 
                        "the result type and the default type are not compatible"
                    );
                    return invoke(forward<Fn>(fn), take_value()).take_or_default(forward<GRef>(g));
                } else {
                    return invoke(forward<Fn>(fn), take_value());
                }
            }
            else return forward<G>(g);  
        }

        template<class Fn, class GRef> 
            requires (is_convertible_v<TRef, GRef>)
        TRef take_or_default(Fn&& fn, GRef&& g) {
            if(is_ok()) return take_value();
            else return forward<G>(g); 
        }

        template<class Fn, class Result = invoke_result_t<Fn, E&&>> 
            requires (is_convertible_v<TRef, Result>)
        TRef take_or_else(Fn&& fn) {
            if(is_ok()) return take_value();
            else return invoke(forward<Fn>(fn), take_error());  
        }
    
        template<class Fn>
        result& then_apply(Fn&& fn) {
            if(is_ok()) invoke(forward<Fn>(fn), borrow_value());
            return *this;
        }

        
        template<class Fn>
        result& or_apply(Fn&& fn) {
            if(is_error()) invoke(forward<Fn>(fn), borrow_error());
            return *this;
        }

        template<class Fn, class Gn>
        result& either_apply(Fn&& fn, Gn&& gn) {
            if(is_ok()) invoke(forward<Fn>(fn), borrow_value());
            else invoke(forward<Gn>(gn), borrow_error());
            return *this;
        }

        // inspect
        template<class U = T, class Fn>
            requires (!is_const_v<U>)
        result& inspect(Fn&& fn) {
            if(is_ok()) invoke(forward<Fn>(fn), borrow_value());
            return *this;
        }

        template<class Fn>
        result& inspect_error(Fn&& fn) {
            if(is_error()) invoke(forward<Fn>(fn), borrow_error());
            return *this;
        }

        template<class Fn>
        result& inspect(Fn&& fn) const {
            if(is_ok()) invoke(forward<Fn>(fn), borrow_value_const());
            return *this;
        }

        template<class Fn>
        result& inspect_error(Fn&& fn) const {
            if(is_error()) invoke(forward<Fn>(fn), borrow_error_const());
            return *this;
        }
    };

    /* ========== result<void,E> ========== */
    template<class E>
    class result<void, E>{
        maybe<E> err;
    public:
        result(ok_t<void>): err(nothing){}
        template<class Et>
            requires (is_constructible_v<E, Et&&>)
        result(err_t<Et>&& e): err(some<E>(*e)){}

        /* move */
        template<class Ut, class Et>
            requires (is_constructible_v<E, Et&&>)
        result(result<Ut,Et>&& other): err(nothing) {
            if(other.is_ok()) err = other.take_error();
        }

        template<class Ut, class Et>
            requires (is_constructible_v<E, Et&&>)
        result& operator=(result<Ut,Et>&& other){
            if(this == &other) return *this;
            if(err) err.try_remove_value();
            if(other.is_ok()) err = other.take_error();
            return *this;
        }

        /* copy */
        template<class Ut, class Et>
            requires (is_constructible_v<E, const Et&>)
        result(const result<Ut,Et>& other): err(nothing) {
            if(other.is_ok()) err = other.borrow_const();
        }

        template<class Ut, class Et>
            requires (is_constructible_v<E, const Et&>)
        result& operator=(const result<Ut,Et>& other){
            if(this == &other) return *this;
            if(err) err.try_remove_value();
            if(other.is_ok()) err = other.borrow_const();
            return *this;
        }

        /* validity */
        bool is_ok() const { return !static_cast<bool>(err); }
        bool is_error() const { return static_cast<bool>(err); }
        operator bool() const { return is_ok(); }
        bool operator!() const { return is_error(); }

        /* forwarding */
        err_t<E> propogate() {
            return err_t<E>(this->take_error());
        }

        /* take */
        E&& take_error() {
            return err.take();
        }
        maybe<E> safe_take_error() {
            if(is_ok()) return maybe<T>(take_error());
            else return nothing;
        }

        /* borrow */
        E& borrow_error() { 
            return err.borrow();
        }
        const E& borrow_error_const() const { 
            return err.borrow_const();
        }

        /* operations */
        template<class Fn, class Result = invoke_result_t<Fn>>
            requires (is_result_t<Result>)
        Result then(Fn&& fn){
            using value_type = Result::value_type;
            using err_type = Result::error_type;

            if(is_ok()) {
                if constexpr(is_void_v<value_type>){
                    invoke(forward<Fn>(fn));
                    return Ok;
                } else if constexpr(is_result_t<Result>) {
                    using res_err = typename Result::error_type;
                    static_assert(
                        is_constructible_v<res_err, E>, 
                        "the two result types are not compatible"
                    );
                    return invoke(forward<Fn>(fn));
                }
                else
                    return ok<Result>(invoke(forward<Fn>(fn)));
            } else return propogate();
        }

        template<class Fn, class Gn,
            class FResult = invoke_result_t<Fn>,
            class GResult = invoke_result_t<Gn, E&&>
        > requires (is_same_v<FResult, GResult>)
        FResult then_or_else(Fn&& fn, Gn&& gn) {
            if(is_ok())
                return invoke(forward<Fn>(fn));
            else
                return invoke(forward<Gn>(gn), take_error());
        }

        template<class Fn, class G,
            class Result = invoke_result_t<Fn>
        > requires (is_constructible_v<G, Result>)
        Result then_or_default(Fn&& fn, G&& g) {
            if(is_ok()) {
                if constexpr(is_result_t<Result>){
                    using val_type = typename Result::value_type;
                    static_assert(
                        is_same_v<G, val_type>, 
                        "the result type and the default type are not compatible"
                    );
                    return invoke(forward<Fn>(fn)).take_or_default(forward<G>(g));
                } else {
                    return invoke(forward<Fn>(fn));
                }
            }
            else return forward<G>(g);
        }

        // apply
        template<class Fn>
        result& then_apply(Fn&& fn) {
            if(is_ok()) invoke(forward<Fn>(fn));
            return *this;
        }

        
        template<class Fn>
        result& or_apply(Fn&& fn) {
            if(is_error()) invoke(forward<Fn>(fn), borrow_error());
            return *this;
        }

        template<class Fn, class Gn>
        result& either_apply(Fn&& fn, Gn&& gn) {
            if(is_ok()) invoke(forward<Fn>(fn));
            else invoke(forward<Gn>(gn), borrow_error());
            return *this;
        }

        template<class Fn>
        result& inspect_error(Fn&& fn) {
            if(is_error()) invoke(forward<Fn>(fn), borrow_error());
            return *this;
        }

        template<class Fn>
        result& inspect_error(Fn&& fn) const {
            if(is_error()) invoke(forward<Fn>(fn), borrow_error_const());
            return *this;
        }

    };

    /* ========== oK_t and err_t ========== */
    template<class T> requires (!is_reference_v<T> && !is_void_v<T>)
    class ok_t<T> {
        T val;
    public:
        template<class... Args> requires (is_constructible_v<T, Args...>)
        ok_t(Args&&... args): val(forward<Args>(args)...){}
        T&& operator*() { return move(val); }
    };

    template<class TRef>
        requires (is_reference_v<TRef>)
    struct ok_t<TRef> {
        using T = remove_reference_t<TRef>;
        T* ref;
    public:
        template<class URef> requires (is_reference_v<URef> && is_convertible_v<URef, TRef>)
        ok_t(URef ref): ref(&ref){}
        T&& operator*() { return move(*ref); }
    };

    template<typename T>
    using result_ok = ok_t<T>;

    template<class E>
    struct err_t {
        E err;
    public:
        template<class... Args> requires (is_constructible_v<E, Args...>)
        err_t(Args&&... args): err(forward<Args>(args)...){}
        E&& operator*() { return move(err); }
    };

    template<class E>
    using error = err_t<E>;
}