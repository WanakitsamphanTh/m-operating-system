#pragma once
#include <type_traits>

namespace MK {
    struct Bootstrap{
        static constexpr bool uses_vaddr = false;
    };
    struct Permanent{
        static constexpr bool uses_vaddr = true;
    };
    template<class T>
    concept KernelSession
        = std::is_same_v<T, Bootstrap>
        || std::is_same_v<T, Permanent>;

};