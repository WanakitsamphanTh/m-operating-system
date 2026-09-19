#pragma once

#include <type_traits>
namespace mstd {
    struct Bootstrap{};
    struct Permanent{};
    template<class T>
    concept KernelSession
        = std::is_same_v<T, Bootstrap>
        || std::is_same_v<T, Permanent>;
};