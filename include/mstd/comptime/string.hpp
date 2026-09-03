#pragma once
#include <utility>

namespace mstd {
    template<size_t N>
    class comptime_string {
        char str[N];
    public:
        template<size_t M>
        consteval comptime_string(comptime_string<M>& src) {
            size_t cap = N > M ? M : N;
            for(size_t i = 0; i < cap; i++)
                str[i] = src[i];
        }

        template<size_t M>
        consteval comptime_string(const char (&src)[M]) {
            size_t cap = N > M ? M : N;
            for(size_t i = 0; i < cap; i++)
                str[i] = src[i];
        }

        template<size_t i, char c>
        consteval auto split() const {
            if constexpr(i == N) {
                return std::make_pair(substr<0,i>(), comptime_string<0>());
            } else if constexpr (str[i] == c){
                return std::make_pair(substr<0,i>(), substr<i+1,N>());
            } else {
                return split<i+1, c>();
            }
        }

        template<char c>
        consteval auto split() const {
            return split<0,c>();
        }

        /* operator */
        consteval char& operator[](size_t i) const {
            return str[i];
        }

        /* append */
        template<size_t M>
        consteval auto append(const char (&other)[M]) const {
            char new_str[N + M - 1];
            for(size_t i = 0; i < N - 1; i++)
                new_str[i] = str[i];
            for(size_t i = 0; i < M; i++)
                new_str[N + i - 1] = other[i];
            return comptime_string<N + M - 1>(new_str);
        }
        template<size_t M>
        consteval auto append(comptime_string<M> other) const {
            char new_str[N + M - 1];
            for(size_t i = 0; i < N - 1; i++)
                new_str[i] = str[i];
            for(size_t i = 0; i < M; i++)
                new_str[N + i - 1] = other[i];
            return comptime_string<N + M - 1>(new_str);
        }

        /* slice */
        template<size_t beg, size_t end>
        consteval auto substr() const {
            static_assert(beg < end, "invalid slice size");
            if(beg >= N) return comptime_string<0>();
            const size_t bound = end > N ? N : end;
            char sliced[bound - beg + 1];
            for(size_t i = beg; i < bound; i++)
                sliced[i - beg] = str[i];
            sliced[bound - beg] = '\0';
            return comptime_string<bound - beg + 1>(sliced);
        }

        operator const char*() const {
            return str;
        }
    };

    template<>
    class comptime_string<0> {
    public:
        consteval size_t len() const { return 0; }   
    };
}