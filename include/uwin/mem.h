
#include <stdint.h>

#ifndef UWIN_UTIL_MEM_H
#define UWIN_UTIL_MEM_H

#ifdef __cplusplus
extern "C" {
#endif

extern void *guest_base;
#define TARGET_ADDRESS_SPACE_SIZE 0x80000000

#define g2h(x) ((void*)((uintptr_t)guest_base + (uintptr_t)(uint32_t)(x)))
#define h2g(x) ((uint32_t)((uintptr_t)(void*)(x) - (uintptr_t)guest_base))

#ifdef __cplusplus
}


template<typename T>
static inline T* g2hx(uint32_t addr) {
    return static_cast<T*>(g2h(addr));
}
#endif

#endif
