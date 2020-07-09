//
// Created by dcnick3 on 6/18/20.
//

#include "uwin-jit/basic_block_cache.h"
#include "uwin-jit/basic_block_compiler.h"

#include <mutex>
#include <unordered_map>

namespace uwin {
    namespace jit {

        static std::unordered_map<uint32_t, std::unique_ptr<basic_block>> cache;

        static std::unique_ptr<basic_block> compile_block(cpu_static_context& ctx, uint32_t eip) {
            return std::unique_ptr<basic_block>(new basic_block(std::move(native_basic_block_compiler::compile(ctx, eip))));
        }

        // the most dumb cache ever
        basic_block* get_native_basic_block(cpu_static_context& ctx, uint32_t eip) {
            //return compile_block(ctx, eip);

            auto it = cache.find(eip);
            if (it == cache.end()) {
                return (cache[eip] = compile_block(ctx, eip)).get();
            } else {
                return it->second.get();
            }
        }

    }
}
