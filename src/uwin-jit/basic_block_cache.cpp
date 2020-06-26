//
// Created by dcnick3 on 6/18/20.
//

#include "uwin-jit/basic_block_cache.h"
#include "uwin-jit/basic_block_compiler.h"

#include <mutex>

namespace uwin {
    namespace jit {

        // no cache yet =)
        std::shared_ptr<basic_block> get_basic_block(cpu_static_context& ctx, uint32_t eip) {
            return std::make_shared<basic_block>(std::move(basic_block_compiler::compile(ctx, eip)));
        }

    }
}
