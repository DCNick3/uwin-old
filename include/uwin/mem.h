
#include <stdint.h>


extern void* guest_base;
#define TARGET_ADDRESS_SPACE_SIZE 0x80000000

#define g2h(x) ((void*)((uintptr_t)guest_base + (uintptr_t)(uint32_t)(x)))
#define h2g(x) ((uint32_t)((uintptr_t)(void*)(x) - (uintptr_t)guest_base))

