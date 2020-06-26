//
// Created by dcnick3 on 6/24/20.
//

#ifndef UWIN_BASIC_BLOCK_COMPILER_H
#define UWIN_BASIC_BLOCK_COMPILER_H

#include "uwin-jit/sljit.h"
#include "uwin-jit/basic_block.h"
#include "Zydis/Zydis.h"

#include "uwin/mem.h"

namespace uwin {
    namespace jit {

        class basic_block_compiler;

        class basic_block_tmp_value {
            int slot_index;
            basic_block_compiler& compiler;

        public:
            inline sljit_ref get_ref() const {
                return sljit_ref::reg(get_reg());
            }

            inline sljit_reg get_reg() const {
                return static_cast<sljit_reg>(static_cast<int>(sljit_reg::R0) + slot_index);
            }

            inline operator sljit_ref() const {
                return get_ref();
            }

            inline operator sljit_reg() const {
                return get_reg();
            }

            inline basic_block_tmp_value(int index, basic_block_compiler& compiler) : slot_index(index),
                compiler(compiler) {
            }
            inline ~basic_block_tmp_value();
        };

        enum class op_size {
            S_8,
            S_16,
            S_32,
        };

        static inline sljit::sljit_s32 get_sized_mov(op_size sz) {
            switch (sz) {
                case op_size::S_8:
                    return SLJIT_MOV32_U8;
                case op_size::S_16:
                    return SLJIT_MOV32_U16;
                case op_size::S_32:
                    return SLJIT_MOV32;
            }
            throw new std::exception();
        }

        class basic_block_compiler : sljit_compiler {
            uint32_t guest_address;
            uint8_t* host_address;
            size_t block_size;
            cpu_static_context& ctx;

            bool used_tmp_slots[6];

            basic_block_compiler(cpu_static_context& ctx, uint32_t guest_address, uint8_t* host_address);

            void compile_instruction(ZydisDecodedInstruction& instr);

            void dump_instruction(ZydisDecodedInstruction& instr);

            // operands must be already zero-extended
            void emit_arith_with_flags(op_size sz, sljit::sljit_s32 op, sljit_ref dst, sljit_ref src1, sljit_ref src2,
                                       bool update_carry = true);

            void emit_load_operand(sljit_ref dst, ZydisDecodedOperand& op);
            void emit_store_operand(ZydisDecodedOperand& op, sljit_ref src);

            void emit_store_memory(op_size sz, cpu_segment seg, sljit_ref offset, sljit_ref src);
            void emit_load_memory(op_size sz, sljit_ref dst, cpu_segment seg, sljit_ref offset);

            static inline sljit_reg ctx_reg() {
                return sljit_reg::S0;
            }

            static inline sljit_ref ref_reg(cpu_reg reg) {
                return sljit_ref::mem1(ctx_reg(), offsetof(cpu_context, regs) +
                        static_cast<int>(reg) * sizeof(sljit::sljit_s32));
            }

            inline basic_block_tmp_value alloc_tmp() {
                for (int i = 0; i < sizeof(used_tmp_slots) / sizeof(*used_tmp_slots); i++)
                    if (!used_tmp_slots[i]) {
                        used_tmp_slots[i] = true;
                        return basic_block_tmp_value(i, *this);
                    }

                throw std::exception();
            }

        public:

            inline void free_tmp(int slot) {
                used_tmp_slots[slot] = false;
            }

            basic_block compile();

            inline static basic_block compile(cpu_static_context& ctx, uint32_t guest_address) {
                basic_block_compiler compiler(ctx, guest_address, static_cast<uint8_t*>(g2h(guest_address)));
                return compiler.compile();
            }
        };


        basic_block_tmp_value::~basic_block_tmp_value() {
            compiler.free_tmp(slot_index);
        }
    }
}

#endif //UWIN_BASIC_BLOCK_COMPILER_H
