#pragma once

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
        /* operator */
        const char& operator[](size_t i) const {
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
        consteval auto slice() const {
            static_assert(beg < end, "invalid slice size");
            const size_t bound = end > N ? N : end;
            char sliced[bound - beg];
            for(size_t i = beg; i < bound; i++)
                sliced[i] = str[i];
            return comptime_string<bound - beg>(sliced);
        }

        operator const char*() const {
            return str;
        }

    };
}