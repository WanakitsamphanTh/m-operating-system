#pragma once 
#include <mstd/monadic/result.hpp>
#include <mstd/lazy.hpp>
#include <utility>
#include <type_traits>

namespace mstd {

    using std::move;
    using std::forward;
    using std::is_invocable_r_v;
    using std::invoke;
    using std::is_void_v;
    
    enum class lazy_result_state {
        unevaluated,
        ok,
        err
    };

    template<class T, class E, class Fn>
        requires (is_invocable_r_v<result<T, E>&&, Fn>)
    class lazy<result<T, E>, Fn> {
        mutable lazy_result_state state;
        mutable union {
            result<T, E> res;
            Fn fn;
        } storage;

        void eval() const {
            if (state == lazy_result_state::unevaluated) {
                Fn fn = move(storage.fn);
                storage.fn.~Fn();
                invoke(fn, &storage.res);
                if(storage.res.is_ok()) {
                    state = lazy_result_state::ok;
                } else {
                    state = lazy_result_state::err;
                }
            }
        }

    public:

        template<class Ut, class Et, class Fn2>
        friend class lazy<result<Ut, Et>, Fn2>; 

        /* constructor */
        template<class Fn>
            requires (is_invocable_r_v<result<T, E>&&, Fn>)
        lazy(Fn&& fn): state(lazy_result_state::unevaluated) {
            new(&storage.fn) Fn(std::forward<Fn>(fn));
        }

        lazy(result<T, E>&& value): state(lazy_result_state::unevaluated) {
            new(&storage.res) result<T, E>(move(value));
            if(storage.res.is_ok()) {
                state = lazy_result_state::ok;
            } else {
                state = lazy_result_state::err;
            }
        }

        /* constructors from ok_t and err_t */
        template<class Ut>
            requires (is_constructible_v<result<T, E>, ok_t<Ut>>)
        lazy(ok_t<Ut>&& res): state(lazy_result_state::unevaluated) {
            new(&storage.res) result<T, E>(std::forward<ok_t<Ut>>(res));
            state = lazy_result_state::ok;
        }

        template<class Et>
            requires (is_constructible_v<result<T, E>, err_t<Et>>)
        lazy(err_t<Et>&& res): state(lazy_result_state::unevaluated) {
            new(&storage.res) result<T, E>(std::forward<err_t<Et>>(res));
            state = lazy_result_state::err;
        }

        /* move constructor*/
        lazy(const lazy&) = delete;
        lazy& operator=(const lazy&) = delete;

        template<class Ut, class Et, class Fn2>
            requires (is_constructible_v<result<T, E>, result<Ut, Et>>)
        lazy(lazy<result<Ut, Et>, Fn2>&& other) noexcept : state(lazy_result_state::unevaluated) {
            if (other.state == lazy_result_state::unevaluated) {
                new(&storage.fn) Fn(move(other.storage.fn));
            } else {
                new(&storage.res) result<T, E>(move(other.storage.res));
                if(storage.res.is_ok()) {
                    state = lazy_result_state::ok;
                } else {
                    state = lazy_result_state::err;
                }
            }
            state = other.state;
        }

        template<class Ut, class Et, class Fn2>
            requires (is_constructible_v<result<T, E>, result<Ut, Et>>)
        lazy& operator=(lazy<result<Ut, Et>, Fn2>&& other) noexcept {
            if (this == &other) return *this;     
            this->~lazy();
            new(this) lazy(move(other));
            return *this;
        }

        /* destructor */
        ~lazy() {
            if(state == lazy_result_state::unevaluated) {
                storage.fn.~Fn();
            } else {
                storage.res.~result<T,E>();
            }
        }

        /* validity */
        bool is_ok() const {
            eval();
            return state == lazy_result_state::ok;
        }

        bool is_err() const {
            eval();
            return state == lazy_result_state::err;
        }

        operator bool() const {
            eval();
            return state == lazy_result_state::ok;
        }

        bool operator!() const {
            eval();
            return state == lazy_result_state::err;
        }

        lazy_result_state get_state() const {
            return state;
        }

        /* type conversion */
        operator result<T, E>() const {
            eval();
            return move(storage.res);
        }

        /* ownership */
        template<class U = T>
            requires (!is_void_v<T>)
        auto take_value() {
            eval();
            return storage.res.take_value();
        }

        E take_error() {
            eval();
            return storage.res.take_error();
        }

        template<class U = T>
            requires (!is_void_v<T>)
        auto borrow_value() {
            eval();
            return storage.res.borrow_value();
        }

        E& borrow_error() {
            eval();
            return storage.res.borrow_error();
        }

        template<class U = T>
            requires (!is_void_v<T>)
        auto borrow_value_const() const {
            eval();
            return storage.res.borrow_value_const();
        }

        const E& borrow_error_const() const {
            eval();
            return storage.res.borrow_error_const();
        }

        /* propogation */
        err_T<E> propogate() {
            eval();
            return storage.res.propogate();
        }

        /* chained operations*/
        
    };
}