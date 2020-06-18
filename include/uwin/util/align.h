
#ifndef UWIN_ALIGNUTILS_H 
#define UWIN_ALIGNUTILS_H

#include <stdint.h>

#define UW_IS_ALIGNED(x, a) (((uintptr_t)(x)) % (a) == 0)

#define UW_ALIGN_DOWN(n, m) ((n) / (m) * (m))
#define UW_ALIGN_UP(n, m) UW_ALIGN_DOWN((n) + (m) - 1, (m))

#endif
