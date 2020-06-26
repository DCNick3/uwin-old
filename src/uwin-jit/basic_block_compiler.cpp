//
// Created by dcnick3 on 6/24/20.
//

#include "uwin-jit/basic_block_compiler.h"
#include "Zydis/Zydis.h"

#include <assert.h>

namespace uwin {
    namespace jit {
        basic_block_compiler::basic_block_compiler(cpu_static_context& ctx, uint32_t guest_address, uint8_t *host_address)
            : ctx(ctx), guest_address(guest_address), host_address(host_address)
        {

        }

        static cpu_reg zydis_reg(ZydisRegister zreg) {
            switch (zreg) {
                case ZYDIS_REGISTER_AL:
                case ZYDIS_REGISTER_EAX: return cpu_reg::EAX;
                case ZYDIS_REGISTER_EBX: return cpu_reg::EBX;
                case ZYDIS_REGISTER_ECX: return cpu_reg::ECX;
                case ZYDIS_REGISTER_EDX: return cpu_reg::EDX;
                case ZYDIS_REGISTER_ESI: return cpu_reg::ESI;
                case ZYDIS_REGISTER_EDI: return cpu_reg::EDI;
                case ZYDIS_REGISTER_ESP: return cpu_reg::ESP;
                case ZYDIS_REGISTER_EBP: return cpu_reg::EBP;

                default:
                    throw std::exception();
            }
        }

        static op_size zydis_op_size(ZydisDecodedOperand& op) {
            switch (op.size) {
                case 8:
                    return op_size::S_8;
                case 16:
                    return op_size::S_16;
                case 32:
                    return op_size::S_32;
            }
            throw std::exception();
        }

        static cpu_segment zydis_seg(ZydisRegister reg) {
            switch (reg) {
                case ZYDIS_REGISTER_CS: return cpu_segment::CS;
                case ZYDIS_REGISTER_DS: return cpu_segment::DS;
                case ZYDIS_REGISTER_ES: return cpu_segment::ES;
                case ZYDIS_REGISTER_SS: return cpu_segment::SS;
                case ZYDIS_REGISTER_FS: return cpu_segment::FS;
                case ZYDIS_REGISTER_GS: return cpu_segment::GS;
                default:
                    throw std::exception();
            }
        }

        void basic_block_compiler::compile_instruction(ZydisDecodedInstruction& instr)
        {
            switch (instr.mnemonic) {
                case ZYDIS_MNEMONIC_PUSH:
                    {
                        auto val = alloc_tmp();
                        assert(instr.operands[0].visibility == ZYDIS_OPERAND_VISIBILITY_EXPLICIT);
                        emit_load_operand(val, instr.operands[0]);
                        // TODO: make less generic, add handling for cases where one can get around w/o temporary
                        emit_op2(SLJIT_SUB32, ref_reg(cpu_reg::ESP), ref_reg(cpu_reg::ESP),
                                sljit_ref::imm(instr.operands[0].size / 8));
                        assert(instr.operands[0].size == 32);
                        emit_store_memory(op_size::S_32, cpu_segment::SS, ref_reg(cpu_reg::ESP), val);
                    }
                    break;
                case ZYDIS_MNEMONIC_POP:
                    {
                        auto val = alloc_tmp();
                        assert(instr.operands[0].size == 32);
                        emit_load_memory(op_size::S_32, val, cpu_segment::SS, ref_reg(cpu_reg::ESP));
                        emit_op2(SLJIT_ADD32, ref_reg(cpu_reg::ESP), ref_reg(cpu_reg::ESP),
                                 sljit_ref::imm(instr.operands[0].size / 8));
                        emit_store_operand(instr.operands[0], val);
                    }
                    break;
                case ZYDIS_MNEMONIC_CALL:
                    {
                        assert(instr.operands[0].visibility == ZYDIS_OPERAND_VISIBILITY_EXPLICIT);
                        assert(instr.operands[0].type == ZYDIS_OPERAND_TYPE_IMMEDIATE ||
                                instr.operands[0].type == ZYDIS_OPERAND_TYPE_REGISTER);
                        uint32_t new_eip = guest_address;
                        if (instr.operands[0].type == ZYDIS_OPERAND_TYPE_IMMEDIATE) {
                            if (!instr.operands[0].imm.is_relative)
                                new_eip = instr.operands[0].imm.value.u;
                            else
                                new_eip += instr.operands[0].imm.value.s;
                        }

                        emit_op2(SLJIT_SUB32, ref_reg(cpu_reg::ESP), ref_reg(cpu_reg::ESP), sljit_ref::imm(4));
                        emit_store_memory(op_size::S_32, cpu_segment::SS, ref_reg(cpu_reg::ESP),
                                          sljit_ref::imm(guest_address));

                        if (instr.operands[0].type == ZYDIS_OPERAND_TYPE_IMMEDIATE) {
                            emit_op1(SLJIT_MOV32, ref_reg(cpu_reg::EIP), sljit_ref::imm(new_eip));
                        } else {
                            emit_op1(SLJIT_MOV32, ref_reg(cpu_reg::EIP), ref_reg(zydis_reg(instr.operands[0].reg.value)));
                        }
                    }
                    break;
                case ZYDIS_MNEMONIC_RET:
                    {
                        assert(instr.operands[0].visibility == ZYDIS_OPERAND_VISIBILITY_HIDDEN);
                        emit_load_memory(op_size::S_32, ref_reg(cpu_reg::EIP), cpu_segment::SS, ref_reg(cpu_reg::ESP));
                        emit_op2(SLJIT_ADD32, ref_reg(cpu_reg::ESP), ref_reg(cpu_reg::ESP), sljit_ref::imm(4));
                    }
                    break;
                case ZYDIS_MNEMONIC_JMP:
                    {
                        assert(instr.operands[0].visibility == ZYDIS_OPERAND_VISIBILITY_EXPLICIT);
                        emit_load_operand(ref_reg(cpu_reg::EIP), instr.operands[0]);
                    }
                    break;
                case ZYDIS_MNEMONIC_MOV:
                    {
                        assert(instr.operands[0].visibility == ZYDIS_OPERAND_VISIBILITY_EXPLICIT);
                        assert(instr.operands[1].visibility == ZYDIS_OPERAND_VISIBILITY_EXPLICIT);
                        if ((instr.operands[0].type == ZYDIS_OPERAND_TYPE_REGISTER || instr.operands[0].type == ZYDIS_OPERAND_TYPE_MEMORY) &&
                            instr.operands[1].type == ZYDIS_OPERAND_TYPE_REGISTER) {
                            assert(instr.operands[0].size == 32);

                            emit_store_operand(instr.operands[0],
                                    ref_reg(zydis_reg(instr.operands[1].reg.value)));
                        } else if ((instr.operands[0].type == ZYDIS_OPERAND_TYPE_REGISTER || instr.operands[0].type == ZYDIS_OPERAND_TYPE_MEMORY) &&
                                   instr.operands[1].type == ZYDIS_OPERAND_TYPE_IMMEDIATE) {
                            assert(!instr.operands[1].imm.is_relative);

                            emit_store_operand(instr.operands[0], sljit_ref::imm(instr.operands[1].imm.value.u));
                        } else if (instr.operands[0].type == ZYDIS_OPERAND_TYPE_REGISTER &&
                                   instr.operands[1].type == ZYDIS_OPERAND_TYPE_MEMORY) {

                            emit_load_operand(ref_reg(zydis_reg(instr.operands[0].reg.value)),
                                    instr.operands[1]);
                        } else
                            goto fail;
                    }
                    break;
                case ZYDIS_MNEMONIC_SUB:
                    {
                        //assert(instr.operands[0].visibility == ZYDIS_OPERAND_VISIBILITY_EXPLICIT);
                        assert(instr.operands[1].visibility == ZYDIS_OPERAND_VISIBILITY_EXPLICIT);

                        basic_block_tmp_value op1 = alloc_tmp(), op2 = alloc_tmp();
                        emit_load_operand(op1, instr.operands[0]);
                        emit_load_operand(op2, instr.operands[1]);

                        emit_arith_with_flags(op_size::S_32, SLJIT_SUB32,
                                              op1,
                                              op1,
                                              op2);

                        emit_store_operand(instr.operands[0], op1);
                    }
                    break;
                case ZYDIS_MNEMONIC_LEA:
                    {
                        assert(instr.operands[0].visibility == ZYDIS_OPERAND_VISIBILITY_EXPLICIT);
                        assert(instr.operands[1].visibility == ZYDIS_OPERAND_VISIBILITY_EXPLICIT);
                        assert(instr.operands[0].type == ZYDIS_OPERAND_TYPE_REGISTER);
                        assert(instr.operands[1].type == ZYDIS_OPERAND_TYPE_MEMORY);
                        assert(instr.operands[0].size == 32);


                        if (instr.operands[1].mem.base == ZYDIS_REGISTER_NONE &&
                            instr.operands[1].mem.index == ZYDIS_REGISTER_NONE) {
                            assert(instr.operands[1].mem.disp.has_displacement);

                            emit_op1(SLJIT_MOV32, ref_reg(zydis_reg(instr.operands[0].reg.value)),
                                    sljit_ref::imm(instr.operands[1].imm.value.u));
                        } else if (instr.operands[1].mem.index == ZYDIS_REGISTER_NONE &&
                            instr.operands[1].mem.disp.has_displacement) {

                            emit_op2(SLJIT_ADD32, ref_reg(zydis_reg(instr.operands[0].reg.value)),
                                     ref_reg(zydis_reg(instr.operands[1].mem.base)),
                                     sljit_ref::imm(instr.operands[1].imm.value.u));
                        } else
                            goto fail;
                    }
                    break;
                case ZYDIS_MNEMONIC_CMP:
                    {
                        //assert(instr.operands[0].visibility == ZYDIS_OPERAND_VISIBILITY_EXPLICIT);
                        assert(instr.operands[1].visibility == ZYDIS_OPERAND_VISIBILITY_EXPLICIT);

                        basic_block_tmp_value op1 = alloc_tmp(), op2 = alloc_tmp();
                        emit_load_operand(op1, instr.operands[0]);
                        emit_load_operand(op2, instr.operands[1]);

                        //assert(instr.operands[0].size == 32);

                        emit_arith_with_flags(zydis_op_size(instr.operands[0]), SLJIT_SUB32,
                                              sljit_ref::unused(),
                                              op1, op2);
                    }
                    break;
                case ZYDIS_MNEMONIC_JB:
                case ZYDIS_MNEMONIC_JNB:
                case ZYDIS_MNEMONIC_JBE:
                case ZYDIS_MNEMONIC_JNBE:
                case ZYDIS_MNEMONIC_JZ:
                case ZYDIS_MNEMONIC_JNZ:
                    {
                        assert(instr.operands[0].visibility == ZYDIS_OPERAND_VISIBILITY_EXPLICIT);
                        assert(instr.operands[0].type == ZYDIS_OPERAND_TYPE_IMMEDIATE);
                        assert(instr.operands[0].imm.is_relative);
                        uint32_t new_eip = guest_address + instr.operands[0].imm.value.s;

                        if (instr.mnemonic == ZYDIS_MNEMONIC_JB || instr.mnemonic == ZYDIS_MNEMONIC_JNB) {
                            emit_op2(SLJIT_AND32 | SLJIT_SET_Z, sljit_ref::unused(), ref_reg(cpu_reg::EFLAGS),
                                     sljit_ref::imm(static_cast<int>(cpu_flags::CF)));
                            basic_block_tmp_value tmp = alloc_tmp();
                            emit_op1(SLJIT_MOV32, tmp, sljit_ref::imm(guest_address));
                            emit_cmov(instr.mnemonic == ZYDIS_MNEMONIC_JB ? sljit_flagtype::NOT_ZERO32 : sljit_flagtype::ZERO32,
                                    tmp, sljit_ref::imm(new_eip));

                            emit_op1(SLJIT_MOV32, ref_reg(cpu_reg::EIP), tmp);
                        } else if (instr.mnemonic == ZYDIS_MNEMONIC_JBE || instr.mnemonic == ZYDIS_MNEMONIC_JNBE) {
                            emit_op2(SLJIT_AND32 | SLJIT_SET_Z, sljit_ref::unused(), ref_reg(cpu_reg::EFLAGS),
                                     sljit_ref::imm(static_cast<int>(cpu_flags::CF) | static_cast<int>(cpu_flags::ZF)));

                            basic_block_tmp_value tmp = alloc_tmp();
                            emit_op1(SLJIT_MOV32, tmp, sljit_ref::imm(guest_address));
                            emit_cmov(instr.mnemonic == ZYDIS_MNEMONIC_JBE ? sljit_flagtype::NOT_ZERO32 : sljit_flagtype::ZERO32,
                                      tmp, sljit_ref::imm(new_eip));

                            emit_op1(SLJIT_MOV32, ref_reg(cpu_reg::EIP), tmp);
                        } else if (instr.mnemonic == ZYDIS_MNEMONIC_JZ || instr.mnemonic == ZYDIS_MNEMONIC_JNZ) {
                            emit_op2(SLJIT_AND32 | SLJIT_SET_Z, sljit_ref::unused(), ref_reg(cpu_reg::EFLAGS),
                                     sljit_ref::imm(static_cast<int>(cpu_flags::ZF)));

                            basic_block_tmp_value tmp = alloc_tmp();
                            emit_op1(SLJIT_MOV32, tmp, sljit_ref::imm(guest_address));
                            emit_cmov(instr.mnemonic == ZYDIS_MNEMONIC_JZ ? sljit_flagtype::NOT_ZERO32 : sljit_flagtype::ZERO32,
                                      tmp, sljit_ref::imm(new_eip));

                            emit_op1(SLJIT_MOV32, ref_reg(cpu_reg::EIP), tmp);
                        } else
                            goto fail;
                    }
                    break;
                case ZYDIS_MNEMONIC_OR:
                    {
                        assert(instr.operands[0].visibility == ZYDIS_OPERAND_VISIBILITY_EXPLICIT);
                        assert(instr.operands[1].visibility == ZYDIS_OPERAND_VISIBILITY_EXPLICIT);

                        basic_block_tmp_value op1 = alloc_tmp(), op2 = alloc_tmp();
                        emit_load_operand(op1, instr.operands[0]);
                        emit_load_operand(op2, instr.operands[1]);

                        // TODO: handle cases when we have values at out hands (imm and reg)

                        emit_arith_with_flags(zydis_op_size(instr.operands[0]), SLJIT_OR32, op1, op1, op2);

                        emit_store_operand(instr.operands[0], op1);
                    }
                    break;
                case ZYDIS_MNEMONIC_TEST:
                    {
                        assert(instr.operands[0].visibility == ZYDIS_OPERAND_VISIBILITY_EXPLICIT);
                        assert(instr.operands[1].visibility == ZYDIS_OPERAND_VISIBILITY_EXPLICIT);

                        basic_block_tmp_value op1 = alloc_tmp(), op2 = alloc_tmp();
                        emit_load_operand(op1, instr.operands[0]);
                        emit_load_operand(op2, instr.operands[1]);

                        emit_arith_with_flags(zydis_op_size(instr.operands[0]), SLJIT_AND32, sljit_ref::unused(), op1, op2);
                    }
                    break;
                case ZYDIS_MNEMONIC_XOR:
                {
                    assert(instr.operands[0].visibility == ZYDIS_OPERAND_VISIBILITY_EXPLICIT);
                    assert(instr.operands[1].visibility == ZYDIS_OPERAND_VISIBILITY_EXPLICIT);

                    basic_block_tmp_value op1 = alloc_tmp(), op2 = alloc_tmp();
                    emit_load_operand(op1, instr.operands[0]);
                    emit_load_operand(op2, instr.operands[1]);

                    // TODO: handle cases when we have values at out hands (imm and reg)

                    emit_arith_with_flags(zydis_op_size(instr.operands[0]), SLJIT_XOR32, op1, op1, op2);

                    emit_store_operand(instr.operands[0], op1);
                }
                    break;
                case ZYDIS_MNEMONIC_MOVSX:
                    {
                        assert(instr.operands[0].visibility == ZYDIS_OPERAND_VISIBILITY_EXPLICIT);
                        assert(instr.operands[1].visibility == ZYDIS_OPERAND_VISIBILITY_EXPLICIT);
                        basic_block_tmp_value tmp = alloc_tmp();
                        emit_load_operand(tmp, instr.operands[1]);

                        emit_store_operand(instr.operands[0], tmp);
                    }
                    break;
                case ZYDIS_MNEMONIC_INC:
                    {
                        assert(instr.operands[0].visibility == ZYDIS_OPERAND_VISIBILITY_EXPLICIT);
                        basic_block_tmp_value op = alloc_tmp();
                        emit_load_operand(op, instr.operands[0]);

                        emit_arith_with_flags(zydis_op_size(instr.operands[0]), SLJIT_ADD32, op, op, sljit_ref::imm(1));

                        emit_store_operand(instr.operands[0], op);
                    }
                    break;
                default:
                fail:

                    dump_instruction(instr);
                    uw_log("Unknown instruction\n");
                    throw std::exception();
            }
        }


        void basic_block_compiler::dump_instruction(ZydisDecodedInstruction &instr) {
            ZydisFormatter formatter;
            ZydisFormatterInit(&formatter, ZYDIS_FORMATTER_STYLE_ATT);

            char buffer[256];
            ZydisFormatterFormatInstruction(&formatter, &instr, buffer, sizeof(buffer),
                                            guest_address - instr.length);

            uw_log("%08x: %s\n", guest_address - instr.length, buffer);
        }

        static bool changes_eip(ZydisDecodedInstruction& instr)
        {
            for (int i = 0; i < instr.operand_count; i++)
                if (instr.operands[i].reg.value == ZYDIS_REGISTER_EIP && instr.operands[i].actions & ZYDIS_OPERAND_ACTION_MASK_WRITE)
                    return true;
            return false;
        }

        basic_block basic_block_compiler::compile() {

            ZydisDecoder decoder;
            ZydisDecoderInit(&decoder, ZYDIS_MACHINE_MODE_LEGACY_32,
                    ZYDIS_ADDRESS_WIDTH_32);

            emit_enter(0, SLJIT_ARG1(UW), 6, 6, 0, 0, 0);

            ZydisDecodedInstruction instruction;

            while (true) {
                ZyanStatus res = ZydisDecoderDecodeBuffer(&decoder, host_address, 100, &instruction);
                assert(ZYAN_SUCCESS(res));

                host_address += instruction.length;
                guest_address += instruction.length;
                block_size += instruction.length;

                dump_instruction(instruction);

                compile_instruction(instruction);

                if (changes_eip(instruction))
                    break;
            }

            emit_return();

            auto code = generate_code<basic_block_func>();

            code.dump();

            return basic_block(guest_address - block_size, block_size, std::move(code));
        }

        void basic_block_compiler::emit_store_operand(ZydisDecodedOperand& op, sljit_ref src) {
            switch (op.type) {
                case ZYDIS_OPERAND_TYPE_IMMEDIATE:
                    throw std::exception();

                case ZYDIS_OPERAND_TYPE_REGISTER:
                    emit_op1(get_sized_mov(zydis_op_size(op)), ref_reg(zydis_reg(op.reg.value)), src);
                    break;
                case ZYDIS_OPERAND_TYPE_MEMORY:
                    assert(op.mem.type == ZYDIS_MEMOP_TYPE_MEM);
                    if (op.mem.base == ZYDIS_REGISTER_NONE && op.mem.index == ZYDIS_REGISTER_NONE) {
                        assert(op.mem.disp.has_displacement);
                        emit_store_memory(zydis_op_size(op), zydis_seg(op.mem.segment),
                                         sljit_ref::imm(op.mem.disp.value), src);
                    } else if (op.mem.index == ZYDIS_REGISTER_NONE && op.mem.disp.has_displacement) {
                        auto tmp = alloc_tmp();
                        emit_op2(SLJIT_ADD32, tmp, ref_reg(zydis_reg(op.mem.base)), sljit_ref::imm(op.mem.disp.value));
                        emit_store_memory(zydis_op_size(op), zydis_seg(op.mem.segment), tmp, src);
                    } else if (op.mem.index == ZYDIS_REGISTER_NONE) {
                        auto tmp = alloc_tmp();
                        emit_op1(SLJIT_MOV32, tmp, ref_reg(zydis_reg(op.mem.base)));
                        emit_store_memory(zydis_op_size(op), zydis_seg(op.mem.segment), tmp, src);
                    } else
                        throw std::exception();
                    break;
                default:
                    throw std::exception();
            }
        }

        void basic_block_compiler::emit_load_operand(sljit_ref dst, ZydisDecodedOperand &op) {
            switch (op.type) {
                case ZYDIS_OPERAND_TYPE_IMMEDIATE:
                    if (!op.imm.is_relative)
                        emit_op1(get_sized_mov(zydis_op_size(op)), dst, sljit_ref::imm(op.imm.value.u));
                    else
                        emit_op1(get_sized_mov(zydis_op_size(op)), dst, sljit_ref::imm(op.imm.value.u + guest_address));
                    break;
                case ZYDIS_OPERAND_TYPE_REGISTER:
                    emit_op1(get_sized_mov(zydis_op_size(op)), dst, ref_reg(zydis_reg(op.reg.value)));
                    break;
                case ZYDIS_OPERAND_TYPE_MEMORY:
                    assert(op.mem.type == ZYDIS_MEMOP_TYPE_MEM);
                    if (op.mem.base == ZYDIS_REGISTER_NONE && op.mem.index == ZYDIS_REGISTER_NONE) {
                        assert(op.mem.disp.has_displacement);
                        emit_load_memory(zydis_op_size(op), dst, zydis_seg(op.mem.segment),
                                sljit_ref::imm(op.mem.disp.value));
                    } else if (op.mem.index == ZYDIS_REGISTER_NONE && op.mem.disp.has_displacement) {
                        auto tmp = alloc_tmp();
                        emit_op2(SLJIT_ADD32, tmp, ref_reg(zydis_reg(op.mem.base)), sljit_ref::imm(op.mem.disp.value));
                        emit_load_memory(zydis_op_size(op), dst, zydis_seg(op.mem.segment), tmp);
                    } else if (op.mem.index == ZYDIS_REGISTER_NONE) {
                        auto tmp = alloc_tmp();
                        emit_op1(SLJIT_MOV32, tmp, ref_reg(zydis_reg(op.mem.base)));
                        emit_load_memory(zydis_op_size(op), dst, zydis_seg(op.mem.segment), tmp);
                    } else
                        throw std::exception();
                    break;
                default:
                    throw std::exception();
            }
        }

        void basic_block_compiler::emit_store_memory(op_size sz, cpu_segment seg, sljit_ref offset, sljit_ref src) {
            if (is_segment_static(seg)) {
                sljit::sljit_p base = ctx.host_static_segment_bases[static_cast<int>(seg)];
                if (offset.is_imm()) {
                    emit_op1(get_sized_mov(sz), sljit_ref::mem0(base + offset.get_param()), src);
                } else if (offset.is_reg()) {
                    emit_op1(get_sized_mov(sz), sljit_ref::mem1(static_cast<sljit_reg>(offset.get_base()), base), src);
                    // TODO: more cases possible. Some of them may be optimized
                } else {
                    auto tmp = alloc_tmp();
                    emit_op1(SLJIT_MOV32, tmp, offset);
                    emit_op1(get_sized_mov(sz), sljit_ref::mem1(tmp, base), src);
                }
            } else {
                throw std::exception();
            }
        }

        void basic_block_compiler::emit_load_memory(op_size sz, sljit_ref dst, cpu_segment seg, sljit_ref offset) {
            if (is_segment_static(seg)) {
                sljit::sljit_p base = ctx.host_static_segment_bases[static_cast<int>(seg)];

                if (offset.is_imm()) {
                    emit_op1(get_sized_mov(sz), dst, sljit_ref::mem0(base + offset.get_param()));
                } else if (offset.is_reg()) {
                    emit_op1(get_sized_mov(sz), dst, sljit_ref::mem1(static_cast<sljit_reg>(offset.get_base()), base));
                    // TODO: more cases possible. Some of them may be optimized
                } else {
                    auto tmp = alloc_tmp();
                    emit_op1(SLJIT_MOV32, tmp, offset);
                    emit_op1(get_sized_mov(sz), dst, sljit_ref::mem1(tmp, base));
                }
            } else {
                throw std::exception();
            }
        }

        void basic_block_compiler::emit_arith_with_flags(op_size sz, sljit::sljit_s32 op, sljit_ref dst, sljit_ref src1, sljit_ref src2,
                bool update_carry) {
            basic_block_tmp_value pending_flags = alloc_tmp(),
                tmp = alloc_tmp();


            emit_op1(SLJIT_MOV32, pending_flags, sljit_ref::imm(0));
            emit_op1(SLJIT_MOV32, tmp, sljit_ref::imm(0));

            bool is_32 = sz == op_size::S_32;
            bool is_add = op == SLJIT_ADD32;
            bool is_sub = op == SLJIT_SUB32;
            bool is_bitwise = op == SLJIT_OR32 || op == SLJIT_AND32 || op == SLJIT_XOR32;

            if (is_add || is_sub)
                assert(sz == op_size::S_32);

            emit_op2(op | SLJIT_SET_Z
                | (is_32 && is_add ? SLJIT_SET_CARRY : 0)
                | (is_sub ? SLJIT_SET_LESS : 0), is_bitwise ? dst : sljit_ref::unused(), src1, src2);
            emit_cmov(sljit_flagtype::ZERO32, pending_flags, sljit_ref::imm(static_cast<int>(cpu_flags::ZF)));
            if (is_add) {
                if (update_carry)
                    emit_op2(SLJIT_ADDC32, tmp, sljit_ref::imm(0), sljit_ref::imm(0));
            } else if (is_sub) {
                if (update_carry)
                    emit_cmov(sljit_flagtype::LESS32, tmp, sljit_ref::imm(static_cast<int>(cpu_flags::CF)));
            } else if (is_bitwise) {
                // no CF for bitwise
            } else {
                throw std::exception();
            }
            if ((is_add || is_sub) && update_carry)
                emit_op2(SLJIT_OR32, pending_flags, pending_flags, tmp); // set CF

            if (!is_bitwise && !dst.is_unused())
                emit_op2(op, dst, src1, src2);

            // TODO: set other flags

            uint32_t flags_mask = static_cast<int>(cpu_flags::ZF) | (update_carry ? static_cast<int>(cpu_flags::CF) : 0) |
                    static_cast<int>(cpu_flags::PF) | static_cast<int>(cpu_flags::OF) | static_cast<int>(cpu_flags::SF);

            emit_op2(SLJIT_AND32, ref_reg(cpu_reg::EFLAGS), ref_reg(cpu_reg::EFLAGS), sljit_ref::imm(~flags_mask));
            emit_op2(SLJIT_OR32, ref_reg(cpu_reg::EFLAGS), ref_reg(cpu_reg::EFLAGS), pending_flags);
        }
    }
}
