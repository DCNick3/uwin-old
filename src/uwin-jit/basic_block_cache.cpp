//
// Created by dcnick3 on 6/18/20.
//

#include "uwin-jit/basic_block_compiler.h"

#include <mutex>
#include <unordered_map>

namespace uwin {
    namespace jit {

        static std::unordered_map<uint32_t, std::unique_ptr<basic_block>> cache;

        static basic_block* compile_block(cpu_static_context& ctx, uint32_t eip) {
            return (cache[eip] = std::unique_ptr<basic_block>(new basic_block(std::move(native_basic_block_compiler::compile(ctx, eip))))).get();
        }

        // this number was selected by fine-tuning on raspberry pi 3b+ in RelWithDebInfo configuration running game initialization until main menu
        // (first shrd instruction)
        static const int max_depth = 32;

        static basic_block* get_native_basic_block_recur(cpu_static_context& ctx, uint32_t eip, int depth) {
            auto it = cache.find(eip);
            if (it == cache.end()) {
                auto block = compile_block(ctx, eip);

#ifndef UW_USE_JITFIX
                if (depth < max_depth) {
                    for (auto& p : block->get_out_predictions()) {
                        p.second = get_native_basic_block_recur(ctx, p.first, depth + 1);
                    }
                }
#endif
                return block;
            } else {
                return it->second.get();
            }
        }

        basic_block* get_native_basic_block(cpu_static_context& ctx, uint32_t eip) {
            return get_native_basic_block_recur(ctx, eip, 0);
        }

    }
}
