
#include <stdint.h>


extern void* guest_base;
#define TARGET_ADDRESS_SPACE_SIZE 0x80000000

#define g2h(x) ((void*)(guest_base + (unsigned long)(uint32_t)(x)))
#define h2g(x) ((uint32_t)((unsigned long)(void*)(x) - (unsigned long)guest_base))

