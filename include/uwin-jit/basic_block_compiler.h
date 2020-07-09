//
// Created by dcnick3 on 6/24/20.
//

#ifndef UWIN_BASIC_BLOCK_COMPILER_H
#define UWIN_BASIC_BLOCK_COMPILER_H

#include "uwin-jit/basic_block.h"
#include "uwin-jit/aarch64_code_generator.h"
#include "Zydis/Zydis.h"

#include "uwin/mem.h"

#include "uwin/util/log.h"

namespace uwin {
    namespace jit {


        template<typename C>
        class basic_block_compiler {
            uint32_t guest_address;
            uint8_t* host_address;
            size_t block_size;
            const cpu_static_context& ctx;
            C code_generator;

            bool used_tmp_slots[6];

            basic_block_compiler(const cpu_static_context& ctx, uint32_t guest_address, uint8_t *host_address)
                    : ctx(ctx), guest_address(guest_address), host_address(host_address), code_generator(ctx) {}

            void compile_instruction(ZydisDecodedInstruction& instr)
            {
            }

            void dump_instruction(ZydisDecodedInstruction& instr) {
                ZydisFormatter formatter;
                ZydisFormatterInit(&formatter, ZYDIS_FORMATTER_STYLE_ATT);

                char buffer[256];
                ZydisFormatterFormatInstruction(&formatter, &instr, buffer, sizeof(buffer),
                                                guest_address - instr.length);

                uw_log("%08x: %s\n", guest_address - instr.length, buffer);
            }



        public:


            inline void free_tmp(int slot) {
                used_tmp_slots[slot] = false;
            }

            inline basic_block compile() {

                ZydisDecoder decoder;
                ZydisDecoderInit(&decoder, ZYDIS_MACHINE_MODE_LEGACY_32,
                                 ZYDIS_ADDRESS_WIDTH_32);

                code_generator.emit_prologue();

                ZydisDecodedInstruction instruction;

                uint32_t block_guest_address = guest_address;

                while (true) {
                    ZyanStatus res = ZydisDecoderDecodeBuffer(&decoder, host_address, 100, &instruction);
                    assert(ZYAN_SUCCESS(res));

                    host_address += instruction.length;
                    guest_address += instruction.length;
                    block_size += instruction.length;

#ifdef UW_JIT_TRACE
                    dump_instruction(instruction);
#endif

                    if (!code_generator.emit_instruction(guest_address, instruction, block_size > 0x10000))
                        break;
                }

                code_generator.emit_epilogue();

                return basic_block(block_guest_address, block_size, std::move(code_generator.commit()));
            }

            inline static basic_block compile(cpu_static_context& ctx, uint32_t guest_address) {
                basic_block_compiler compiler(ctx, guest_address, static_cast<uint8_t*>(g2h(guest_address)));
                return compiler.compile();
            }
        };

        typedef aarch64_code_generator native_code_generator;
        typedef basic_block_compiler<aarch64_code_generator> native_basic_block_compiler;

    }
}

#endif //UWIN_BASIC_BLOCK_COMPILER_H
