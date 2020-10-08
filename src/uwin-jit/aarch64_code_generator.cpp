//
// Created by dcnick3 on 6/27/20.
//

#include "uwin/util/log.h"
#include "uwin/uwin.h"

#include "uwin-jit/aarch64_code_generator.h"

namespace uwin {
    namespace jit {

        typedef aarch64_code_generator::tmp_holder tmp_holder;

        static cpu_reg zydis_hireg(const ZydisRegister zreg) {
            switch (zreg) {
                case ZYDIS_REGISTER_AH:
                    return cpu_reg::EAX;
                case ZYDIS_REGISTER_BH:
                    return cpu_reg::EBX;
                case ZYDIS_REGISTER_CH:
                    return cpu_reg::ECX;
                case ZYDIS_REGISTER_DH:
                    return cpu_reg::EDX;
                default:
                    throw std::exception();
            }
        }

        static bool zydis_is_highreg(const ZydisRegister zreg) {
            switch (zreg) {
                case ZYDIS_REGISTER_AH:
                case ZYDIS_REGISTER_BH:
                case ZYDIS_REGISTER_CH:
                case ZYDIS_REGISTER_DH:
                    return true;
                default:
                    return false;
            }
        }

        static cpu_reg zydis_reg(const ZydisRegister zreg) {
            switch (zreg) {
                case ZYDIS_REGISTER_AL:
                case ZYDIS_REGISTER_AX:
                case ZYDIS_REGISTER_EAX: return cpu_reg::EAX;

                case ZYDIS_REGISTER_BL:
                case ZYDIS_REGISTER_BX:
                case ZYDIS_REGISTER_EBX: return cpu_reg::EBX;

                case ZYDIS_REGISTER_CL:
                case ZYDIS_REGISTER_CX:
                case ZYDIS_REGISTER_ECX: return cpu_reg::ECX;

                case ZYDIS_REGISTER_DL:
                case ZYDIS_REGISTER_DX:
                case ZYDIS_REGISTER_EDX: return cpu_reg::EDX;

                case ZYDIS_REGISTER_SIL:
                case ZYDIS_REGISTER_SI:
                case ZYDIS_REGISTER_ESI: return cpu_reg::ESI;

                case ZYDIS_REGISTER_DIL:
                case ZYDIS_REGISTER_DI:
                case ZYDIS_REGISTER_EDI: return cpu_reg::EDI;

                case ZYDIS_REGISTER_SP:
                case ZYDIS_REGISTER_ESP: return cpu_reg::ESP;

                case ZYDIS_REGISTER_BP:
                case ZYDIS_REGISTER_EBP: return cpu_reg::EBP;

                default:
                    throw std::exception();
            }
        }

        template<bool SGN = false>
        static op_size zydis_reg_size(const ZydisRegister zreg) {
            switch (zreg) {
                case ZYDIS_REGISTER_AL:
                case ZYDIS_REGISTER_BL:
                case ZYDIS_REGISTER_CL:
                case ZYDIS_REGISTER_DL:
                case ZYDIS_REGISTER_SIL:
                case ZYDIS_REGISTER_DIL:
                case ZYDIS_REGISTER_AH:
                case ZYDIS_REGISTER_BH:
                case ZYDIS_REGISTER_CH:
                case ZYDIS_REGISTER_DH:
                    if (SGN)
                        return op_size::S_8;
                    else
                        return op_size::U_8;
                case ZYDIS_REGISTER_AX:
                case ZYDIS_REGISTER_BX:
                case ZYDIS_REGISTER_CX:
                case ZYDIS_REGISTER_DX:
                case ZYDIS_REGISTER_SI:
                case ZYDIS_REGISTER_DI:
                case ZYDIS_REGISTER_BP:
                case ZYDIS_REGISTER_SP:
                    if (SGN)
                        return op_size::S_16;
                    else
                        return op_size::U_16;
                case ZYDIS_REGISTER_EAX:
                case ZYDIS_REGISTER_EBX:
                case ZYDIS_REGISTER_ECX:
                case ZYDIS_REGISTER_EDX:
                case ZYDIS_REGISTER_ESI:
                case ZYDIS_REGISTER_EDI:
                case ZYDIS_REGISTER_EBP:
                case ZYDIS_REGISTER_ESP:
                    if (SGN)
                        return op_size::S_32;
                    else
                        return op_size::U_32;

                default:
                    throw std::exception();
            }
        }


        template<bool SGN = false>
        static op_size zydis_op_size(const ZydisDecodedOperand& op) {
            if (SGN) {
                switch (op.size) {
                    case 8:
                        return op_size::S_8;
                    case 16:
                        return op_size::S_16;
                    case 32:
                        return op_size::S_32;
                    case 64:
                        return op_size::S_64;
                }
            } else {
                switch (op.size) {
                    case 8:
                        return op_size::U_8;
                    case 16:
                        return op_size::U_16;
                    case 32:
                        return op_size::U_32;
                    case 64:
                        return op_size::U_64;
                }
            }
            throw std::exception();
        }

        template <bool SGN>
        static op_size op_size_change_sign(op_size sz) {
            if (SGN)
                switch (sz) {
                    case op_size::S_8:
                    case op_size::U_8:
                        return op_size::S_8;
                    case op_size::S_16:
                    case op_size::U_16:
                        return op_size::S_16;
                    case op_size::S_32:
                    case op_size::U_32:
                        return op_size::S_32;
                    case op_size::S_64:
                    case op_size::U_64:
                        return op_size::S_64;
                }
            else
                switch (sz) {
                    case op_size::S_8:
                    case op_size::U_8:
                        return op_size::U_8;
                    case op_size::S_16:
                    case op_size::U_16:
                        return op_size::U_16;
                    case op_size::S_32:
                    case op_size::U_32:
                        return op_size::U_32;
                    case op_size::S_64:
                    case op_size::U_64:
                        return op_size::U_64;
                }
            throw std::exception();
        }

        static int op_size_to_bits(op_size sz) {
            switch (sz) {
                case op_size::S_8:
                case op_size::U_8:
                    return 8;
                case op_size::S_16:
                case op_size::U_16:
                    return 16;
                case op_size::S_32:
                case op_size::U_32:
                    return 32;
                case op_size::S_64:
                case op_size::U_64:
                    return 64;
            }
            throw std::exception();
        }

        static cpu_segment zydis_seg(const ZydisRegister reg) {
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

        static bool zydis_is_seg(const ZydisRegister reg) {
            switch (reg) {
                case ZYDIS_REGISTER_CS:
                case ZYDIS_REGISTER_DS:
                case ZYDIS_REGISTER_ES:
                case ZYDIS_REGISTER_SS:
                case ZYDIS_REGISTER_FS:
                case ZYDIS_REGISTER_GS:
                    return true;
                default:
                    return false;
            }
        }

        void aarch64_code_generator::emit_prologue() {
            masm.Bind(&start_label);

            masm.Stp(vixl::aarch64::x29, vixl::aarch64::x30,
                    vixl::aarch64::MemOperand(vixl::aarch64::sp, -(int64_t)stack_frame_size, vixl::aarch64::AddrMode::PreIndex));

            masm.Mov(vixl::aarch64::x29, vixl::aarch64::sp);

            masm.Ldr(guest_base_reg(), ctx.guest_base);
        }

        void aarch64_code_generator::emit_epilogue() {
            masm.Ldp(vixl::aarch64::x29, vixl::aarch64::x30,
                         vixl::aarch64::MemOperand(vixl::aarch64::sp, (int64_t)stack_frame_size, vixl::aarch64::AddrMode::PostIndex));

            masm.Ret();
        }


#define F_SF static_cast<uint32_t>(cpu_flags::SF)
#define F_ZF static_cast<uint32_t>(cpu_flags::ZF)
#define F_CF static_cast<uint32_t>(cpu_flags::CF)
#define F_OF static_cast<uint32_t>(cpu_flags::OF)
#define F_PF static_cast<uint32_t>(cpu_flags::PF)

#define F_SF_SHIFT (7)
#define F_ZF_SHIFT (6)
#define F_CF_SHIFT (0)
#define F_OF_SHIFT (11)
#define F_PF_SHIFT (2)

        static cpu_context* interrupt_runtime_helper(cpu_context* ctx, uint64_t interrupt_number) {
            if (interrupt_number == 0x80) {
                ctx->EAX() = uw_cpu_do_syscall(ctx->EAX(), ctx->EBX(), ctx->ECX(), ctx->EDX(),
                        ctx->ESI(), ctx->EDI(), ctx->EBP(), 0, 0);
            } else {
                uw_log("interrupt_number = %02x\n", (unsigned)interrupt_number);
                uw_cpu_panic("Unknown interrupt number");
            }

            return ctx;
        }

        template<int elem_size>
        static cpu_context* rep_movs_runtime_helper(cpu_context* ctx) {
            size_t num = ctx->ECX() * elem_size; // no way to interrupt long copy

            void* src = g2h(ctx->ESI());
            void* dst = g2h(ctx->EDI());

            memmove(dst, src, num);

            ctx->ECX() = 0;
            ctx->ESI() += num;
            ctx->EDI() += num;

            return ctx;
        }

        template<typename E>
        static cpu_context* rep_stos_runtime_helper(cpu_context* ctx) {
            size_t num = ctx->ECX(); // no way to interrupt long memset

            E *dst = g2hx<E>(ctx->EDI());
            E val = static_cast<E>(ctx->EAX());
            if (val == 0 || sizeof(E) == 1)
                memset(dst, val, num * sizeof(E));
            else {
                for (size_t i = 0; i < num; i++)
                    dst[i] = val;
            }

            ctx->ECX() = 0;
            ctx->EDI() += num * sizeof(E);

            return ctx;
        }

        template<typename E>
        static cpu_context* repne_scas_runtime_helper(cpu_context* ctx)
        {
            size_t num = ctx->ECX();
            E* ptr = g2hx<E>(ctx->EDI());
            E cmp = static_cast<E>(ctx->EAX());

            size_t i = 0;
            bool cont = true;
            while (i < num && cont) {
                cont = ptr[i] != cmp;
                i++;
            }

            ctx->EDI() += sizeof(E) * i;
            ctx->ECX() -= i;

            return ctx;
        }

        template<typename E>
        static cpu_context* repe_scas_runtime_helper(cpu_context* ctx)
        {
            size_t num = ctx->ECX();
            E* ptr = g2hx<E>(ctx->EDI());
            E cmp = static_cast<E>(ctx->EAX());

            size_t i = 0;
            bool cont = true;
            while (i < num && cont) {
                cont = ptr[i] == cmp;
                i++;
            }

            ctx->EDI() += sizeof(E) * i;
            ctx->ECX() -= i;
            return ctx;
        }

        template<typename E>
        static cpu_context* repe_cmps_runtime_helper(cpu_context* ctx)
        {
            size_t num = ctx->ECX();
            E* sptr = g2hx<E>(ctx->ESI());
            E* dptr = g2hx<E>(ctx->EDI());

            size_t i = 0;
            bool cont = true;
            while (i < num && cont) {
                cont = sptr[i] == dptr[i];
                i++;
            }

            ctx->ESI() += sizeof(E) * i;
            ctx->EDI() += sizeof(E) * i;
            ctx->ECX() -= i;

            return ctx;
        }

        static cpu_context* fwait_runtime_helper(cpu_context* ctx)
        {
            ctx->fpu_ctx->fwait();
            return ctx;
        }

        static cpu_context* fpu_reg_runtime_helper(cpu_context* ctx, uint32_t opcode)
        {
            ctx->fpu_ctx->reg_op(opcode);
            return ctx;
        }

        static cpu_context* fpu_mem_runtime_helper(cpu_context* ctx, uint32_t opcode, uint32_t linear_address,
                uint32_t segment_offset, uint32_t segment)
        {
            ctx->fpu_ctx->mem_op(opcode, linear_address, segment_offset, segment);
            return ctx;
        }

        static cpu_context* update_flags_helper(cpu_context* ctx)
        {
            // TODO: take care of reserved stuff
            auto flags = ctx->EFLAGS();
            bool pf = (flags & static_cast<int>(cpu_flags::PF)) != 0;
            bool sf = (flags & static_cast<int>(cpu_flags::SF)) != 0;
            bool zf = (flags & static_cast<int>(cpu_flags::ZF)) != 0;
            auto res = zf ? 0 : 1;
            auto pneg = zf != pf;
            auto sneg = sf;

            ctx->F_TYPE() = static_cast<int>(flags_op::load_flags) | (sneg << 31) | (pneg << 30);
            ctx->F_RES() = res;
            //assert((ctx->EFLAGS() & supported_flags_mask) == ctx->EFLAGS());
            return ctx;
        }

        static cpu_context* cpuid_helper(cpu_context* ctx)
        {
            auto leaf = ctx->EAX();
            switch (leaf) {
                // here we tell that we are pentium 4
                case 1:
                    ctx->EAX() = 0x00000f12;
                    ctx->EBX() = 0x00010000;
                    // no extensions besides x87 =)
                    ctx->ECX() = 0;
                    ctx->EDX() = 0x1;
                    break;
                default:
                    throw std::exception();
            }

            return ctx;
        }


        static bool changes_eip(const ZydisDecodedInstruction& instr)
        {
            for (int i = 0; i < instr.operand_count; i++)
                if (instr.operands[i].reg.value == ZYDIS_REGISTER_EIP && instr.operands[i].actions & ZYDIS_OPERAND_ACTION_MASK_WRITE)
                    return true;
            return false;
        }

        // atomicity of operations is provided by allowing only one compiled basic block to be executed at a time
        bool aarch64_code_generator::emit_instruction(uint32_t guest_eip, const ZydisDecodedInstruction &instr, bool force_terminate_block) {
            current_guest_eip = guest_eip;

            if (guest_eip == 6449829) {
                uw_log("bonk\n");
            }

            if (instr.meta.isa_set == ZYDIS_ISA_SET_X87) {

                // we have two kinds of FPU operations: register-only and memory-accessing
                // memory-accessing have only one memory operand
                // we decode the operand, decide whether the instruction needs memory and then call the appropriate handler
                auto opcode = instr.opcode;
                uint32_t full_opcode = (opcode << 8 & 0x700) | *g2hx<uint8_t>(guest_eip - instr.length + 1);
                if (instr.mnemonic == ZYDIS_MNEMONIC_FWAIT) {
                    register_saver_scope save(*this, false);
                    masm.CallRuntime(fwait_runtime_helper);
                } else if (instr.raw.modrm.mod == 0b11) {
                    // register-only
                    register_saver_scope save(*this, false);
                    masm.Mov(vixl::aarch64::x1, full_opcode);
                    masm.CallRuntime(fpu_reg_runtime_helper);
                } else {
                    // memory-accesing
                    ZydisDecodedOperand op;
                    if (instr.operands[1].type == ZYDIS_OPERAND_TYPE_MEMORY && instr.operands[0].type != ZYDIS_OPERAND_TYPE_MEMORY)
                        op = instr.operands[1];
                    else if (instr.operands[1].type != ZYDIS_OPERAND_TYPE_MEMORY && instr.operands[0].type == ZYDIS_OPERAND_TYPE_MEMORY)
                        op = instr.operands[0];
                    else
                        throw std::exception();
                    vixl::aarch64::XRegister offset, linaddr;
                    auto segment = zydis_seg(op.mem.segment);
                    {
                        auto offset_register = emit_load_effective_address_tmp(op);
                        auto linear_address_register = get_tmp_reg();

                        if (segment == cpu_segment::CS || segment == cpu_segment::DS || segment == cpu_segment::ES ||
                            segment == cpu_segment::SS) {
                            masm.Mov(linear_address_register.w(), offset_register.w());
                        } else {
                            auto tmp = emit_load_seg_tmp(segment);
                            masm.Add(linear_address_register.x(), tmp.x(),
                                     vixl::aarch64::Operand(offset_register.x(), vixl::aarch64::Extend::UXTW));
                            masm.Sub(linear_address_register.x(), linear_address_register.x(),
                                     guest_base_reg());
                        }
                        offset = offset_register.x();
                        linaddr = linear_address_register.x();
                    }

                    {
                        register_saver_scope save(*this, false);
                        masm.Mov(vixl::aarch64::x1, full_opcode);
                        masm.Mov(vixl::aarch64::x2, linaddr);
                        masm.Mov(vixl::aarch64::x3, offset);
                        masm.Mov(vixl::aarch64::x4, static_cast<int>(segment));
                        masm.CallRuntime(fpu_mem_runtime_helper);
                    }
                }
            } else {
                switch (instr.mnemonic) {
                    case ZYDIS_MNEMONIC_PUSH: {
                        // Zydis sucks at this =(
                        //assert(instr.operands[0].size == 32 || instr.operands[0].type == ZYDIS_OPERAND_TYPE_IMMEDIATE);
                        // how does x86 extend non-immediate values?
                        auto esp = emit_load_reg_tmp(op_size::U_32, cpu_reg::ESP);
                        auto val = instr.operands[0].type == ZYDIS_OPERAND_TYPE_IMMEDIATE
                                ? emit_load_operand_tmp(op_size::U_32, instr.operands[0])
                                : emit_load_operand_tmp(zydis_op_size(instr.operands[0]), instr.operands[0]);
                        masm.Sub(esp.w(), esp.w(), 4);

                        emit_store_memory(op_size::U_32, val, cpu_segment::SS, esp);

                        emit_store_reg(op_size::U_32, esp, cpu_reg::ESP);
                    }
                        break;
                    case ZYDIS_MNEMONIC_POP: {
                        auto esp = emit_load_reg_tmp(op_size::U_32, cpu_reg::ESP);
                        auto val = emit_load_memory_tmp(op_size::U_32, cpu_segment::SS, esp);

                        emit_store_operand(val, instr.operands[0]);

                        masm.Add(esp.w(), esp.w(), 4);
                        emit_store_reg(op_size::U_32, esp, cpu_reg::ESP);
                    }
                        break;
                    case ZYDIS_MNEMONIC_JMP: {
                        if (instr.operands[0].type == ZYDIS_OPERAND_TYPE_IMMEDIATE)
                            hint_branch(get_imm_operand(op_size::U_32, instr.operands[0]));
                        auto new_eip = emit_load_operand_tmp(op_size::U_32, instr.operands[0]);
                        emit_store_reg(op_size::U_32, new_eip, cpu_reg::EIP);
                    }
                        break;
                    case ZYDIS_MNEMONIC_CALL: {
                        assert(instr.meta.branch_type == ZYDIS_BRANCH_TYPE_NEAR);
                        if (instr.operands[0].type == ZYDIS_OPERAND_TYPE_IMMEDIATE)
                            hint_branch(get_imm_operand(op_size::U_32, instr.operands[0]));
                        auto new_eip = emit_load_operand_tmp(op_size::U_32, instr.operands[0]);
                        auto current_eip = emit_load_imm_tmp(current_guest_eip);
                        auto esp = emit_load_reg_tmp(op_size::U_32, cpu_reg::ESP);
                        masm.Sub(esp.w(), esp.w(), 4);

                        emit_store_memory(op_size::U_32, current_eip, cpu_segment::SS, esp);

                        emit_store_reg(op_size::U_32, new_eip, cpu_reg::EIP);
                        emit_store_reg(op_size::U_32, esp, cpu_reg::ESP);
                    }
                        break;
                    case ZYDIS_MNEMONIC_RET: {
                        assert(instr.meta.branch_type == ZYDIS_BRANCH_TYPE_NEAR);
                        auto esp = emit_load_reg_tmp(op_size::U_32, cpu_reg::ESP);
                        auto new_eip = emit_load_memory_tmp(op_size::U_32, cpu_segment::SS, esp);

                        emit_store_reg(op_size::U_32, new_eip, cpu_reg::EIP);

                        if (instr.operands[0].visibility != ZYDIS_OPERAND_VISIBILITY_HIDDEN) {
                            assert(instr.operands[0].type == ZYDIS_OPERAND_TYPE_IMMEDIATE);
                            assert(!instr.operands[0].imm.is_signed);
                            masm.Add(esp.w(), esp.w(), 4 + instr.operands[0].imm.value.u);
                        } else
                            masm.Add(esp.w(), esp.w(), 4);
                        emit_store_reg(op_size::U_32, esp, cpu_reg::ESP);
                    }
                        break;
                    case ZYDIS_MNEMONIC_LEAVE: {
                        auto new_esp = emit_load_reg_tmp(op_size::U_32, cpu_reg::EBP);
                        auto new_ebp = emit_load_memory_tmp(op_size::U_32, cpu_segment::SS, new_esp);
                        masm.Add(new_esp.w(), new_esp.w(), 4);

                        emit_store_reg(op_size::U_32, new_esp, cpu_reg::ESP);
                        emit_store_reg(op_size::U_32, new_ebp, cpu_reg::EBP);
                    }
                        break;
                    case ZYDIS_MNEMONIC_MOV: {
                        assert(instr.operands[0].size == instr.operands[1].size);
                        auto val = emit_load_operand_tmp(zydis_op_size(instr.operands[0]), instr.operands[1]);
                        emit_store_operand(val, instr.operands[0]);
                    }
                        break;
                    case ZYDIS_MNEMONIC_XCHG: {
                        auto op1 = emit_load_operand_tmp(zydis_op_size(instr.operands[0]), instr.operands[0]);
                        auto op2 = emit_load_operand_tmp(zydis_op_size(instr.operands[1]), instr.operands[1]);
                        emit_store_operand(op2, instr.operands[0]);
                        emit_store_operand(op1, instr.operands[1]);
                    }
                        break;

                    case ZYDIS_MNEMONIC_INC: {
                        auto op = emit_load_operand_tmp(zydis_op_size(instr.operands[0]), instr.operands[0]);
                        auto res = get_tmp_reg();
                        masm.Adds(res.w(), op.w(), 1);
                        emit_update_flags(zydis_op_size(instr.operands[0]), flags_op::inc, res, op, op);

                        emit_store_operand(res, instr.operands[0]);
                    }
                        break;
                    case ZYDIS_MNEMONIC_DEC: {
                        auto op = emit_load_operand_tmp(zydis_op_size(instr.operands[0]), instr.operands[0]);
                        auto res = get_tmp_reg();
                        masm.Subs(res.w(), op.w(), 1);
                        emit_update_flags(zydis_op_size(instr.operands[0]), flags_op::dec, res, op, op);

                        emit_store_operand(res, instr.operands[0]);
                    }
                        break;

                    case ZYDIS_MNEMONIC_ADD: {
                        auto op1 = emit_load_operand_tmp(zydis_op_size(instr.operands[0]), instr.operands[0]);
                        auto op2 = emit_load_operand_tmp(zydis_op_size(instr.operands[0]), instr.operands[1]);
                        auto res = get_tmp_reg();
                        masm.Adds(res.w(), op1.w(), op2.w());
                        emit_update_flags(zydis_op_size(instr.operands[0]), flags_op::add, res, op1, op2);

                        emit_store_operand(res, instr.operands[0]);
                    }
                        break;
                    case ZYDIS_MNEMONIC_ADC: {
                        {
                            auto cond = emit_test_condition<cpu_condition::c>();
                            auto tmp = get_tmp_reg();
                            masm.Csel(tmp.x(), 0b0010 << 28, 0, cond);
                            masm.Msr(vixl::aarch64::SystemRegister::NZCV, tmp.x());
                        }

                        auto op1 = emit_load_operand_tmp(zydis_op_size(instr.operands[0]), instr.operands[0]);
                        auto op2 = emit_load_operand_tmp(zydis_op_size(instr.operands[0]), instr.operands[1]);
                        auto res = get_tmp_reg();


                        masm.Adcs(res.w(), op1.w(), op2.w());
                        emit_update_flags(zydis_op_size(instr.operands[0]), flags_op::adc, res, op1, op2);

                        emit_store_operand(res, instr.operands[0]);
                    }
                        break;
                    case ZYDIS_MNEMONIC_XADD: {
                        // commonly used to implement atomic fetch and add.
                        // atomicity is provided by allowing only one basic block to be executed concurrently
                        auto op1 = emit_load_operand_tmp(zydis_op_size(instr.operands[0]), instr.operands[0]);
                        auto op2 = emit_load_operand_tmp(zydis_op_size(instr.operands[0]), instr.operands[1]);
                        auto res = get_tmp_reg();
                        assert(instr.operands[0].size == 32);
                        masm.Adds(res.w(), op1.w(), op2.w());
                        emit_update_flags(zydis_op_size(instr.operands[0]), flags_op::add, res, op1, op2);

                        emit_store_operand(res, instr.operands[0]);
                        emit_store_operand(op1, instr.operands[1]);
                    }
                        break;

                    case ZYDIS_MNEMONIC_SUB: {
                        auto op1 = emit_load_operand_tmp(zydis_op_size(instr.operands[0]), instr.operands[0]);
                        auto op2 = emit_load_operand_tmp(zydis_op_size(instr.operands[0]), instr.operands[1]);
                        auto res = get_tmp_reg();
                        masm.Subs(res.w(), op1.w(), op2.w());
                        emit_update_flags(zydis_op_size(instr.operands[0]), flags_op::sub, res, op1, op2);

                        emit_store_operand(res, instr.operands[0]);
                    }
                        break;
                    case ZYDIS_MNEMONIC_SBB: {

                        {
                            // borrow is inverted
                            auto cond = emit_test_condition<cpu_condition::nc>();
                            auto tmp = get_tmp_reg();
                            masm.Csel(tmp.x(), 0b0010 << 28, 0, cond);
                            masm.Msr(vixl::aarch64::SystemRegister::NZCV, tmp.x());
                        }

                        auto op1 = emit_load_operand_tmp(zydis_op_size(instr.operands[0]), instr.operands[0]);
                        auto op2 = emit_load_operand_tmp(zydis_op_size(instr.operands[0]), instr.operands[1]);
                        auto res = get_tmp_reg();

                        masm.Sbcs(res.w(), op1.w(), op2.w());
                        emit_update_flags(zydis_op_size(instr.operands[0]), flags_op::sbb, res, op1, op2);

                        emit_store_operand(res, instr.operands[0]);
                    }
                        break;
                    case ZYDIS_MNEMONIC_CMP: {
                        auto op1 = emit_load_operand_tmp(zydis_op_size(instr.operands[0]), instr.operands[0]);
                        auto op2 = emit_load_operand_tmp(zydis_op_size(instr.operands[0]), instr.operands[1]);
                        auto res = get_tmp_reg();
                        masm.Subs(res.w(), op1.w(), op2.w());

                        emit_update_flags(zydis_op_size(instr.operands[0]), flags_op::sub, res, op1, op2);
                    }
                        break;
                    case ZYDIS_MNEMONIC_CMPXCHG: {
                        assert(instr.operands[0].size == 32);
                        auto eax = emit_load_reg_tmp(op_size::U_32, cpu_reg::EAX);
                        auto dst = emit_load_operand_tmp(zydis_op_size(instr.operands[0]), instr.operands[0]);
                        auto val = emit_load_operand_tmp(zydis_op_size(instr.operands[0]), instr.operands[1]);

                        auto res = get_tmp_reg();
                        masm.Subs(res.w(), eax.w(), dst.w());

                        vixl::aarch64::Label skip;
                        masm.B(vixl::aarch64::Condition::eq, &skip);
                        masm.Move(val.w(), dst.w());
                        masm.Bind(&skip);

                        // simulate the cmp part
                        emit_update_flags(zydis_op_size(instr.operands[0]), flags_op::sub, res, eax, dst);

                        emit_store_operand(val, instr.operands[0]);
                    }
                        break;
                    case ZYDIS_MNEMONIC_NEG: {
                        auto op1 = emit_load_imm_tmp(0);
                        auto op2 = emit_load_operand_tmp(zydis_op_size(instr.operands[0]), instr.operands[0]);
                        auto res = get_tmp_reg();
                        masm.Subs(res.w(), op1.w(), op2.w());
                        emit_update_flags(zydis_op_size(instr.operands[0]), flags_op::sub, res, op1, op2);

                        emit_store_operand(res, instr.operands[0]);
                    }
                        break;
                    case ZYDIS_MNEMONIC_NOT: {
                        auto op = emit_load_operand_tmp(zydis_op_size(instr.operands[0]), instr.operands[0]);
                        masm.Mvn(op.w(), op.w());
                        emit_store_operand(op, instr.operands[0]);
                    }
                        break;
                    case ZYDIS_MNEMONIC_DIV: {
                        auto divisor = emit_load_operand_tmp(zydis_op_size(instr.operands[0]), instr.operands[0]);
                        vixl::aarch64::Label skip_de; // faults are unrecoverable

                        masm.Cbnz(divisor.w(), &skip_de); // skip if we are note dividing by zero
                        vixl::aarch64::Label call_de;
                        masm.Bind(&call_de);

                        emit_interrupt(0); // Division Error

                        masm.Bind(&skip_de);

                        auto dividend = get_tmp_reg();
                        if (instr.operands[0].size == 32) {
                            emit_load_reg(op_size::U_32, dividend, cpu_reg::EAX);

                            {
                                auto dividend_hi = emit_load_reg_tmp(op_size::U_32, cpu_reg::EDX);
                                masm.Orr(dividend.x(), dividend.x(),
                                         vixl::aarch64::Operand(dividend_hi.x(), vixl::aarch64::Shift::LSL, 32));
                            }
                        } else if (instr.operands[0].size == 16) {
                            emit_load_reg(op_size::U_16, dividend, cpu_reg::EAX);

                            {
                                auto dividend_hi = emit_load_reg_tmp(op_size::U_16, cpu_reg::EDX);
                                masm.Orr(dividend.x(), dividend.x(),
                                         vixl::aarch64::Operand(dividend_hi.x(), vixl::aarch64::Shift::LSL, 16));
                            }
                        } else if (instr.operands[0].size == 8) {
                            emit_load_reg(op_size::U_16, dividend, cpu_reg::EAX);
                        } else
                            throw std::exception();

                        auto quotient = get_tmp_reg();
                        masm.Udiv(quotient.x(), dividend.x(), divisor.x());

                        masm.Tst(quotient.x(), ((1ULL << instr.operands[0].size) - 1) << instr.operands[0].size);
                        masm.B(vixl::aarch64::Condition::ne, &call_de);

                        // now compute the remainder
                        auto rem = get_tmp_reg();

                        masm.Msub(rem.x(), quotient.x(), divisor.x(), dividend.x());

                        if (instr.operands[0].size == 32) {
                            emit_store_reg(op_size::U_32, quotient, cpu_reg::EAX);
                            emit_store_reg(op_size::U_32, rem, cpu_reg::EDX);
                        } else if (instr.operands[0].size == 16) {
                            emit_store_reg(op_size::U_16, quotient, cpu_reg::EAX);
                            emit_store_reg(op_size::U_16, rem, cpu_reg::EDX);
                        } else if (instr.operands[0].size == 8) {
                            masm.Orr(quotient.w(), quotient.w(),
                                    vixl::aarch64::Operand(rem.w(), vixl::aarch64::Shift::LSL, 8));
                            emit_store_reg(op_size::U_16, quotient, cpu_reg::EAX);
                        } else
                            throw std::exception();
                    }
                        break;
                    case ZYDIS_MNEMONIC_IDIV: {
                        assert(instr.operands[0].size == 32);
                        auto divisor = emit_load_operand_tmp(zydis_op_size(instr.operands[0]), instr.operands[0]);
                        vixl::aarch64::Label skip_de; // faults are unrecoverable

                        masm.Cbnz(divisor.w(), &skip_de); // skip if we are note dividing by zero
                        vixl::aarch64::Label call_de;
                        masm.Bind(&call_de);

                        emit_interrupt(0); // Division Error

                        masm.Bind(&skip_de);

                        auto dividend = emit_load_reg_tmp(op_size::U_32, cpu_reg::EAX);

                        {
                            auto dividend_hi = emit_load_reg_tmp(op_size::U_32, cpu_reg::EDX);
                            masm.Orr(dividend.x(), dividend.x(),
                                     vixl::aarch64::Operand(dividend_hi.x(), vixl::aarch64::Shift::LSL, 32));
                        }

                        auto quotient = get_tmp_reg();
                        masm.Sdiv(quotient.x(), dividend.x(), divisor.x());

                        // too low
                        masm.Cmp(quotient.x(), 0xffffffff80000000ULL);
                        masm.B(vixl::aarch64::Condition::lt, &call_de);
                        // too high
                        masm.Cmp(quotient.x(), 0x000000007fffffffULL);
                        masm.B(vixl::aarch64::Condition::gt, &call_de);

                        // now compute the remainder
                        auto rem = get_tmp_reg();

                        masm.Smsubl(rem.x(), quotient.w(), divisor.w(), dividend.x());

                        emit_store_reg(op_size::U_32, quotient, cpu_reg::EAX);
                        emit_store_reg(op_size::U_32, rem, cpu_reg::EDX);
                    }
                        break;

                    case ZYDIS_MNEMONIC_IMUL: {
                        int form = 1;
                        if (instr.operands[1].visibility == ZYDIS_OPERAND_VISIBILITY_EXPLICIT)
                            form = 2;
                        if (instr.operands[2].visibility == ZYDIS_OPERAND_VISIBILITY_EXPLICIT)
                            form = 3;

                        auto op1 = get_tmp_reg();
                        if (form == 1)
                            emit_load_reg(zydis_op_size<true>(instr.operands[0]), op1, cpu_reg::EAX);
                        else if (form == 2)
                            emit_load_operand(zydis_op_size<true>(instr.operands[0]), op1, instr.operands[0]);
                        else if (form == 3)
                            // sign extend the imm operand
                            emit_load_operand(op_size::S_32, op1, instr.operands[2]);
                        else
                            throw std::exception();

                        auto op2 = form == 1
                                ? emit_load_operand_tmp(zydis_op_size<true>(instr.operands[0]), instr.operands[0])
                                : emit_load_operand_tmp(zydis_op_size<true>(instr.operands[1]), instr.operands[1]);
                        auto res = get_tmp_reg();

                        masm.Smull(res.x(), op1.w(), op2.w());

                        emit_update_flags(zydis_op_size(instr.operands[0]), flags_op::imul, res, op1, op2);

                        if (form == 1) {
                            switch (zydis_op_size(instr.operands[0])) {
                                case op_size::U_8:
                                    emit_store_reg(op_size::U_16, res, cpu_reg::EAX);
                                    break;
                                case op_size::U_16:
                                    emit_store_reg(op_size::U_16, res, cpu_reg::EAX);
                                    masm.Lsr(res.w(), res.w(), 16);
                                    emit_store_reg(op_size::U_16, res, cpu_reg::EDX);
                                    break;
                                case op_size::U_32:
                                    emit_store_reg(op_size::U_32, res, cpu_reg::EAX);
                                    masm.Lsr(res.x(), res.x(), 32);
                                    emit_store_reg(op_size::U_32, res, cpu_reg::EDX);
                                    break;
                                default:
                                    throw std::exception();
                            }
                        } else
                            emit_store_operand(res, instr.operands[0]);
                    }
                        break;

                    case ZYDIS_MNEMONIC_MUL:
                    {
                        auto op1 = get_tmp_reg();
                        emit_load_operand(zydis_op_size(instr.operands[0]), op1, instr.operands[0]);
                        auto op2 = get_tmp_reg();
                        emit_load_reg(zydis_op_size(instr.operands[0]), op2, cpu_reg::EAX);

                        auto res = get_tmp_reg();

                        masm.Umull(res.x(), op1.w(), op2.w());

                        emit_update_flags(zydis_op_size(instr.operands[0]), flags_op::mul, res, op1, op2);

                        switch (zydis_op_size(instr.operands[0])) {
                            case op_size::U_8:
                                emit_store_reg(op_size::U_16, res, cpu_reg::EAX);
                                break;
                            case op_size::U_16:
                                emit_store_reg(op_size::U_16, res, cpu_reg::EAX);
                                masm.Lsr(res.w(), res.w(), 16);
                                emit_store_reg(op_size::U_16, res, cpu_reg::EDX);
                                break;
                            case op_size::U_32:
                                emit_store_reg(op_size::U_32, res, cpu_reg::EAX);
                                masm.Lsr(res.x(), res.x(), 32);
                                emit_store_reg(op_size::U_32, res, cpu_reg::EDX);
                                break;
                            default:
                                throw std::exception();
                        }
                    }
                        break;

                    case ZYDIS_MNEMONIC_LEA: {
                        auto eaddr = emit_load_effective_address_tmp(instr.operands[1]);
                        emit_store_operand(eaddr, instr.operands[0]);
                    }
                        break;


                    case ZYDIS_MNEMONIC_JB:
                        emit_conditional_jump<cpu_condition::b>(instr);
                        break;
                    case ZYDIS_MNEMONIC_JNB:
                        emit_conditional_jump<cpu_condition::nb>(instr);
                        break;
                    case ZYDIS_MNEMONIC_JBE:
                        emit_conditional_jump<cpu_condition::be>(instr);
                        break;
                    case ZYDIS_MNEMONIC_JNBE:
                        emit_conditional_jump<cpu_condition::nbe>(instr);
                        break;
                    case ZYDIS_MNEMONIC_JL:
                        emit_conditional_jump<cpu_condition::l>(instr);
                        break;
                    case ZYDIS_MNEMONIC_JNL:
                        emit_conditional_jump<cpu_condition::nl>(instr);
                        break;
                    case ZYDIS_MNEMONIC_JLE:
                        emit_conditional_jump<cpu_condition::le>(instr);
                        break;
                    case ZYDIS_MNEMONIC_JNLE:
                        emit_conditional_jump<cpu_condition::nle>(instr);
                        break;
                    case ZYDIS_MNEMONIC_JO:
                        emit_conditional_jump<cpu_condition::o>(instr);
                        break;
                    case ZYDIS_MNEMONIC_JNO:
                        emit_conditional_jump<cpu_condition::no>(instr);
                        break;
                    case ZYDIS_MNEMONIC_JP:
                        emit_conditional_jump<cpu_condition::p>(instr);
                        break;
                    case ZYDIS_MNEMONIC_JNP:
                        emit_conditional_jump<cpu_condition::np>(instr);
                        break;
                    case ZYDIS_MNEMONIC_JS:
                        emit_conditional_jump<cpu_condition::s>(instr);
                        break;
                    case ZYDIS_MNEMONIC_JNS:
                        emit_conditional_jump<cpu_condition::ns>(instr);
                        break;
                    case ZYDIS_MNEMONIC_JZ:
                        emit_conditional_jump<cpu_condition::z>(instr);
                        break;
                    case ZYDIS_MNEMONIC_JNZ:
                        emit_conditional_jump<cpu_condition::nz>(instr);
                        break;

                    case ZYDIS_MNEMONIC_SETB:
                        emit_conditional_set<cpu_condition::b>(instr);
                        break;
                    case ZYDIS_MNEMONIC_SETNB:
                        emit_conditional_set<cpu_condition::nb>(instr);
                        break;
                    case ZYDIS_MNEMONIC_SETBE:
                        emit_conditional_set<cpu_condition::be>(instr);
                        break;
                    case ZYDIS_MNEMONIC_SETNBE:
                        emit_conditional_set<cpu_condition::nbe>(instr);
                        break;
                    case ZYDIS_MNEMONIC_SETL:
                        emit_conditional_set<cpu_condition::l>(instr);
                        break;
                    case ZYDIS_MNEMONIC_SETNL:
                        emit_conditional_set<cpu_condition::nl>(instr);
                        break;
                    case ZYDIS_MNEMONIC_SETLE:
                        emit_conditional_set<cpu_condition::le>(instr);
                        break;
                    case ZYDIS_MNEMONIC_SETNLE:
                        emit_conditional_set<cpu_condition::nle>(instr);
                        break;
                    case ZYDIS_MNEMONIC_SETO:
                        emit_conditional_set<cpu_condition::o>(instr);
                        break;
                    case ZYDIS_MNEMONIC_SETNO:
                        emit_conditional_set<cpu_condition::no>(instr);
                        break;
                    case ZYDIS_MNEMONIC_SETP:
                        emit_conditional_set<cpu_condition::p>(instr);
                        break;
                    case ZYDIS_MNEMONIC_SETNP:
                        emit_conditional_set<cpu_condition::np>(instr);
                        break;
                    case ZYDIS_MNEMONIC_SETS:
                        emit_conditional_set<cpu_condition::s>(instr);
                        break;
                    case ZYDIS_MNEMONIC_SETNS:
                        emit_conditional_set<cpu_condition::ns>(instr);
                        break;
                    case ZYDIS_MNEMONIC_SETZ:
                        emit_conditional_set<cpu_condition::z>(instr);
                        break;
                    case ZYDIS_MNEMONIC_SETNZ:
                        emit_conditional_set<cpu_condition::nz>(instr);
                        break;

                    case ZYDIS_MNEMONIC_OR: {
                        auto op1 = emit_load_operand_tmp(zydis_op_size(instr.operands[0]), instr.operands[0]);
                        auto op2 = emit_load_operand_tmp(zydis_op_size(instr.operands[0]), instr.operands[1]);
                        auto res = get_tmp_reg();

                        masm.Orr(res.w(), op1.w(), op2.w());
                        emit_update_flags(zydis_op_size(instr.operands[0]), flags_op::or_, res, op1, op2);

                        emit_store_operand(res, instr.operands[0]);
                    }
                        break;
                    case ZYDIS_MNEMONIC_XOR: {
                        auto op1 = emit_load_operand_tmp(zydis_op_size(instr.operands[0]), instr.operands[0]);
                        auto op2 = emit_load_operand_tmp(zydis_op_size(instr.operands[0]), instr.operands[1]);
                        auto res = get_tmp_reg();

                        masm.Eor(res.w(), op1.w(), op2.w());
                        emit_update_flags(zydis_op_size(instr.operands[0]), flags_op::xor_, res, op1, op2);

                        emit_store_operand(res, instr.operands[0]);
                    }
                        break;
                    case ZYDIS_MNEMONIC_AND:
                    case ZYDIS_MNEMONIC_TEST: {
                        auto op1 = emit_load_operand_tmp(zydis_op_size(instr.operands[0]), instr.operands[0]);
                        auto op2 = emit_load_operand_tmp(zydis_op_size(instr.operands[0]), instr.operands[1]);
                        auto res = get_tmp_reg();
                        masm.Ands(res.w(), op1.w(), op2.w());
                        emit_update_flags(zydis_op_size(instr.operands[0]), flags_op::and_, res, op1, op2);
                        if (instr.mnemonic == ZYDIS_MNEMONIC_AND)
                            emit_store_operand(res, instr.operands[0]);
                    }
                        break;

                    case ZYDIS_MNEMONIC_SHR: {
                        auto val = emit_load_operand_tmp(zydis_op_size(instr.operands[0]), instr.operands[0]);
                        auto amnt = emit_load_operand_tmp(op_size::U_8, instr.operands[1]);
                        auto res = get_tmp_reg();

                        masm.And(amnt.w(), amnt.w(), 31);

                        vixl::aarch64::Label skip;
                        masm.Cbz(amnt.w(), &skip);

                        masm.Lsr(res.w(), val.w(), amnt.w());

                        emit_update_flags(zydis_op_size(instr.operands[0]), flags_op::shr, res, val, amnt);

                        emit_store_operand(res, instr.operands[0]);

                        masm.Bind(&skip);
                    }
                        break;
                    case ZYDIS_MNEMONIC_SAR: {
                        auto val = emit_load_operand_tmp(zydis_op_size<true>(instr.operands[0]), instr.operands[0]);
                        auto amnt = emit_load_operand_tmp(op_size::U_8, instr.operands[1]);
                        auto res = get_tmp_reg();

                        masm.And(amnt.w(), amnt.w(), 31);

                        vixl::aarch64::Label skip;
                        masm.Cbz(amnt.w(), &skip);

                        masm.Asr(res.w(), val.w(), amnt.w());

                        emit_update_flags(zydis_op_size(instr.operands[0]), flags_op::sar, res, val, amnt);

                        emit_store_operand(res, instr.operands[0]);

                        masm.Bind(&skip);
                    }
                        break;

                    case ZYDIS_MNEMONIC_SHL: {
                        auto val = emit_load_operand_tmp(zydis_op_size(instr.operands[0]), instr.operands[0]);
                        auto amnt = emit_load_operand_tmp(op_size::U_8, instr.operands[1]);
                        auto res = get_tmp_reg();

                        masm.And(amnt.w(), amnt.w(), 31);

                        vixl::aarch64::Label skip;
                        masm.Cbz(amnt.w(), &skip);

                        masm.Lsl(res.w(), val.w(), amnt.w());

                        emit_update_flags(zydis_op_size(instr.operands[0]), flags_op::shl, res, val, amnt);

                        emit_store_operand(res, instr.operands[0]);

                        masm.Bind(&skip);
                    }
                        break;

                    case ZYDIS_MNEMONIC_ROL: {
                        auto val = emit_load_operand_tmp(zydis_op_size(instr.operands[0]), instr.operands[0]);
                        auto amnt = emit_load_operand_tmp(op_size::U_8, instr.operands[1]);
                        auto res = get_tmp_reg();

                        masm.And(amnt.w(), amnt.w(), 31);

                        vixl::aarch64::Label skip;
                        masm.Cbz(amnt.w(), &skip);

                        auto amnt_mod_sz = get_tmp_reg();
                        masm.And(amnt_mod_sz.w(), amnt.w(), instr.operands[0].size - 1); // a bit of bit magic

                        masm.Lsl(res.w(), val.w(), amnt_mod_sz.w());

                        {
                            auto tmp = emit_load_imm_tmp(instr.operands[0].size);
                            masm.Sub(tmp.w(), tmp.w(), amnt_mod_sz.w());
                            masm.Lsr(tmp.w(), val.w(), tmp.w());
                            masm.Orr(res.w(), res.w(), tmp.w());
                        }

                        emit_update_flags(zydis_op_size(instr.operands[0]), flags_op::rol, res, val, amnt);

                        emit_store_operand(res, instr.operands[0]);

                        masm.Bind(&skip);
                    }
                        break;

                    case ZYDIS_MNEMONIC_ROR:
                    {
                        auto val = emit_load_operand_tmp(zydis_op_size(instr.operands[0]), instr.operands[0]);
                        auto amnt = emit_load_operand_tmp(op_size::U_8, instr.operands[1]);
                        auto res = get_tmp_reg();

                        masm.And(amnt.w(), amnt.w(), 31);

                        vixl::aarch64::Label skip;
                        masm.Cbz(amnt.w(), &skip);

                        auto amnt_mod_sz = get_tmp_reg();
                        masm.And(amnt_mod_sz.w(), amnt.w(), instr.operands[0].size - 1); // a bit of bit magic

                        masm.Lsr(res.w(), val.w(), amnt_mod_sz.w());

                        {
                            auto tmp = emit_load_imm_tmp(instr.operands[0].size);
                            masm.Sub(tmp.w(), tmp.w(), amnt_mod_sz.w());
                            masm.Lsl(tmp.w(), val.w(), tmp.w());
                            masm.Orr(res.w(), res.w(), tmp.w());
                        }


                        emit_update_flags(zydis_op_size(instr.operands[0]), flags_op::ror, res, val, amnt);

                        emit_store_operand(res, instr.operands[0]);

                        masm.Bind(&skip);
                    }
                        break;

                    case ZYDIS_MNEMONIC_MOVZX: {
                        auto val = emit_load_operand_tmp(zydis_op_size<false>(instr.operands[1]), instr.operands[1]);
                        emit_store_operand(val, instr.operands[0]);
                    }
                        break;

                    case ZYDIS_MNEMONIC_MOVSX: {
                        auto val = emit_load_operand_tmp(zydis_op_size<true>(instr.operands[1]), instr.operands[1]);
                        emit_store_operand(val, instr.operands[0]);
                    }
                        break;

                    case ZYDIS_MNEMONIC_INT: {
                        assert(used_tmp_regs.empty());
                        assert(instr.operands[0].type == ZYDIS_OPERAND_TYPE_IMMEDIATE);
                        assert(!instr.operands[0].imm.is_relative && !instr.operands[0].imm.is_signed);

                        emit_interrupt(instr.operands[0].imm.value.u);
                    }
                        break;

                    case ZYDIS_MNEMONIC_MOVSB: {
                        assert(!(instr.attributes & (ZYDIS_ATTRIB_HAS_REPE | ZYDIS_ATTRIB_HAS_REPNE)));

                        if (instr.attributes & ZYDIS_ATTRIB_HAS_REP) {
                            register_saver_scope saver(*this, false);

                            masm.CallRuntime(rep_movs_runtime_helper < 1 > );
                        } else {
                            auto saddr = emit_load_reg_tmp(op_size::U_32, cpu_reg::ESI);
                            auto daddr = emit_load_reg_tmp(op_size::U_32, cpu_reg::EDI);

                            auto tmp = emit_load_memory_tmp(op_size::U_8, cpu_segment::ES, saddr);
                            emit_store_memory(op_size::U_8, tmp, cpu_segment::ES, daddr);

                            masm.Add(saddr.w(), saddr.w(), 1);
                            masm.Add(daddr.w(), daddr.w(), 1);
                            emit_store_reg(op_size::U_32, saddr, cpu_reg::ESI);
                            emit_store_reg(op_size::U_32, daddr, cpu_reg::EDI);
                        }
                    }
                        break;
                    case ZYDIS_MNEMONIC_MOVSW: {
                        assert(!(instr.attributes & (ZYDIS_ATTRIB_HAS_REPE | ZYDIS_ATTRIB_HAS_REPNE)));
                        if (instr.attributes & ZYDIS_ATTRIB_HAS_REP) {
                            register_saver_scope saver(*this, false);

                            masm.CallRuntime(rep_movs_runtime_helper < 2 > );
                        } else {
                            auto saddr = emit_load_reg_tmp(op_size::U_32, cpu_reg::ESI);
                            auto daddr = emit_load_reg_tmp(op_size::U_32, cpu_reg::EDI);

                            auto tmp = emit_load_memory_tmp(op_size::U_16, cpu_segment::ES, saddr);
                            emit_store_memory(op_size::U_16, tmp, cpu_segment::ES, daddr);

                            masm.Add(saddr.w(), saddr.w(), 2);
                            masm.Add(daddr.w(), daddr.w(), 2);
                            emit_store_reg(op_size::U_32, saddr, cpu_reg::ESI);
                            emit_store_reg(op_size::U_32, daddr, cpu_reg::EDI);
                        }
                    }
                        break;
                    case ZYDIS_MNEMONIC_MOVSD: {
                        assert(!(instr.attributes & (ZYDIS_ATTRIB_HAS_REPE | ZYDIS_ATTRIB_HAS_REPNE)));
                        if (instr.attributes & ZYDIS_ATTRIB_HAS_REP) {
                            register_saver_scope saver(*this, false);

                            masm.CallRuntime(rep_movs_runtime_helper < 4 > );
                        } else {
                            auto saddr = emit_load_reg_tmp(op_size::U_32, cpu_reg::ESI);
                            auto daddr = emit_load_reg_tmp(op_size::U_32, cpu_reg::EDI);

                            auto tmp = emit_load_memory_tmp(op_size::U_32, cpu_segment::ES, saddr);
                            emit_store_memory(op_size::U_32, tmp, cpu_segment::ES, daddr);

                            masm.Add(saddr.w(), saddr.w(), 4);
                            masm.Add(daddr.w(), daddr.w(), 4);
                            emit_store_reg(op_size::U_32, saddr, cpu_reg::ESI);
                            emit_store_reg(op_size::U_32, daddr, cpu_reg::EDI);
                        }
                    }
                        break;

                    case ZYDIS_MNEMONIC_STOSB: {
                        assert(!(instr.attributes & (ZYDIS_ATTRIB_HAS_REPE | ZYDIS_ATTRIB_HAS_REPNE)));
                        if (instr.attributes & ZYDIS_ATTRIB_HAS_REP) {
                            register_saver_scope saver(*this, false);

                            masm.CallRuntime(rep_stos_runtime_helper < uint8_t > );
                        } else {
                            auto addr = emit_load_reg_tmp(op_size::U_32, cpu_reg::EDI);
                            auto val = emit_load_reg_tmp(op_size::U_8, cpu_reg::EAX); // AL actually
                            emit_store_memory(op_size::U_8, val, cpu_segment::ES, addr);

                            masm.Add(addr.w(), addr.w(), 1);
                            emit_store_reg(op_size::U_32, addr, cpu_reg::EDI);
                        }
                    }
                        break;
                    case ZYDIS_MNEMONIC_STOSW: {
                        assert(!(instr.attributes & (ZYDIS_ATTRIB_HAS_REPE | ZYDIS_ATTRIB_HAS_REPNE)));
                        if (instr.attributes & ZYDIS_ATTRIB_HAS_REP) {
                            register_saver_scope saver(*this, false);

                            masm.CallRuntime(rep_stos_runtime_helper < uint16_t > );
                        } else {
                            auto addr = emit_load_reg_tmp(op_size::U_32, cpu_reg::EDI);
                            auto val = emit_load_reg_tmp(op_size::U_16, cpu_reg::EAX); // AX actually
                            emit_store_memory(op_size::U_16, val, cpu_segment::ES, addr);

                            masm.Add(addr.w(), addr.w(), 2);
                            emit_store_reg(op_size::U_32, addr, cpu_reg::EDI);
                        }
                    }
                        break;
                    case ZYDIS_MNEMONIC_STOSD: {
                        assert(!(instr.attributes & (ZYDIS_ATTRIB_HAS_REPE | ZYDIS_ATTRIB_HAS_REPNE)));
                        if (instr.attributes & ZYDIS_ATTRIB_HAS_REP) {
                            register_saver_scope saver(*this, false);

                            masm.CallRuntime(rep_stos_runtime_helper < uint32_t > );
                        } else {
                            auto addr = emit_load_reg_tmp(op_size::U_32, cpu_reg::EDI);
                            auto val = emit_load_reg_tmp(op_size::U_32, cpu_reg::EAX);
                            emit_store_memory(op_size::U_32, val, cpu_segment::ES, addr);

                            masm.Add(addr.w(), addr.w(), 4);
                            emit_store_reg(op_size::U_32, addr, cpu_reg::EDI);
                        }
                    }
                        break;
                    case ZYDIS_MNEMONIC_SCASB: {
                        assert(instr.attributes & ZYDIS_ATTRIB_HAS_REPNE || instr.attributes & ZYDIS_ATTRIB_HAS_REPE);
                        if (instr.attributes & ZYDIS_ATTRIB_HAS_REPNE)
                        {
                            register_saver_scope saver(*this, false);
                            masm.CallRuntime(repne_scas_runtime_helper < uint8_t > );
                        } else if (instr.attributes & ZYDIS_ATTRIB_HAS_REPE) {
                            register_saver_scope saver(*this, false);
                            masm.CallRuntime(repe_scas_runtime_helper < uint8_t > );
                        } else {
                            throw std::exception();
                        }

                        auto tmp = emit_load_reg_tmp(op_size::U_32, cpu_reg::EDI);
                        masm.Sub(tmp.w(), tmp.w(), 1);
                        auto val = emit_load_memory_tmp(op_size::U_8, cpu_segment::ES, tmp);
                        auto al = emit_load_reg_tmp(op_size::U_8, cpu_reg::EAX);

                        auto res = get_tmp_reg();
                        masm.Subs(res.w(), al.w(), val.w());

                        emit_update_flags(op_size::U_8, flags_op::sub, res, al, val);
                    }
                        break;
                    case ZYDIS_MNEMONIC_SCASW: {
                        assert(instr.attributes & ZYDIS_ATTRIB_HAS_REPNE || instr.attributes & ZYDIS_ATTRIB_HAS_REPE);
                        if (instr.attributes & ZYDIS_ATTRIB_HAS_REPNE)
                        {
                            register_saver_scope saver(*this, false);
                            masm.CallRuntime(repne_scas_runtime_helper < uint16_t > );
                        } else if (instr.attributes & ZYDIS_ATTRIB_HAS_REPE) {
                            register_saver_scope saver(*this, false);
                            masm.CallRuntime(repe_scas_runtime_helper < uint16_t > );
                        } else {
                            throw std::exception();
                        }

                        auto tmp = emit_load_reg_tmp(op_size::U_32, cpu_reg::EDI);
                        masm.Sub(tmp.w(), tmp.w(), 2);
                        auto val = emit_load_memory_tmp(op_size::U_16, cpu_segment::ES, tmp);
                        auto ax = emit_load_reg_tmp(op_size::U_16, cpu_reg::EAX);

                        auto res = get_tmp_reg();
                        masm.Subs(res.w(), ax.w(), val.w());

                        emit_update_flags(op_size::U_16, flags_op::sub, res, ax, val);
                    }
                        break;
                    case ZYDIS_MNEMONIC_SCASD: {
                        assert(instr.attributes & ZYDIS_ATTRIB_HAS_REPNE || instr.attributes & ZYDIS_ATTRIB_HAS_REPE);
                        if (instr.attributes & ZYDIS_ATTRIB_HAS_REPNE)
                        {
                            register_saver_scope saver(*this, false);
                            masm.CallRuntime(repne_scas_runtime_helper < uint32_t > );
                        } else if (instr.attributes & ZYDIS_ATTRIB_HAS_REPE) {
                            register_saver_scope saver(*this, false);
                            masm.CallRuntime(repe_scas_runtime_helper < uint32_t > );
                        } else {
                            throw std::exception();
                        }

                        auto tmp = emit_load_reg_tmp(op_size::U_32, cpu_reg::EDI);
                        masm.Sub(tmp.w(), tmp.w(), 4);
                        auto val = emit_load_memory_tmp(op_size::U_32, cpu_segment::ES, tmp);
                        auto eax = emit_load_reg_tmp(op_size::U_32, cpu_reg::EAX);

                        auto res = get_tmp_reg();
                        masm.Subs(res.w(), eax.w(), val.w());

                        emit_update_flags(op_size::U_32, flags_op::sub, res, eax, val);
                    }
                        break;

                    case ZYDIS_MNEMONIC_CMPSB:
                    {
                        assert(instr.attributes & ZYDIS_ATTRIB_HAS_REPE);
                        {
                            register_saver_scope saver(*this, false);
                            masm.CallRuntime(repe_cmps_runtime_helper<uint8_t> );
                        }

                        auto tmp = emit_load_reg_tmp(op_size::U_32, cpu_reg::ESI);
                        masm.Sub(tmp.w(), tmp.w(), 1);
                        auto sval = emit_load_memory_tmp(op_size::U_8, cpu_segment::ES, tmp);
                        emit_load_reg(op_size::U_32, tmp, cpu_reg::EDI);
                        masm.Sub(tmp.w(), tmp.w(), 1);
                        auto dval = emit_load_memory_tmp(op_size::U_8, cpu_segment::ES, tmp);

                        auto res = get_tmp_reg();
                        masm.Subs(res.w(), sval.w(), dval.w());

                        emit_update_flags(op_size::U_8, flags_op::sub, res, sval, dval);
                    }
                        break;
                    case ZYDIS_MNEMONIC_CMPSW:
                    {
                        assert(instr.attributes & ZYDIS_ATTRIB_HAS_REPE);
                        {
                            register_saver_scope saver(*this, false);
                            masm.CallRuntime(repe_cmps_runtime_helper<uint16_t> );
                        }

                        auto tmp = emit_load_reg_tmp(op_size::U_32, cpu_reg::ESI);
                        masm.Sub(tmp.w(), tmp.w(), 2);
                        auto sval = emit_load_memory_tmp(op_size::U_16, cpu_segment::ES, tmp);
                        emit_load_reg(op_size::U_32, tmp, cpu_reg::EDI);
                        masm.Sub(tmp.w(), tmp.w(), 2);
                        auto dval = emit_load_memory_tmp(op_size::U_16, cpu_segment::ES, tmp);

                        auto res = get_tmp_reg();
                        masm.Subs(res.w(), sval.w(), dval.w());

                        emit_update_flags(op_size::U_16, flags_op::sub, res, sval, dval);
                    }
                        break;
                    case ZYDIS_MNEMONIC_CMPSD:
                    {
                        assert(instr.attributes & ZYDIS_ATTRIB_HAS_REPE);
                        {
                            register_saver_scope saver(*this, false);
                            masm.CallRuntime(repe_cmps_runtime_helper<uint8_t> );
                        }

                        auto tmp = emit_load_reg_tmp(op_size::U_32, cpu_reg::ESI);
                        masm.Sub(tmp.w(), tmp.w(), 4);
                        auto sval = emit_load_memory_tmp(op_size::U_32, cpu_segment::ES, tmp);
                        emit_load_reg(op_size::U_32, tmp, cpu_reg::EDI);
                        masm.Sub(tmp.w(), tmp.w(), 4);
                        auto dval = emit_load_memory_tmp(op_size::U_32, cpu_segment::ES, tmp);

                        auto res = get_tmp_reg();
                        masm.Subs(res.w(), sval.w(), dval.w());

                        emit_update_flags(op_size::U_32, flags_op::sub, res, sval, dval);
                    }
                        break;

                    case ZYDIS_MNEMONIC_CBW: {
                        auto al = emit_load_reg_tmp(op_size::S_8, cpu_reg::EAX);
                        emit_store_reg(op_size::U_16, al, cpu_reg::EAX);
                    }
                        break;
                    case ZYDIS_MNEMONIC_CWDE: {
                        auto ax = emit_load_reg_tmp(op_size::S_16, cpu_reg::EAX);
                        emit_store_reg(op_size::U_32, ax, cpu_reg::EAX);
                    }
                        break;
                    case ZYDIS_MNEMONIC_CWD: {
                        auto ax = emit_load_reg_tmp(op_size::S_16, cpu_reg::EAX);
                        masm.Lsr(ax.x(), ax.x(), 16);
                        emit_store_reg(op_size::U_16, ax, cpu_reg::EDX);
                    }
                        break;

                    case ZYDIS_MNEMONIC_CDQ: {
                        auto eax = emit_load_reg_tmp(op_size::S_32, cpu_reg::EAX);
                        masm.Lsr(eax.x(), eax.x(), 32);
                        emit_store_reg(op_size::U_32, eax, cpu_reg::EDX);
                    }
                        break;

                    case ZYDIS_MNEMONIC_NOP:
                        // The easiest one =)
                        break;

                    case ZYDIS_MNEMONIC_CLD: {
                        // D flag is assumed to be not set, so nothing to do here
                    }
                        break;

                    case ZYDIS_MNEMONIC_PUSHFD:
                    {
                        auto eflags = emit_load_reg_tmp(op_size::U_32, cpu_reg::EFLAGS);

                        auto esp = emit_load_reg_tmp(op_size::U_32, cpu_reg::ESP);
                        masm.Sub(esp.w(), esp.w(), 4);

                        emit_store_memory(op_size::U_32, eflags, cpu_segment::SS, esp);

                        emit_store_reg(op_size::U_32, esp, cpu_reg::ESP);
                    }
                        break;

                    case ZYDIS_MNEMONIC_POPFD:
                    {
                        auto esp = emit_load_reg_tmp(op_size::U_32, cpu_reg::ESP);

                        auto new_eflags = emit_load_memory_tmp(op_size::U_32, cpu_segment::SS, esp);

                        masm.Add(esp.w(), esp.w(), 4);

                        emit_store_reg(op_size::U_32, esp, cpu_reg::ESP);
                        emit_update_flags(op_size::U_32, flags_op::load_flags, new_eflags, new_eflags, new_eflags);
                    }
                        break;

                    case ZYDIS_MNEMONIC_SAHF:
                    {
                        auto ref = ref_cpu_reg(cpu_reg::EAX);
                        ref.AddOffset(1); // load AH

                        auto new_eflags = emit_load_sized_tmp(op_size::U_8, ref);
                        auto old_eflags = emit_load_reg_tmp(op_size::U_32, cpu_reg::EFLAGS);
                        masm.And(old_eflags.w(), old_eflags.w(), 0xffffff00ULL);
                        masm.Orr(new_eflags.w(), new_eflags.w(), old_eflags.w());
                        emit_update_flags(op_size::U_32, flags_op::load_flags, new_eflags, new_eflags, new_eflags);
                    }
                        break;

                    case ZYDIS_MNEMONIC_CPUID:
                    {
                        register_saver_scope saver(*this, false);
                        masm.CallRuntime(cpuid_helper);
                    }
                        break;

                    case ZYDIS_MNEMONIC_BSF:
                    {
                        auto src = emit_load_operand_tmp(zydis_op_size(instr.operands[1]), instr.operands[1]);
                        auto tmp = get_tmp_reg();
                        masm.Rbit(tmp.w(), src.w());
                        masm.Clz(tmp.w(), tmp.w());

                        emit_update_flags(zydis_op_size(instr.operands[1]), flags_op::bsf, tmp, src, src);

                        emit_store_operand(tmp, instr.operands[0]);

                        /*
                        emit_load_reg(op_size::U_32, tmp, cpu_reg::EFLAGS);
                        masm.And(tmp.w(), tmp.w(), ~(uint32_t)F_ZF);
                        masm.Orr(tmp.w(), tmp.w(), tmp_flags.w());
                        emit_store_reg(op_size::U_32, tmp, cpu_reg::EFLAGS);
                         */
                    }
                        break;


                    case ZYDIS_MNEMONIC_SHLD:
                    {
                        auto amnt = emit_load_operand_tmp(op_size::U_8, instr.operands[2]);

                        vixl::aarch64::Label skip;

                        masm.Cbz(amnt.w(), &skip);

                        auto val = emit_load_operand_tmp(zydis_op_size(instr.operands[0]), instr.operands[0]);
                        auto res = get_tmp_reg();

                        masm.Lsl(res.w(), val.w(), amnt.w());

                        {
                            auto aux = emit_load_operand_tmp(zydis_op_size(instr.operands[1]), instr.operands[1]);

                            int sz = instr.operands[0].size;
                            auto tmp = emit_load_imm_tmp(sz);
                            masm.Sub(tmp.w(), tmp.w(), amnt.w());
                            masm.Lsr(tmp.w(), aux.w(), tmp.w());
                            masm.Orr(res.w(), res.w(), tmp.w());
                        }


                        emit_update_flags(zydis_op_size(instr.operands[0]), flags_op::shld, res, val, amnt);

                        emit_store_operand(res, instr.operands[0]);

                        masm.Bind(&skip);
                    }
                        break;
                    case ZYDIS_MNEMONIC_SHRD:
                    {
                        auto amnt = emit_load_operand_tmp(op_size::U_8, instr.operands[2]);

                        vixl::aarch64::Label skip;

                        masm.Cbz(amnt.w(), &skip);

                        auto val = emit_load_operand_tmp(zydis_op_size(instr.operands[0]), instr.operands[0]);
                        auto res = get_tmp_reg();

                        masm.Lsr(res.w(), val.w(), amnt.w());

                        {
                            auto aux = emit_load_operand_tmp(zydis_op_size(instr.operands[1]), instr.operands[1]);

                            int sz = instr.operands[0].size;
                            auto tmp = emit_load_imm_tmp(sz);
                            masm.Sub(tmp.w(), tmp.w(), amnt.w());
                            masm.Lsl(tmp.w(), aux.w(), tmp.w());
                            masm.Orr(res.w(), res.w(), tmp.w());
                        }


                        emit_update_flags(zydis_op_size(instr.operands[0]), flags_op::shrd, res, val, amnt);

                        emit_store_operand(res, instr.operands[0]);

                        masm.Bind(&skip);
                    }
                        break;
                    case ZYDIS_MNEMONIC_RCR:
                    {
                        vixl::aarch64::Label skipall;
                        auto amnt = emit_load_operand_tmp(op_size::U_8, instr.operands[1]);

                        masm.Cbz(amnt.w(), &skipall);

                        auto cond = emit_test_condition<cpu_condition::b>();

                        auto val = emit_load_operand_tmp(zydis_op_size(instr.operands[0]), instr.operands[0]);
                        auto res = get_tmp_reg();

                        masm.And(amnt.w(), amnt.w(), 31);

                        auto cval = get_tmp_reg();
                        masm.Csel(cval.w(), 1, 0, cond);

                        // amnt = amnt mod (sz + 1)
                        {
                            {
                                auto tmp = get_tmp_reg();
                                auto mod = emit_load_imm_tmp(instr.operands[0].size + 1);
                                masm.Udiv(tmp.w(), amnt.w(), mod.w());
                                masm.Msub(amnt.w(), tmp.w(), mod.w(), amnt.w());
                            }

                            masm.Cbz(amnt.w(), &skipall);


                            masm.Lsr(res.w(), val.w(), amnt.w());

                            {
                                auto shift_comp = emit_load_imm_tmp(instr.operands[0].size + 1);
                                masm.Sub(shift_comp.w(), shift_comp.w(), amnt.w());

                                vixl::aarch64::Label skip;
                                masm.Cmp(amnt.w(), 1);
                                masm.B(vixl::aarch64::Condition::eq, &skip);
                                {
                                    auto tmp = get_tmp_reg();
                                    masm.Lsl(tmp.w(), val.w(), shift_comp.w());
                                    masm.Orr(res.w(), res.w(), tmp.w());
                                }

                                masm.Bind(&skip);

                                masm.Sub(shift_comp.w(), shift_comp.w(), 1);

                                masm.Lsl(cval.w(), cval.w(), shift_comp.w());
                                masm.Orr(res.w(), res.w(), cval.w());
                            }
                        }

                        emit_update_flags(zydis_op_size(instr.operands[0]), flags_op::rcr, res, val, amnt);

                        emit_store_operand(res, instr.operands[0]);

                        masm.Bind(&skipall);
                    }
                        break;

                    case ZYDIS_MNEMONIC_XLAT:
                    {
                        auto base = emit_load_reg_tmp(op_size::U_32, cpu_reg::EBX);
                        auto index = emit_load_reg_tmp(op_size::U_8, cpu_reg::EAX);
                        masm.Add(base.w(), base.w(), index.w());
                        emit_load_memory(op_size::U_8, index, cpu_segment::DS, base);
                        emit_store_reg(op_size::U_8, index, cpu_reg::EAX);
                    }
                        break;


                    // those are reached through branch hinting, but not actually executed
                    case ZYDIS_MNEMONIC_BTS:  // not needed
                    case ZYDIS_MNEMONIC_BSR:  // not needed
                    case ZYDIS_MNEMONIC_EMMS: // not needed
                    case ZYDIS_MNEMONIC_MOVQ: // not needed
                    case ZYDIS_MNEMONIC_MOVD: // not needed
                    case ZYDIS_MNEMONIC_BT:   // not needed
                    case ZYDIS_MNEMONIC_STD:  // direction flag = 1 is not supported
                    {
                        hint_block_unreached();
                        emit_interrupt(6); // UD
                    }
                        break;

                    default: {
                        dump_generated_code();
                        uw_log("Unknown instruction\n");
                        ZydisFormatter fmt;
                        ZydisFormatterInit(&fmt, ZYDIS_FORMATTER_STYLE_ATT);
                        char buffer[256];
                        uint32_t addr = current_guest_eip - instr.length;
                        ZydisFormatterFormatInstruction(&fmt, &instr, buffer, sizeof(buffer), addr);
                        uw_log("%08x: %s\n", (unsigned)addr, buffer);

                        throw std::exception();
                    }
                }
            }

#ifdef UW_JIT_FORCE_ONE_INSTRUCTION_BLOCKS
            force_terminate_block = true;
#endif

            if (force_terminate_block) {
                if (!changes_eip(instr) || (instr.mnemonic == ZYDIS_MNEMONIC_INT)) {
                    emit_store_reg(op_size::U_32, emit_load_imm_tmp(guest_eip), cpu_reg::EIP);
                    hint_branch(guest_eip);
                }

                return false; // always build basic blocks of one instruction
            } else
                return !changes_eip(instr) || (instr.mnemonic == ZYDIS_MNEMONIC_INT);
        }

        xmem_piece aarch64_code_generator::commit() {
            size_t instr_count = masm.GetSizeOfCodeGenerated() / 4;

            masm.FinalizeCode();
#ifdef UW_JIT_TRACE
            dump_generated_code();
#endif

            auto begin = masm.GetBuffer()->GetStartAddress<uint8_t*>();
            auto end = masm.GetBuffer()->GetEndAddress<uint8_t*>();
            xmem_piece xmem(end - begin);
            std::memcpy(xmem.rw_ptr(), begin, end - begin);

            assert(masm.GetLabelAddress<uint8_t*>(&start_label) == begin);

            return std::move(xmem);
        }



        tmp_holder aarch64_code_generator::emit_load_effective_address_tmp(const ZydisDecodedOperand &operand) {
            assert(operand.type == ZYDIS_OPERAND_TYPE_MEMORY);

            if (operand.mem.base == ZYDIS_REGISTER_NONE && operand.mem.index == ZYDIS_REGISTER_NONE) {
                assert(operand.mem.disp.has_displacement);

                return emit_load_imm_tmp(operand.mem.disp.value);
            } else if (operand.mem.index == ZYDIS_REGISTER_NONE) {
                if (operand.mem.disp.has_displacement) {
                    auto addr = emit_load_reg_tmp(op_size::U_32, zydis_reg(operand.mem.base));
                    masm.Add(addr.w(), addr.w(), operand.mem.disp.value);

                    return std::move(addr);
                } else {
                    return emit_load_reg_tmp(zydis_reg_size(operand.mem.base), zydis_reg(operand.mem.base));
                }
            } else if (operand.mem.base == ZYDIS_REGISTER_NONE) {
                auto tmp = emit_load_reg_tmp(zydis_reg_size(operand.mem.index), zydis_reg(operand.mem.index));
                int scale_shift;
                switch (operand.mem.scale) {
                    case 1:
                        scale_shift = 0;
                        break;
                    case 2:
                        scale_shift = 1;
                        break;
                    case 4:
                        scale_shift = 2;
                        break;
                    case 8:
                        scale_shift = 3;
                        break;
                    default:
                        throw std::exception();
                }

                masm.Add(tmp.w(), vixl::aarch64::wzr,
                         vixl::aarch64::Operand(tmp.w(), vixl::aarch64::Shift::LSL, scale_shift));

                if (operand.mem.disp.has_displacement) {
                    masm.Add(tmp.w(), tmp.w(), operand.mem.disp.value);
                }

                return std::move(tmp);
            } else {
                auto addr = emit_load_reg_tmp(zydis_reg_size(operand.mem.base), zydis_reg(operand.mem.base));
                auto index = emit_load_reg_tmp(zydis_reg_size(operand.mem.index), zydis_reg(operand.mem.index));
                int scale_shift;
                switch (operand.mem.scale) {
                    case 1:
                        scale_shift = 0;
                        break;
                    case 2:
                        scale_shift = 1;
                        break;
                    case 4:
                        scale_shift = 2;
                        break;
                    case 8:
                        scale_shift = 3;
                        break;
                    default:
                        throw std::exception();
                }

                masm.Add(addr.w(), addr.w(), vixl::aarch64::Operand(index.w(), vixl::aarch64::Shift::LSL, scale_shift));

                if (operand.mem.disp.has_displacement) {
                    masm.Add(addr.w(), addr.w(), operand.mem.disp.value);
                }

                return std::move(addr);
            }
        }

        uint64_t aarch64_code_generator::get_imm_operand(op_size sz, const ZydisDecodedOperand &operand) {
            assert(operand.type == ZYDIS_OPERAND_TYPE_IMMEDIATE);
            uint64_t val = current_guest_eip;
            if (operand.imm.is_relative) {
                if (operand.imm.is_signed)
                    val += operand.imm.value.s;
                else
                    val += operand.imm.value.u;
            } else {
                if (operand.imm.is_signed)
                    val = operand.imm.value.s;
                else
                    val = operand.imm.value.u;
            }
            switch (sz) {
                case op_size::U_8:
                    val &= 0xffLLU;
                    break;
                case op_size::U_16:
                    val &= 0xffffLLU;
                    break;
                case op_size::U_32:
                    val &= 0xffffffffLLU;
                    break;
                case op_size::S_8:
                case op_size::S_16:
                case op_size::S_32:
                    assert(operand.imm.is_signed);
                    break;
                default:
                    throw std::exception();
            }
            return val;
        }

        void aarch64_code_generator::emit_load_operand(op_size sz, const tmp_holder &dst, const ZydisDecodedOperand &operand) {
            switch (operand.type) {
                case ZYDIS_OPERAND_TYPE_IMMEDIATE:
                {
                    emit_load_imm(dst, get_imm_operand(sz, operand));
                }
                    break;

                case ZYDIS_OPERAND_TYPE_REGISTER:
                    if (zydis_is_seg(operand.reg.value)) {
                        uint16_t val;
                        switch (zydis_seg(operand.reg.value)) {
                            case cpu_segment::CS: val = __USER_CS; break;
                            case cpu_segment::DS: val = __USER_DS; break;
                            case cpu_segment::ES: val = __USER_DS; break;
                            case cpu_segment::SS: val = __USER_DS; break;
                            case cpu_segment::FS: val = __USER_FS; break;
                            case cpu_segment::GS: val = __USER_GS; break;
                            default:
                                throw std::exception();
                        }
                        emit_load_imm(dst, val);
                    } else if (!zydis_is_highreg(operand.reg.value)) {
                        emit_load_reg(sz, dst, zydis_reg(operand.reg.value));
                    } else {
                        // hack for the ah, bh, ch and dh
                        auto ref = ref_cpu_reg(zydis_hireg(operand.reg.value));
                        ref.AddOffset(1);
                        emit_load_sized(sz, dst, ref);
                    }
                    break;

                case ZYDIS_OPERAND_TYPE_MEMORY:
                    emit_load_memory(sz, dst, zydis_seg(operand.mem.segment),
                                         emit_load_effective_address_tmp(operand));

                    break;

                default:
                    throw std::exception();
            }
        }

        tmp_holder aarch64_code_generator::emit_load_operand_tmp(op_size sz, const ZydisDecodedOperand &operand) {
            auto tmp = get_tmp_reg();
            emit_load_operand(sz, tmp, operand);
            return std::move(tmp);
        }

        void aarch64_code_generator::emit_store_operand(const tmp_holder& src, const ZydisDecodedOperand& operand) {
            switch (operand.type) {
                case ZYDIS_OPERAND_TYPE_REGISTER:
                    if (zydis_is_seg(operand.reg.value)) {
                        uint16_t val;
                        switch (zydis_seg(operand.reg.value)) {
                            case cpu_segment::CS: val = __USER_CS; break;
                            case cpu_segment::DS: val = __USER_DS; break;
                            case cpu_segment::ES: val = __USER_DS; break;
                            case cpu_segment::SS: val = __USER_DS; break;
                            case cpu_segment::FS: val = __USER_FS; break;
                            case cpu_segment::GS: val = __USER_GS; break;
                            default:
                                throw std::exception();
                        }
                        masm.Cmp(src.w(), val);
                        vixl::aarch64::Label skip;
                        masm.B(vixl::aarch64::Condition::eq, &skip);
                        emit_interrupt(0);
                        masm.Bind(&skip);
                    } else if (!zydis_is_highreg(operand.reg.value))
                        emit_store_reg(zydis_op_size(operand), src, zydis_reg(operand.reg.value));
                    else {
                        // hack for the ah, bh, ch and dh
                        auto ref = ref_cpu_reg(zydis_hireg(operand.reg.value));
                        ref.AddOffset(1); // move to high part;
                        emit_store_sized(zydis_op_size(operand), src, ref);
                    }
                    break;

                case ZYDIS_OPERAND_TYPE_MEMORY:
                    emit_store_memory(zydis_op_size(operand), src, zydis_seg(operand.mem.segment),
                                      emit_load_effective_address_tmp(operand));
                    break;

                default:
                    throw std::exception();
            }
        }


        void aarch64_code_generator::emit_load_reg(op_size sz, const tmp_holder &dst, cpu_reg reg) {
            emit_load_sized(sz, dst, ref_cpu_reg(reg));
        }

        tmp_holder aarch64_code_generator::emit_load_reg_tmp(op_size sz, cpu_reg reg) {
            auto tmp = get_tmp_reg();
            emit_load_reg(sz, tmp, reg);
            return std::move(tmp);
        }

        void aarch64_code_generator::emit_store_reg(op_size sz, const tmp_holder &src, cpu_reg reg) {
            switch (sz) {
                case op_size::S_8:
                case op_size::U_8:
                    masm.Strb(src.w(), ref_cpu_reg(reg));
                    break;
                case op_size::S_16:
                case op_size::U_16:
                    masm.Strh(src.w(), ref_cpu_reg(reg));
                    break;
                case op_size::S_32:
                case op_size::U_32:
                    masm.Str(src.w(), ref_cpu_reg(reg));
                    break;
                default:
                    throw std::exception();
            }
        }


        void aarch64_code_generator::emit_store_sized(op_size sz, const tmp_holder& src, vixl::aarch64::MemOperand mem_op) {
            if (src.is_normal()) {
                switch (sz) {
                    case op_size::S_8:
                    case op_size::U_8:
                        masm.Strb(src.w(), mem_op);
                        break;
                    case op_size::S_16:
                    case op_size::U_16:
                        masm.Strh(src.w(), mem_op);
                        break;
                    case op_size::S_32:
                    case op_size::U_32:
                        masm.Str(src.w(), mem_op);
                        break;
                    case op_size::S_64:
                    case op_size::U_64:
                        masm.Str(src.x(), mem_op);
                        break;
                    default:
                        throw std::exception();
                }
            } else {
                switch (sz) {
                    case op_size::U_16:
                        masm.Str(src.h(), mem_op);
                        break;
                    case op_size::U_32:
                        masm.Str(src.s(), mem_op);
                        break;
                    case op_size::U_64:
                        masm.Str(src.d(), mem_op);
                        break;
                    default:
                        throw std::exception();
                }
            }
        }

        tmp_holder aarch64_code_generator::emit_load_sized_tmp(op_size sz, vixl::aarch64::MemOperand mem_op) {
            auto tmp = get_tmp_reg();
            emit_load_sized(sz, tmp, mem_op);
            return tmp;
        }

        void aarch64_code_generator::emit_load_sized(op_size sz, const tmp_holder& dst, vixl::aarch64::MemOperand mem_op) {
            if (dst.is_normal()) {
                switch (sz) {
                    case op_size::U_64:
                    case op_size::S_64:
                        masm.Ldr(dst.x(), mem_op);
                        break;

                    case op_size::S_32:
                        masm.Ldrsw(dst.x(), mem_op);
                        break;
                    case op_size::U_32:
                        masm.Ldr(dst.w(), mem_op);
                        break;

                    case op_size::S_16:
                        masm.Ldrsh(dst.x(), mem_op);
                        break;
                    case op_size::U_16:
                        masm.Ldrh(dst.w(), mem_op);
                        break;

                    case op_size::S_8:
                        masm.Ldrsb(dst.x(), mem_op);
                        break;
                    case op_size::U_8:
                        masm.Ldrb(dst.w(), mem_op);
                        break;
                    default:
                        throw std::exception();
                }
            } else {
                switch (sz) {
                    case op_size::U_64:
                        masm.Ldr(dst.d(), mem_op);
                        break;

                    case op_size::U_32:
                        masm.Ldr(dst.s(), mem_op);
                        break;

                    case op_size::U_16:
                        masm.Ldr(dst.h(), mem_op);
                        break;
                    default:
                        throw std::exception();
                }
            }
        }

        void aarch64_code_generator::emit_load_sized(op_size sz, const tmp_holder &dst, const tmp_holder& addr) {
            emit_load_sized(sz, dst, vixl::aarch64::MemOperand(addr.x()));
        }


        void aarch64_code_generator::emit_load_memory(op_size sz, const tmp_holder &dst, cpu_segment segment,
                                                      const tmp_holder &offset_register) {
            auto actual_address = get_tmp_reg();
            if (segment == cpu_segment::CS || segment == cpu_segment::DS || segment == cpu_segment::ES || segment == cpu_segment::SS) {
                masm.Add(actual_address.x(), guest_base_reg(),
                         vixl::aarch64::Operand(offset_register.x(), vixl::aarch64::Extend::UXTW));
            } else {
                emit_load_seg(actual_address, segment);
                masm.Add(actual_address.x(), actual_address.x(),
                        vixl::aarch64::Operand(offset_register.x(), vixl::aarch64::Extend::UXTW));
            }

            emit_load_sized(sz, dst, actual_address);
        }

        void aarch64_code_generator::emit_load_memory(op_size sz, const tmp_holder& dst, cpu_segment segment, tmp_holder&& offset_register) {
            if (segment == cpu_segment::CS || segment == cpu_segment::DS || segment == cpu_segment::ES || segment == cpu_segment::SS) {
                masm.Add(offset_register.x(), guest_base_reg(),
                         vixl::aarch64::Operand(offset_register.x(), vixl::aarch64::Extend::UXTW));
            } else {
                auto tmp = emit_load_seg_tmp(segment);
                masm.Add(offset_register.x(), tmp.x(), vixl::aarch64::Operand(offset_register.x(), vixl::aarch64::Extend::UXTW));
            }

            emit_load_sized(sz, dst, offset_register);
        }

        void aarch64_code_generator::emit_store_memory(op_size sz, const tmp_holder &src, cpu_segment segment,
                                                       const tmp_holder& offset_register) {
            auto actual_address = get_tmp_reg();
            if (segment == cpu_segment::CS || segment == cpu_segment::DS || segment == cpu_segment::ES || segment == cpu_segment::SS) {
                masm.Add(actual_address.x(), guest_base_reg(),
                         vixl::aarch64::Operand(offset_register.x(), vixl::aarch64::Extend::UXTW));

            } else {
                emit_load_seg(actual_address, segment);
                masm.Add(actual_address.x(), actual_address.x(),
                         vixl::aarch64::Operand(offset_register.x(), vixl::aarch64::Extend::UXTW));
            }

            emit_store_sized(sz, src, vixl::aarch64::MemOperand(actual_address.x()));
        }


        void aarch64_code_generator::emit_load_imm(const tmp_holder& dst, uint64_t imm) {
            if (!masm.TryOneInstrMoveImmediate(dst.x(), imm))
                masm.Ldr(dst.x(), imm);
        }

        tmp_holder aarch64_code_generator::emit_load_imm_tmp(uint64_t imm) {
            auto tmp = get_tmp_reg();

            emit_load_imm(tmp, imm);

            return std::move(tmp);
        }


#pragma clang diagnostic push
#pragma ide diagnostic ignored "OCSimplifyInspection"
// mappings from aarch64 nzcv to x86 eflags (only on 32-bit operands), on smaller ones hacks needed
// N -> SF
// Z -> ZF
// C -> CF (need to invert if used as borrow)
// V -> OF
        static uint32_t flag_table[16] = {
         // F_SF | F_ZF | F_CF | F_OF          N Z C V
            0    | 0    | 0    | 0   ,      // 0 0 0 0
            0    | 0    | 0    | F_OF,      // 0 0 0 1
            0    | 0    | F_CF | 0   ,      // 0 0 1 0
            0    | 0    | F_CF | F_OF,      // 0 0 1 1
            0    | F_ZF | 0    | 0   ,      // 0 1 0 0
            0    | F_ZF | 0    | F_OF,      // 0 1 0 1
            0    | F_ZF | F_CF | 0   ,      // 0 1 1 0
            0    | F_ZF | F_CF | F_OF,      // 0 1 1 1
            F_SF | 0    | 0    | 0   ,      // 1 0 0 0
            F_SF | 0    | 0    | F_OF,      // 1 0 0 1
            F_SF | 0    | F_CF | 0   ,      // 1 0 1 0
            F_SF | 0    | F_CF | F_OF,      // 1 0 1 1
            F_SF | F_ZF | 0    | 0   ,      // 1 1 0 0
            F_SF | F_ZF | 0    | F_OF,      // 1 1 0 1
            F_SF | F_ZF | F_CF | 0   ,      // 1 1 1 0
            F_SF | F_ZF | F_CF | F_OF,      // 1 1 1 1
        };

        static uint32_t inverted_c_flag_table[16] = {
         // F_SF | F_ZF | F_CF | F_OF          N Z C V
            0    | 0    | F_CF | 0   ,      // 0 0 0 0
            0    | 0    | F_CF | F_OF,      // 0 0 0 1
            0    | 0    | 0    | 0   ,      // 0 0 1 0
            0    | 0    | 0    | F_OF,      // 0 0 1 1
            0    | F_ZF | F_CF | 0   ,      // 0 1 0 0
            0    | F_ZF | F_CF | F_OF,      // 0 1 0 1
            0    | F_ZF | 0    | 0   ,      // 0 1 1 0
            0    | F_ZF | 0    | F_OF,      // 0 1 1 1
            F_SF | 0    | F_CF | 0   ,      // 1 0 0 0
            F_SF | 0    | F_CF | F_OF,      // 1 0 0 1
            F_SF | 0    | 0    | 0   ,      // 1 0 1 0
            F_SF | 0    | 0    | F_OF,      // 1 0 1 1
            F_SF | F_ZF | F_CF | 0   ,      // 1 1 0 0
            F_SF | F_ZF | F_CF | F_OF,      // 1 1 0 1
            F_SF | F_ZF | 0    | 0   ,      // 1 1 1 0
            F_SF | F_ZF | 0    | F_OF,      // 1 1 1 1

        };
#pragma clang diagnostic pop


        void aarch64_code_generator::emit_update_flags(op_size size, flags_op op, const tmp_holder& dst,
                                                       const tmp_holder& src1, const tmp_holder& src2) {
            /*
            uint32_t upd_mask = ~(
                    F_SF |
                    F_ZF |
                    (op == flags_op::inc || op == flags_op::dec ? 0 : F_CF) |
                    F_OF
                    // PF and AF are not implemented
            );

            if (op == flags_op::rol || op == flags_op::ror || op == flags_op::rcr)
                upd_mask |= ~(F_CF | F_OF);

            if (op == flags_op::bsf)
                upd_mask |= ~F_ZF;

            vixl::aarch64::Label skip_all;

            if (op == flags_op::shr || op == flags_op::shl || op == flags_op::sar || op == flags_op::rol || op == flags_op::ror
                || op == flags_op::shld || op == flags_op::shrd || op == flags_op::rcr) {
                masm.Cbz(src2.w(), &skip_all); // shift with zero amount does not really modify flags
            }

            // there is no aarch64 instruction that sets the flags for these operations. Need to do this manually
            if (op == flags_op::or_ || op == flags_op::xor_ || op == flags_op::shr || op == flags_op::shl
                || op == flags_op::sar || op == flags_op::shld || op == flags_op::shrd || op == flags_op::mul) {
                masm.Tst(dst.w(), dst.w());
            }
            if (op == flags_op::imul) {
                masm.Tst(dst.x(), dst.x());
            }

            auto upd_flags = get_tmp_reg();

            uint32_t sign_mask;
            uint64_t sign_mul_mask;
            int sign_pos;
            int bitsize;
            uint32_t val_mask;
            vixl::aarch64::Extend val_sext;
            switch (size) {
                case op_size::S_8:
                case op_size::U_8:
                sz_8:
                    sign_mask = 0x80;
                    val_mask = 0xff;
                    sign_pos = 7;
                    bitsize = 8;
                    val_sext = vixl::aarch64::Extend::SXTB;
                    break;
                case op_size::S_16:
                case op_size::U_16:
                sz_16:
                    sign_mask = 0x8000;
                    val_mask = 0xffff;
                    sign_pos = 15;
                    bitsize = 16;
                    val_sext = vixl::aarch64::Extend::SXTH;
                    break;
                case op_size::S_32:
                case op_size::U_32:
                sz_32:
                    sign_mask = 0x80000000;
                    val_mask = 0xffffffff;
                    sign_pos = 31;
                    bitsize = 32;
                    val_sext = vixl::aarch64::Extend::SXTW;
                    break;
                default:
                    throw std::exception();
            }

            if (op != flags_op::rol && op != flags_op::ror && op != flags_op::rcr && op != flags_op::bsf) {
                // set the general flags
                if (size == op_size::U_32) {
                    // full-size. nzcv can be used
                    auto nzcv = get_tmp_reg();
                    masm.Mrs(nzcv.x(), vixl::aarch64::SystemRegister::NZCV);

                    // PF and AF are not implemented

                    bool need_c_invert = op == flags_op::sub;

                    auto table_reg = get_tmp_reg();
                    if (need_c_invert)
                        masm.Ldr(table_reg.x(), reinterpret_cast<uint64_t>(inverted_c_flag_table));
                    else
                        masm.Ldr(table_reg.x(), reinterpret_cast<uint64_t>(flag_table));

                    masm.Asr(nzcv.x(), nzcv.x(), 28);
                    masm.Ldr(upd_flags.w(),
                             vixl::aarch64::MemOperand(table_reg.x(), nzcv.x(), vixl::aarch64::Shift::LSL, 2));

                    if (op == flags_op::inc || op == flags_op::dec)
                        // those don't update CF. Because Intel
                        masm.And(upd_flags.w(), upd_flags.w(), ~F_CF);
                } else {
                    // smaller size. Need to simulate most flags
                    // ZF
                    masm.Tst(dst.w(), val_mask);
                    masm.Csel(upd_flags.w(), F_ZF, 0, vixl::aarch64::Condition::eq);

                    // SF
                    masm.Tst(dst.w(), sign_mask);
                    auto tmp_flags = get_tmp_reg();
                    masm.Csel(tmp_flags.w(), F_SF, 0, vixl::aarch64::ne);
                    masm.Orr(upd_flags.w(), upd_flags.w(), tmp_flags.w());

                    if (op == flags_op::sub || op == flags_op::add || op == flags_op::inc || op == flags_op::dec) {
                        // detect overflow
                        auto of_tmp = get_tmp_reg();
                        if (op == flags_op::sub || op == flags_op::add) {
                            // sign was same (for add) or different (for sub)
                            masm.Eor(of_tmp.w(), src1.w(), src2.w());
                            vixl::aarch64::Label skip;
                            if (op == flags_op::add)
                                masm.Tbnz(of_tmp.w(), sign_pos, &skip);
                            else if (op == flags_op::sub)
                                masm.Tbz(of_tmp.w(), sign_pos, &skip);
                            else
                                throw std::exception();

                            // but has changed
                            masm.Eor(of_tmp.w(), src1.w(), dst.w());
                            masm.Tst(of_tmp.w(), sign_mask);

                            masm.Csel(tmp_flags.w(), F_OF, 0, vixl::aarch64::ne); // sign changed

                            masm.Bind(&skip);
                        } else if (op == flags_op::inc) {
                            vixl::aarch64::Label skip;
                            masm.Tbnz(src1.w(), sign_pos, &skip);
                            masm.Tbz(dst.w(), sign_pos, &skip);

                            masm.Mov(tmp_flags.w(), F_OF); // sign changed from positive to negative

                            masm.Bind(&skip);
                        } else if (op == flags_op::dec) {
                            vixl::aarch64::Label skip;
                            masm.Tbz(src1.w(), sign_pos, &skip);
                            masm.Tbnz(dst.w(), sign_pos, &skip);

                            masm.Mov(tmp_flags.w(), F_OF); // sign changed from negative to positive

                            masm.Bind(&skip);
                        } else
                            throw std::exception();
                        masm.Orr(upd_flags.w(), upd_flags.w(), tmp_flags.w());
                    }

                    if (op == flags_op::sub || op == flags_op::add) {
                        // detect carry/borrow
                        vixl::aarch64::Label skip;
                        masm.Tbz(dst.w(), sign_pos + 1, &skip);

                        masm.Mov(tmp_flags.w(), F_CF);

                        masm.Bind(&skip);
                        masm.Orr(upd_flags.w(), upd_flags.w(), tmp_flags.w());
                    }
                }
            } else {
                emit_load_imm(upd_flags, 0);
            }


            auto flags_reg = emit_load_reg_tmp(op_size::U_32, cpu_reg::EFLAGS);

            // handle instruction quirks
            if (op == flags_op::shr || op == flags_op::shl || op == flags_op::sar || op == flags_op::rol ||
                op == flags_op::ror || op == flags_op::shld || op == flags_op::shrd || op == flags_op::rcr) {
                // detect shifted-out bit and put it to CF
                if (op == flags_op::rol) {
                    masm.Tst(dst.w(), 1);
                } else if (op == flags_op::ror) {
                    masm.Tst(dst.w(), sign_mask);
                } else {
                    auto sh = emit_load_imm_tmp(1);
                    auto shamt = get_tmp_reg();
                    if (op == flags_op::shr || op == flags_op::sar || op == flags_op::shrd || op == flags_op::rcr)
                        masm.Sub(shamt.w(), src2.w(), 1);
                    else if (op == flags_op::shl || op == flags_op::shld) {
                        emit_load_imm(shamt, bitsize);
                        masm.Sub(shamt.w(), shamt.w(), src2.w());
                    } else
                        throw std::exception();
                    masm.Lsl(sh.w(), sh.w(), shamt.w());
                    masm.Tst(src1.w(), sh.w());
                }
                vixl::aarch64::Label skip;
                masm.B(vixl::aarch64::Condition::eq, &skip);

                masm.Orr(upd_flags.w(), upd_flags.w(), F_CF);

                masm.Bind(&skip);
            }

            // no sar here, as it resets the OF
            if (op == flags_op::shr || op == flags_op::shl || op == flags_op::shrd || op == flags_op::shld) {
                // set OF
                vixl::aarch64::Label skip;

                if (op == flags_op::shr || op == flags_op::shrd) {
                    auto tmp = get_tmp_reg();
                    // to match the implementation of halfix
                    masm.Eor(tmp.w(), dst.w(), vixl::aarch64::Operand(dst.w(), vixl::aarch64::Shift::LSL, 1));
                    masm.Tbz(tmp.w(), sign_pos, &skip);
                } else if (op == flags_op::shl || op == flags_op::shld) {
                    auto tmp = get_tmp_reg();
                    assert(sign_pos > F_CF_SHIFT);
                    masm.Lsl(tmp.w(), upd_flags.w(), sign_pos - F_CF_SHIFT);
                    masm.Eor(tmp.w(), tmp.w(), dst.w());

                    masm.Tbz(tmp.w(), sign_pos, &skip);
                } else
                    throw std::exception();

                masm.Orr(upd_flags.w(), upd_flags.w(), F_OF);

                masm.Bind(&skip);
            }
            if (op == flags_op::rol) {
                // set OF
                auto tmp = get_tmp_reg();
                masm.Eor(tmp.w(), dst.w(), vixl::aarch64::Operand(dst.w(), vixl::aarch64::Shift::LSL, sign_pos));

                vixl::aarch64::Label skip;
                masm.Tbz(tmp.w(), sign_pos, &skip);
                masm.Orr(upd_flags.w(), upd_flags.w(), F_OF);
                masm.Bind(&skip);
            }
            if (op == flags_op::ror || op == flags_op::rcr) {
                // set OF
                auto tmp = get_tmp_reg();
                masm.Eor(tmp.w(), dst.w(), vixl::aarch64::Operand(dst.w(), vixl::aarch64::Shift::LSL, 1));

                vixl::aarch64::Label skip;
                masm.Tbz(tmp.w(), sign_pos, &skip);
                masm.Orr(upd_flags.w(), upd_flags.w(), F_OF);
                masm.Bind(&skip);
            }

            if (op == flags_op::imul) {
                // detect overflow
                auto sext = get_tmp_reg();
                masm.And(sext.x(), dst.x(), val_mask); // simulate store

                // do the sign extension
                switch (size) {
                    case op_size::S_8:
                    case op_size::U_8:
                        masm.Sxtb(sext.x(), sext.x());
                        break;
                    case op_size::S_16:
                    case op_size::U_16:
                        masm.Sxth(sext.x(), sext.x());
                        break;
                    case op_size::S_32:
                    case op_size::U_32:
                        masm.Sxtw(sext.x(), sext.x());
                        break;
                    default:
                        throw std::exception();
                }

                masm.Cmp(sext.x(), dst.x()); // compare
                vixl::aarch64::Label skip;
                masm.B(vixl::aarch64::eq, &skip);

                masm.Orr(upd_flags.w(), upd_flags.w(), F_CF | F_OF);

                masm.Bind(&skip);
            }

            if (op == flags_op::mul) {
                masm.Tst(dst.x(), (uint64_t)val_mask << bitsize);
                auto tmp = get_tmp_reg();
                masm.Csel(tmp.w(), 0, F_CF | F_OF, vixl::aarch64::eq);
                masm.Orr(upd_flags.w(), upd_flags.w(), tmp.w());
            }

            if (op == flags_op::bsf) {
                masm.Tst(dst.w(), 0xffffff20);
                auto tmp_flags = get_tmp_reg();
                masm.Csel(tmp_flags.w(), 0, F_ZF, vixl::aarch64::eq);
                masm.Orr(upd_flags.w(), upd_flags.w(), tmp_flags.w());
            }

            masm.And(flags_reg.w(), flags_reg.w(), upd_mask);
            masm.Orr(flags_reg.w(), flags_reg.w(), upd_flags.w());
            emit_store_reg(op_size::U_32, flags_reg, cpu_reg::EFLAGS);

            masm.Bind(&skip_all);
             */

            uint32_t f_type = (static_cast<uint32_t>(op) & 0xff) |
                    ((static_cast<uint32_t>(op_size_change_sign<false>(size)) & 0xff) << 8);

            switch (op) {

                case flags_op::adc:
                case flags_op::sbb:

                case flags_op::sar:
                case flags_op::shr:
                case flags_op::shl:
                case flags_op::shld:
                case flags_op::shrd:
                    // need to set all the information (F_RES, F_TYPE, F_AUX1, F_AUX2)
                    // F_RES and FOPs are sign-extended

                    emit_sign_extend_word(op_size_change_sign<true>(size), dst, dst);
                    emit_sign_extend_word(op_size_change_sign<true>(size), src1, src1);
                    emit_sign_extend_word(op_size_change_sign<true>(size), src2, src2);

                    emit_store_reg(op_size::U_32, dst, cpu_reg::F_RES);
                    emit_store_reg(op_size::U_32, emit_load_imm_tmp(f_type), cpu_reg::F_TYPE);
                    emit_store_reg(op_size::U_32, src1, cpu_reg::F_AUX1);
                    emit_store_reg(op_size::U_32, src2, cpu_reg::F_AUX2);

                    if (op == flags_op::adc || op == flags_op::sbb)
                        current_f_type = f_type;
                    else
                        current_f_type = 0xffffffff; // we can't know for sure, as it depends on masked count
                    break;

                case flags_op::bsf:
                    // Only F_RES and F_TYPE is enough, but it is actually the source =)
                    emit_sign_extend_word(op_size_change_sign<true>(size), src1, src1);
                    emit_store_reg(op_size::U_32, src1, cpu_reg::F_RES);
                    emit_store_reg(op_size::U_32, emit_load_imm_tmp(f_type), cpu_reg::F_TYPE);
                    current_f_type = f_type;
                    break;
                case flags_op::or_:
                case flags_op::and_:
                case flags_op::xor_:
                    // Only F_RES and F_TYPE is enough
                    emit_sign_extend_word(op_size_change_sign<true>(size), dst, dst);
                    emit_store_reg(op_size::U_32, dst, cpu_reg::F_RES);
                    emit_store_reg(op_size::U_32, emit_load_imm_tmp(f_type), cpu_reg::F_TYPE);
                    current_f_type = f_type;
                    break;
                case flags_op::inc:
                case flags_op::dec:
                    // Need to set F_RES, F_TYPE and preserve the CF into F_AUX1
                {
                    auto cond = emit_test_condition<cpu_condition::c>();
                    auto tmp = get_tmp_reg();
                    masm.Cset(tmp.w(), cond);
                    emit_store_reg(op_size::U_32, tmp, cpu_reg::F_AUX1);
                }

                    emit_sign_extend_word(op_size_change_sign<true>(size), dst, dst);

                    emit_store_reg(op_size::U_32, dst, cpu_reg::F_RES);
                    emit_store_reg(op_size::U_32, emit_load_imm_tmp(f_type), cpu_reg::F_TYPE);

                    current_f_type = f_type;
                    break;
                case flags_op::rol:
                case flags_op::ror:
                case flags_op::rcr:
                {
                    // Need to set only the lo-word of F_TYPE, F_AUX1 and F_AUX2, but only if masked op2 is not zero
                    // (if masked op2 is zero emitted code will be skipped without our help =))
                    emit_sign_extend_word(op_size_change_sign<true>(size), src1, src1);
                    emit_sign_extend_word(op_size_change_sign<true>(size), src2, src2);

                    // store only the lo word of F_TYPE
                    emit_store_reg(op_size::U_16, emit_load_imm_tmp(f_type), cpu_reg::F_TYPE);
                    emit_store_reg(op_size::U_32, src1, cpu_reg::F_AUX1);
                    emit_store_reg(op_size::U_32, src2, cpu_reg::F_AUX2);

                    if (op == flags_op::rcr) {
                        // an ugly hack to implement OF
                        emit_sign_extend_word(op_size_change_sign<true>(size), dst, dst);
                        emit_store_reg(op_size::U_32, dst, cpu_reg::F_AUX3);
                    }

                    current_f_type = 0xffffffff; // we can't know for sure, as it depends on masked count

                }
                    break;
                case flags_op::add:
                case flags_op::sub:
                    // F_TYPE, F_RES and F_AUX1 are enough
                    emit_sign_extend_word(op_size_change_sign<true>(size), dst, dst);
                    emit_sign_extend_word(op_size_change_sign<true>(size), src1, src1);

                    emit_store_reg(op_size::U_32, dst, cpu_reg::F_RES);
                    emit_store_reg(op_size::U_32, emit_load_imm_tmp(f_type), cpu_reg::F_TYPE);
                    emit_store_reg(op_size::U_32, src1, cpu_reg::F_AUX1);

                    current_f_type = f_type;
                    break;
                case flags_op::imul:
                case flags_op::mul:
                    // F_TYPE, F_RES and F_AUX1 are enough, but F_AUX1 is high part of multiplication result
                {
                    auto tmp = get_tmp_reg();
                    emit_sign_extend_word(op_size_change_sign<true>(size), tmp, dst);

                    emit_store_reg(op_size::U_32, tmp, cpu_reg::F_RES);
                    emit_store_reg(op_size::U_32, emit_load_imm_tmp(f_type), cpu_reg::F_TYPE);

                    masm.Bfxil(tmp.x(), dst.x(), op_size_to_bits(size), op_size_to_bits(size));
                    emit_sign_extend_word(op_size_change_sign<true>(size), tmp, tmp);
                    emit_store_reg(op_size::U_32, tmp, cpu_reg::F_AUX1);

                    current_f_type = f_type;
                }
                    break;
                case flags_op::load_flags:
                {
                    current_f_type = f_type;
                    // F_TYPE will be written by the helper
                    //emit_store_reg(op_size::U_32, emit_load_imm_tmp(f_type), cpu_reg::F_TYPE);
                    emit_store_reg(op_size::U_32, src1, cpu_reg::EFLAGS);
                    register_saver_scope s(*this, false);
                    masm.CallRuntime(update_flags_helper);
                }
                    break;
                default:
                    throw std::exception();
            }

        }

        tmp_holder aarch64_code_generator::emit_load_memory_tmp(op_size sz, cpu_segment segment,
                                                                const tmp_holder &offset_register) {
            auto tmp = get_tmp_reg();
            emit_load_memory(sz, tmp, segment, offset_register);
            return tmp;
        }

        void aarch64_code_generator::emit_load_seg(const tmp_holder& dst, cpu_segment seg) {
            masm.Ldr(dst.x(), ref_cpu_seg(seg));

        }
        tmp_holder aarch64_code_generator::emit_load_seg_tmp(cpu_segment seg) {
            auto tmp = get_tmp_reg();
            emit_load_seg(tmp, seg);
            return tmp;
        }

        void aarch64_code_generator::emit_interrupt(int interrupt_number) {
            register_saver_scope saver(*this, false);

            masm.Mov(vixl::aarch64::x1, interrupt_number);

            masm.CallRuntime(interrupt_runtime_helper);

        }

        template<cpu_condition condition>
        vixl::aarch64::Condition aarch64_code_generator::emit_test_condition() {
            switch (condition) {
                case cpu_condition::z:
                case cpu_condition::nz: {
                    auto res = emit_load_reg_tmp(op_size::U_32, cpu_reg::F_RES);
                    masm.Tst(res.w(), res.w());
                    return condition == cpu_condition::z
                        ? vixl::aarch64::Condition::eq
                        : vixl::aarch64::Condition::ne;
                }
                    break;
                case cpu_condition::s:
                case cpu_condition::ns: {
                    auto res = emit_load_reg_tmp(op_size::U_32, cpu_reg::F_RES);
                    auto type = emit_load_reg_tmp(op_size::U_32, cpu_reg::F_TYPE);
                    masm.Eor(res.w(), res.w(), type.w()); // xor with SNEG
                    masm.Tst(res.w(), 0x80000000);
                    return condition == cpu_condition::s
                           ? vixl::aarch64::Condition::ne
                           : vixl::aarch64::Condition::eq;
                }
                    break;
                case cpu_condition::p:
                case cpu_condition::np:
                case cpu_condition::o:
                case cpu_condition::no:
                case cpu_condition::c:
                case cpu_condition::nc: {
                    register_saver_scope saver(*this, true);

                    if (current_f_type == 0xffffffff) {
                        // dynamic
                        masm.CallRuntime(cpu_context::test_flag_dynamic<condition>);
                    } else {
                        // static
                        masm.CallRuntime(cpu_context::get_static_flag_helper<condition>(current_f_type));
                    }
                    masm.Tst(vixl::aarch64::x0, vixl::aarch64::x0);
                    return vixl::aarch64::Condition::ne;
                }
                    break;

                case cpu_condition::be:
                case cpu_condition::nbe:
                {
                    auto nc_cond = emit_test_condition<cpu_condition::nc>();
                    masm.Ccmp(emit_load_reg_tmp(op_size::U_32, cpu_reg::F_RES).w(), 0, vixl::aarch64::StatusFlags::ZFlag, nc_cond);
                    if (condition == cpu_condition::be)
                        return vixl::aarch64::eq;
                    else
                        return vixl::aarch64::ne;
                }
                case cpu_condition::l:
                case cpu_condition::nl:
                {
                    auto of_cond = emit_test_condition<cpu_condition::o>();
                    auto tmp = emit_load_imm_tmp(0);
                    masm.Cinv(tmp.x(), tmp.x(), of_cond);
                    masm.Cinv(tmp.x(), tmp.x(), emit_test_condition<cpu_condition::s>());
                    masm.Tst(tmp.x(), 1);
                    if (condition == cpu_condition::l)
                        return vixl::aarch64::ne;
                    else
                        return vixl::aarch64::eq;
                }
                    break;
                case cpu_condition::nle:
                case cpu_condition::le:
                    {
                    auto nl_cond = emit_test_condition<cpu_condition::nl>();
                    masm.Ccmp(emit_load_reg_tmp(op_size::U_32, cpu_reg::F_RES).w(), 0,
                              vixl::aarch64::StatusFlags::ZFlag, nl_cond);
                    if (condition == cpu_condition::le)
                        return vixl::aarch64::eq;
                    else
                        return vixl::aarch64::ne;
                }
                    break;
            }
            throw std::exception();
        }


        template<cpu_condition cond>
        void aarch64_code_generator::emit_conditional_jump(const ZydisDecodedInstruction& instr) {
            auto branch_cond = emit_test_condition<cond>();

            auto current_eip = emit_load_imm_tmp(current_guest_eip);
            auto branch_eip = emit_load_operand_tmp(op_size::U_32, instr.operands[0]);
            hint_branch(current_guest_eip);
            hint_branch(get_imm_operand(op_size::U_32, instr.operands[0]));

            masm.Csel(current_eip.w(), branch_eip.w(), current_eip.w(), branch_cond);

            emit_store_reg(op_size::U_32, current_eip, cpu_reg::EIP);
        }

        template<cpu_condition cond>
        void aarch64_code_generator::emit_conditional_set(const ZydisDecodedInstruction& instr) {
            auto set_cond = emit_test_condition<cond>();
            auto res = get_tmp_reg();
            masm.Csel(res.w(), 1, 0, set_cond);

            emit_store_operand(res, instr.operands[0]);
        }


        void aarch64_code_generator::emit_sign_extend(op_size sz, const tmp_holder& dst, const tmp_holder& src) {
            switch (sz) {
                case op_size::U_8:
                    masm.Uxtb(dst.x(), src.x());
                    break;
                case op_size::U_16:
                    masm.Uxth(dst.x(), src.x());
                    break;
                case op_size::U_32:
                    masm.Uxtw(dst.x(), src.x());
                    break;
                case op_size::S_64:
                case op_size::U_64:
                    masm.Move(dst.x(), src.x());
                    break;
                case op_size::S_8:
                    masm.Sxtb(dst.x(), src.x());
                    break;
                case op_size::S_16:
                    masm.Sxth(dst.x(), src.x());
                    break;
                case op_size::S_32:
                    masm.Sxtw(dst.x(), src.x());
                    break;
                default:
                    throw std::exception();
            }
        }

        void aarch64_code_generator::emit_sign_extend_word(op_size sz, const tmp_holder& dst, const tmp_holder& src) {
            switch (sz) {
                case op_size::U_8:
                    masm.Uxtb(dst.w(), src.w());
                    break;
                case op_size::U_16:
                    masm.Uxth(dst.w(), src.w());
                    break;
                case op_size::S_32:
                case op_size::U_32:
                    masm.Move(dst.w(), src.w());
                    break;
                case op_size::S_8:
                    masm.Sxtb(dst.w(), src.w());
                    break;
                case op_size::S_16:
                    masm.Sxth(dst.w(), src.w());
                    break;
                default:
                    throw std::exception();
            }
        }


#undef F_SF
#undef F_ZF
#undef F_CF
#undef F_OF
    }
}
