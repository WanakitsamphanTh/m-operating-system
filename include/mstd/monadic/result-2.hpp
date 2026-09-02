#pragma once
#ifdef __cpp_exceptions
#include <stdexcept>
#else
#warning "using result<T,E> in freestading mode is unsafe"
#endif
#include <type_traits>
#include <utility>

namespace mstd {
    using std::is_reference_v;
    using std::is_constructible_v;
    using std::is_assignable_v;
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

    template<class T, class E> class result;
    template<
        class T,
        class E
    > constexpr bool is_result_t<result<T, E>>{true};

    template<class T>
    struct ok_t;

    template<class E>
    struct err_t;

    /* ========== result<T,E> ========== */
    template<class T, class E>
        requires (!is_reference_v<T> && !is_void_v<T>)
    class result<T,E>{
        union { T val; E err } storage;
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
        friend class result<Ut, Et>;

        /* factory */
        template<class U>
            requires (is_constructible_v<T, U>)
        result(ok_t<U>&& ok): _ok(true){
            new(&storage.val) T(*ok);
        }

        template<class U>
            requires (is_constructible_v<E, U>)
        result(err_t<U>&& err): _ok(true){
            new(&storage.err) T(*err);
        }

        /* copy */
        template<class Ut, class Et>
            requires (is_constructible_v<T, Ut> && is_constructible_v<E, Et>)
        result(result<U, E>&& other): _ok(false){
            if(other._ok){
                new(&storage.val) E(other.borrow_value());
                _ok = true;
            } else {
                new(&storage.err) E(other.borrow_error());
                _ok = false;
            }
        }
        template<class Ut, class Et>
            requires (is_constructible_v<T, Ut> && is_constructible_v<E, Et>)
        result&(result<U, E>&& other){
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
            requires (is_constructible_v<T, Ut> && is_constructible_v<E, Et>)
        result(result<U, E>&& other): _ok(false){
            if(other._ok){
                new(&storage.val) E(other.take_value());
                _ok = true;
            } else {
                new(&storage.err) E(other.take_error());
                _ok = false;
            }
        }

        template<class Ut, class Et>
            requires (is_constructible_v<T, Ut> && is_constructible_v<E, Et>)
        result& operator (result<U, E>&& other){
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
        bool is_err() const { return !_ok; }
        operator bool() const { return is_ok(); }
        bool operator!() const { return is_err(); }

        /* forwarding */
        err_t<E> propogate() {
            return err_t<E>(this->take_error());
        }

        /* take */
        T take_value() && {
            #ifdef __cpp_exceptions
            if(is_err()) throw std::runtime_error("cannot take value from erreneous result");
            #endif
            return move(storage.val);
        }
        E take_error() && {
            #ifdef __cpp_exceptions
            if(is_ok()) throw std::runtime_error("cannot take error from correct result");
            #endif
            return move(storage.err);
        }

        /* borrow */
        T& borrow_value() { 
            #ifdef __cpp_exceptions
            if(is_err()) throw std::runtime_error("cannot borrow value from erroneous result");
            #endif
            return storage.val;
        }
        T& borrow_error() { 
            #ifdef __cpp_exceptions
            if(is_ok()) throw std::runtime_error("cannot borrow error from correct result");
            #endif
            return storage.val;
        }
        const T& borrow_value() const { 
            #ifdef __cpp_exceptions
            if(is_err()) throw std::runtime_error("cannot borrow value from erroneous result");
            #else
            #endif
            return storage.val;
        }
        const T& borrow_error() const { 
            #ifdef __cpp_exceptions
            if(is_ok()) throw std::runtime_error("cannot borrow error from correct result");
            #else
            #endif
            return storage.val;
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

        template<class Fn, class Gn
            class FResult = invoke_result_t<Fn, T&&>,
            class GResult = invoke_result_t<Gn, E&&>
        > requires (is_same_v<FResult, GResult>)
        FResult then_or_else(Fn&& fn, Gn&& gn) {
            if(is_ok())
                return invoke(forward<Fn>(fn), take_value());
            else
                return invoke(forward<Gn>(gn), take_error());
        }

        template<class Fn, class G
            class Result = invoke_result_t<Fn, T&&>,
        > requires (is_same_v<Result, G>)
        G then_or_default(Fn&& fn, G&& g) {
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
            requires (is_same_v<T, G>)
        T take_or_default(Fn&& fn, G&& g) {
            if(is_ok()) return take_value();
            else return forward<G>(g);  
        }

        template<class Fn, class Result = invoke_result_t<Fn, E&&>,> 
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
        result& inspect(Fn&& fn) const {
            if(is_ok()) invoke(forward<Fn>(fn), borrow_value());
            return *this;
        }

        template<class Fn>
        result& inspect_error(Fn&& fn) const {
            if(is_error()) invoke(forward<Fn>(fn), borrow_error());
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

    /* ========== result<T&, E> ========== */
    template<class TRef, class E>
        requires (is_reference_v<TRef>)
    class result<TRef,E>{
        using T = remove_reference_t<TRef>;
        T* ref;
        char err_buf[sizeof(E)];
    public:
        using value_type = TRef;
        using error_type = E;
        /* constructors */

        /* copy */

        /* move */
        
        /* validity */
        bool is_ok() const { return ref != nullptr; }
        bool is_err() const { return ref == nullptr; }
        operator bool() const { return is_ok(); }
        bool operator!() const { return is_err(); }

        /* forwarding */
        err_t<E> propogate() {
            return err_t<E>(this->take_error());
        }

        /* ownership */


        /* operation */
        then

        then_or_else

        then_or_default

        default_or_else

        then_apply

        or_apply

        inspect

        inspect_error
    };

    template<class E>
    class result<void,E>{
    public:
        using value_type = void;
        using error_type = E;

        /* constructors */

        /* copy */

        /* move */
        
        /* validity */
        bool is_ok() const { return ref != nullptr; }
        bool is_err() const { return ref == nullptr; }
        operator bool() const { return is_ok(); }
        bool operator!() const { return is_err(); }

        /* forwarding */
        err_t<E> propogate() {
            return err_t<E>(this->take_error());
        }

        /* ownership */
        

        /* operation */
        then

        then_or_else

        then_or_default

        default_or_else

        then_apply

        or_apply

        inspect

        inspect_error
    };


    /* ========== oK_t and err_t ========== */
    template<class T> requires (!is_reference_v<T> && !is_void_v<T>)
    class ok_t<T> {
        T val;
    public:
        template<class... Args> requires (is_constructible_v<T, Args...>)
        ok_t(Args&&... args): val(forward<Args>(args)...){}
        T&& operator*() && { return move(val); }
    };

    template<class TRef>
        requires (is_reference_v<TRef>)
    struct ok_t<TRef> {
        T* ref;
    public:
        template<class URef> requires (is_reference_v<URef> && is_convertible_v<URef, TRef>)
        ok_t(URef ref): ref(&ref){}
        operator T*() && { return ref; }
    };

    template<typename T>
    using result_ok = ok_t<T>;

    template<>
    struct ok_t<void>{};
    constexpr ok_t<void> Ok;

    template<class E>
    struct err_t<E> {
        E err;
    public:
        template<class... Args> requires (is_constructible_v<E, Args...>)
        err_t(Args&&... args): err(forward<Args>(args)...){}
        E&& operator*() && { return move(err); }
    };

    template<class E>
    using result_error = err_t<E>;
}