//
// Created by dcnick3 on 6/18/20.
//

#ifndef UWIN_BASIC_BLOCK_H
#define UWIN_BASIC_BLOCK_H

#include <cstdint>
#include <algorithm>
#include <atomic>

#include "cpu_context.h"

#include "uwin-jit/executable_memory_allocator.h"

#include "uwin/util/log.h"


namespace uwin {
    namespace jit {
        typedef cpu_context* (*basic_block_func)(cpu_context* ctx);

        class basic_block {
            uint32_t guest_address;
            uint32_t size;

            xmem_piece xmem;

#ifdef UW_USE_JITFIX
            void halfix_enter(cpu_context* ctx);
            void halfix_leave(cpu_context* ctx);
#endif
        public:
            inline basic_block(uint32_t guest_address, uint32_t size, xmem_piece xmem)
                : guest_address(guest_address), size(size), xmem(std::move(xmem))
            {}

            inline void execute(cpu_context* ctx) {
#ifdef UW_JIT_TRACE
                uw_log("entering basic block %08lx | %p\n", (unsigned long)guest_address, xmem.rx_ptr());
#endif

#ifdef UW_USE_JITFIX
                halfix_enter(ctx);
#endif

                xmem.prepare_execute();
                auto func = reinterpret_cast<basic_block_func>(xmem.rx_ptr());

                if (guest_address == 0x055BD6B) {
                    uw_log("bonk\n");
                }

                func(ctx);

#ifdef UW_USE_JITFIX
                halfix_leave(ctx);
#endif
            }
        };
    }
}


#endif //UWIN_BASIC_BLOCK_H
