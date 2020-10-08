//
// Created by dcnick3 on 6/18/20.
//

#include "uwin-jit/basic_block_compiler.h"

#include <fstream>
#include <mutex>
#include <unordered_map>

namespace uwin {
    namespace jit {

        static std::unordered_map<uint32_t, std::unique_ptr<basic_block>> cache;

        static basic_block* compile_block(cpu_static_context& ctx, uint32_t eip) {
            return (cache[eip] = std::unique_ptr<basic_block>(new basic_block(std::move(native_basic_block_compiler::compile(ctx, eip))))).get();
        }

        // As much as possible
        static const int max_depth = 512;

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

        void init_basic_block_cache() {
            cache.clear();
            cache.max_load_factor(0.01);
            cache.reserve(100000);
        }

        basic_block* get_native_basic_block(cpu_static_context& ctx, uint32_t eip) {
            return get_native_basic_block_recur(ctx, eip, 0);
        }

        void dump_basic_blocks() {
            std::ofstream f("basic_block_addresses.txt", std::ios::out);
            for (const auto& p : cache) {
                f << std::hex << p.first << std::endl;
            }
        }
    }
}
