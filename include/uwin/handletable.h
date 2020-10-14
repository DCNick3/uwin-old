//
// Created by dcnick3 on 10/14/20.
//

#pragma once

#include "uwin/uwin.h"
#include "uwin/kobj/kobj.h"
#include "uwin/kobj/kdummy.h"


void uw_ht_initialize(void);
void uw_ht_finalize(void);
uint32_t uw_ht_put(std::unique_ptr<uwin::kobj> obj);
uwin::kobj *uw_ht_get(uint32_t handle);
uint32_t uw_ht_put_dummy(uint32_t obj, int type);
uint32_t uw_ht_get_dummy(uint32_t handle, int type);
int uw_ht_get_dummytype(uint32_t handle);
uint32_t uw_ht_newref(uint32_t handle);
void uw_ht_delref(uint32_t handle, uint32_t *dummy_type, uint32_t *dummy_value);

namespace uwin {
    template<typename T, typename... Args>
    inline uint32_t ht_put_new(Args&&... args)
    {
        return uw_ht_put(std::unique_ptr<T>(new T(std::forward<Args>(args)...)));
    }

    template<typename T>
    inline T& ht_get(uint32_t handle)
    {
        auto ptr = uw_ht_get(handle);
        auto conv_ptr = ptr->try_as<T>();
        if (conv_ptr == nullptr)
            std::terminate();
        return *conv_ptr;
    }
}