//
// Created by dcnick3 on 6/18/20.
//

#ifndef UWIN_BASIC_BLOCK_H
#define UWIN_BASIC_BLOCK_H

#include <cstdint>
#include <algorithm>
#include <atomic>
#include <vector>
#include <fstream>
#include <sstream>
#include <unistd.h>

#include "cpu_context.h"

#include "uwin-jit/executable_memory_allocator.h"

#include "uwin/util/log.h"



namespace uwin {
    namespace jit {
#ifdef UW_USE_GDB_JIT_INTERFACE
        void gdb_jit_register_object(std::vector<uint8_t>& vector);
#endif

        typedef cpu_context* (*basic_block_func)(cpu_context* ctx);

        class basic_block;

        void init_basic_block_cache();
        basic_block* get_native_basic_block(cpu_static_context& ctx, uint32_t eip);

#ifdef UW_JIT_GENERATE_PERF_MAP
        class perf_map_writer {
            std::ofstream file;
        public:
            perf_map_writer() : file("/tmp/perf-" + std::to_string(getpid()) + ".map", std::ios::out) {
            }
            void write_entry(size_t address, size_t size, std::string name) {
                file << std::hex << address << " " << size << " " << name << std::endl;
            }
        };

        extern perf_map_writer perf_map_writer_singleton;
#endif

        class basic_block {
            uint32_t guest_address;
            uint32_t size;

            xmem_piece xmem;

            std::vector<std::pair<std::uint32_t, basic_block*>> out_predictions;

#ifdef UW_USE_GDB_JIT_INTERFACE
            std::vector<std::uint8_t> gdb_jit_elf;

            std::vector<std::uint8_t> make_gdb_elf();
#endif

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
#ifdef UW_USE_GDB_JIT_INTERFACE
                gdb_jit_elf = make_gdb_elf();

                /*std::stringstream file_name;
                file_name << "basic_block_0x";
                file_name << std::hex << guest_address;
                file_name << ".elf";
                std::ofstream outfile(file_name.str(), std::ios::out | std::ios::binary);
                outfile.write(reinterpret_cast<const char*>(gdb_jit_elf.data()), gdb_jit_elf.size());*/

                gdb_jit_register_object(gdb_jit_elf);
#endif
#ifdef UW_JIT_GENERATE_PERF_MAP
                std::stringstream stream;
                stream << std::hex << "uw_jit_bb_" << guest_address;
                perf_map_writer_singleton.write_entry(reinterpret_cast<size_t>(this->xmem.rx_ptr()), this->xmem.size(), stream.str());
#endif
            }

            inline void execute(cpu_context* ctx) {
#ifdef UW_JIT_TRACE
                uw_log("entering basic block %08lx | %p\n", (unsigned long)guest_address, xmem.rx_ptr());
#endif

#ifdef UW_USE_JITFIX
                halfix_enter(ctx);
#endif
                auto func = reinterpret_cast<basic_block_func>(xmem.rx_ptr());

                if (guest_address == 0x00050c70b) {
                    //uw_log("bonk\n");
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
