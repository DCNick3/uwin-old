#pragma once


#include <cstdint>
#include <cstdlib>
#include <type_traits>

namespace uwin {

    namespace detail {
        template<typename T>
        static constexpr inline bool is_alignable() {
            return std::is_convertible<T, std::uintptr_t>::value;
        }
    }

    template<typename T>
    static inline bool is_aligned(T val, std::size_t alignment) {
        static_assert(detail::is_alignable<T>(), "Type is not alignable");

        return val % alignment == 0;
    }

    template<typename T>
    static inline T align_down(T val, std::size_t alignment) {
        static_assert(detail::is_alignable<T>(), "Type is not alignable");

        return val / alignment * alignment;
    }

    template<typename T>
    static inline T align_up(T val, std::size_t alignment) {
        static_assert(detail::is_alignable<T>(), "Type is not alignable");

        auto ptr1 = val + alignment - 1;
        return align_down(ptr1, alignment);
    }

}
