//
// Created by dcnick3 on 6/18/20.
//

#ifndef UWIN_BASIC_BLOCK_H
#define UWIN_BASIC_BLOCK_H

#include <cstdint>
#include <algorithm>
#include <atomic>
#include <vector>

#include "cpu_context.h"

#include "uwin-jit/executable_memory_allocator.h"

#include "uwin/util/log.h"



namespace uwin {
    namespace jit {
        typedef cpu_context* (*basic_block_func)(cpu_context* ctx);

        class basic_block;

        basic_block* get_native_basic_block(cpu_static_context& ctx, uint32_t eip);

        class basic_block {
            uint32_t guest_address;
            uint32_t size;

            xmem_piece xmem;

            std::vector<std::pair<std::uint32_t, basic_block*>> out_predictions;

#ifdef UW_USE_JITFIX
            void halfix_enter(cpu_context* ctx);
            void halfix_leave(cpu_context* ctx);
#endif
        public:
            inline basic_block(uint32_t guest_address, uint32_t size, xmem_piece xmem, const std::vector<uint32_t> out_pred)
                : guest_address(guest_address), size(size), xmem(std::move(xmem))
            {
                this->xmem.prepare_execute();
                for (auto addr : out_pred) {
                    out_predictions.emplace_back(addr, nullptr);
                }
            }

            inline void execute(cpu_context* ctx) {
#ifdef UW_JIT_TRACE
                uw_log("entering basic block %08lx | %p\n", (unsigned long)guest_address, xmem.rx_ptr());
#endif

#ifdef UW_USE_JITFIX
                halfix_enter(ctx);
#endif
                auto func = reinterpret_cast<basic_block_func>(xmem.rx_ptr());

                if (guest_address == 0x010771cc) {
                    uw_log("bonk\n");
                }

                func(ctx);

#ifdef UW_USE_JITFIX
                halfix_leave(ctx);
#endif
            }

            inline basic_block* get_native_basic_block_predicted(cpu_static_context& ctx, uint32_t eip) {
                for (auto p : out_predictions) {
                    if (p.first == eip) {
                        if (p.second != nullptr)
                            return p.second;
                        break;
                    }
                }
                return get_native_basic_block(ctx, eip);
            }

            inline std::vector<std::pair<std::uint32_t, basic_block*>>& get_out_predictions() {
                return out_predictions;
            }
        };
    }
}


#endif //UWIN_BASIC_BLOCK_H
