//
// Created by dcnick3 on 6/18/20.
//

#ifndef UWIN_BASIC_BLOCK_CACHE_H
#define UWIN_BASIC_BLOCK_CACHE_H

#include <memory>

#include "uwin-jit/basic_block.h"

namespace uwin {
    namespace jit {

        std::shared_ptr<basic_block> get_basic_block(cpu_static_context& ctx, uint32_t eip);

    }
}

#endif //UWIN_BASIC_BLOCK_CACHE_H
