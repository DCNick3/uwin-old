//
// Created by dcnick3 on 10/14/20.
//

#pragma once

#include <memory>

namespace uwin {
    class kobj {

    public:
        template<typename T>
        T& as() {
            auto* ptr = try_as<T>();
            if (ptr == nullptr)
                std::terminate();
            return *ptr;
        }

        template<typename T>
        T* try_as() {
            return dynamic_cast<T*>(this);
        }

        template<typename T>
        bool is() {
            return try_as<T>() != nullptr;
        }

        virtual ~kobj() = default;
    };
}
