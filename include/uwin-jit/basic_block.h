//
// Created by dcnick3 on 6/18/20.
//

#ifndef UWIN_BASIC_BLOCK_H
#define UWIN_BASIC_BLOCK_H

#include <cstdint>

#include "cpu_context.h"

#include "uwin-jit/sljit.h"


namespace uwin {
    namespace jit {
        typedef void (SLJIT_FUNC *basic_block_func)(cpu_context* ctx);

        class basic_block {
            uint32_t guest_address;
            uint32_t size;

            sljit_code<basic_block_func> code;

        public:
            inline basic_block(uint32_t guest_address, uint32_t size, sljit_code<basic_block_func> code)
                : guest_address(guest_address), size(size), code(std::move(code))
            {}

            inline void SLJIT_FUNC execute(cpu_context* ctx) {
                code.get_pointer()(ctx);
            }
        };
    }
}


#endif //UWIN_BASIC_BLOCK_H
