//
// Created by dcnick3 on 6/27/20.
//

#include "uwin-jit/cpu_context.h"
#include "uwin-jit/executable_memory_allocator.h"
#include "aarch64/macro-assembler-aarch64.h"
#include "Zydis/Zydis.h"

#include "uwin/util/log.h"

#include <unordered_set>
#include <string>
#include <sstream>
#include <iomanip>

namespace uwin {
    namespace jit {
        class aarch64_code_generator;

        enum class op_size {
            U_8,
            U_16,
            U_32,
            U_64,
            S_8,
            S_16,
            S_32,
            S_64,
        };

        enum class arith_op {
            add,
            sub,
            or_,
            and_,
            xor_,
            inc,
            dec,
            sar,
            shr,
            shl,
            imul,
            mul,
            rol,
            ror,
        };

        class aarch64_code_generator {
            const cpu_static_context& ctx;
            vixl::aarch64::MacroAssembler masm;
            vixl::aarch64::Label start_label;

            uint32_t current_guest_eip;

            static const uint32_t normal_register_mask = 0x100;
            static const uint32_t float_register_mask = 0x200;
            static const uint32_t register_kind_mask = 0x300;
            static const uint32_t register_code_mask = 0xff;

            static const int32_t stack_frame_size = 48;

            std::unordered_set<uint32_t> normal_available_tmp_regs;
            std::unordered_set<uint32_t> float_available_tmp_regs;
            std::unordered_set<uint32_t> used_tmp_regs;

        public:
            class tmp_holder {
                aarch64_code_generator& gen;
                int32_t reg;
            public:
                inline tmp_holder(aarch64_code_generator& gen, int32_t reg)
                        : gen(gen), reg(reg) {}

                tmp_holder(const tmp_holder& other) = delete;
                tmp_holder& operator=(const tmp_holder& other) = delete;

                inline tmp_holder(tmp_holder&& other) :
                        reg(other.reg), gen(other.gen) {
                    other.reg = -1;
                }

                inline const vixl::aarch64::WRegister w() const {
                    assert((reg & register_kind_mask) == normal_register_mask);
                    return std::move(vixl::aarch64::WRegister(reg & register_code_mask));
                }

                inline const vixl::aarch64::XRegister x() const {
                    assert((reg & register_kind_mask) == normal_register_mask);
                    return std::move(vixl::aarch64::XRegister(reg & register_code_mask));
                }

                inline const vixl::aarch64::VRegister h() const {
                    assert((reg & register_kind_mask) == float_register_mask);
                    return vixl::aarch64::VRegister(reg & register_code_mask,
                                                    vixl::aarch64::VectorFormat::kFormatH);
                }

                inline const vixl::aarch64::VRegister s() const {
                    assert((reg & register_kind_mask) == float_register_mask);
                    return vixl::aarch64::VRegister(reg & register_code_mask,
                                                    vixl::aarch64::VectorFormat::kFormatS);
                }

                inline const vixl::aarch64::VRegister d() const {
                    assert((reg & register_kind_mask) == float_register_mask);
                    return vixl::aarch64::VRegister(reg & register_code_mask,
                                                    vixl::aarch64::VectorFormat::kFormatD);
                }

                bool is_float() const {
                    return (reg & register_kind_mask) == float_register_mask;
                }

                bool is_normal() const {
                    return (reg & register_kind_mask) == normal_register_mask;
                }

                inline ~tmp_holder() {
                    if (reg != -1)
                        gen.return_tmp_reg(reg);
                }
            };

            class register_saver_scope {
                aarch64_code_generator& gen;
                std::vector<vixl::aarch64::CPURegister> save_list;

                register_saver_scope(const register_saver_scope&) = delete;
                register_saver_scope& operator=(const register_saver_scope&) = delete;

            public:
                inline register_saver_scope(aarch64_code_generator& gen)
                        : gen(gen), save_list() {
                    save_list.emplace_back(gen.guest_base_reg());
                    for (auto v : gen.used_tmp_regs) {
                        if ((v & register_kind_mask) == normal_register_mask)
                            save_list.emplace_back(vixl::aarch64::XRegister(v & register_code_mask));
                        else if ((v & register_kind_mask) == float_register_mask)
                            save_list.emplace_back(vixl::aarch64::VRegister(v & register_code_mask, vixl::aarch64::VectorFormat::kFormatD));
                        else
                            throw std::exception();
                    }
                    assert(save_list.size() <= (stack_frame_size - 16) / 8);
                    for (int i = 0; i < save_list.size(); i++)
                        gen.masm.Str(save_list[i],
                                vixl::aarch64::MemOperand(vixl::aarch64::sp, 16 + i * 8));
                }

                inline ~register_saver_scope() {
                    for (int i = 0; i < save_list.size(); i++)
                        gen.masm.Ldr(save_list[i],
                                vixl::aarch64::MemOperand(vixl::aarch64::sp, 16 + i * 8));
                }
            };
        private:

            inline tmp_holder get_tmp_reg() {
                if (normal_available_tmp_regs.empty())
                    throw std::exception();
                auto r = *normal_available_tmp_regs.begin();
                normal_available_tmp_regs.erase(normal_available_tmp_regs.begin());
                used_tmp_regs.insert(r);
                return tmp_holder(*this, r);
            }

            inline tmp_holder get_tmp_flt_reg() {
                if (float_available_tmp_regs.empty())
                    throw std::exception();
                auto r = *float_available_tmp_regs.begin();
                float_available_tmp_regs.erase(float_available_tmp_regs.begin());
                used_tmp_regs.insert(r);
                return tmp_holder(*this, r);
            }

            inline vixl::aarch64::MemOperand ref_cpu_reg(cpu_reg reg) {
                return vixl::aarch64::MemOperand(vixl::aarch64::x0,
                        offsetof(cpu_context, regs) + 4 * static_cast<int>(reg));
            }

            inline vixl::aarch64::MemOperand ref_cpu_seg(cpu_segment seg) {
                return vixl::aarch64::MemOperand(vixl::aarch64::x0,
                                                 offsetof(cpu_context, host_dynamic_segment_bases) + sizeof(jptr) * static_cast<int>(seg));
            }

            void emit_interrupt(int interrupt_number);

            void emit_update_flags(op_size size, arith_op op, const tmp_holder& dst,
                                   const tmp_holder& src1, const tmp_holder& src2);
            vixl::aarch64::Condition emit_test_flags(cpu_condition condition);

            void emit_load_sized(op_size sz, const tmp_holder& dst, vixl::aarch64::MemOperand mem_op);
            tmp_holder emit_load_sized_tmp(op_size sz, vixl::aarch64::MemOperand mem_op);
            void emit_store_sized(op_size sz, const tmp_holder& src, vixl::aarch64::MemOperand mem_op);

            void emit_load_imm(const tmp_holder&, uint64_t val);
            tmp_holder emit_load_imm_tmp(uint64_t val);

            void emit_load_reg(op_size sz, const tmp_holder& dst, cpu_reg reg);
            tmp_holder emit_load_reg_tmp(op_size sz, cpu_reg reg);
            void emit_store_reg(op_size sz, const tmp_holder& src, cpu_reg reg);

            void emit_load_seg(const tmp_holder& dst, cpu_segment seg);
            tmp_holder emit_load_seg_tmp(cpu_segment seg);

            void emit_load_operand(op_size sz, const tmp_holder& dst, const ZydisDecodedOperand& operand);
            tmp_holder emit_load_operand_tmp(op_size sz, const ZydisDecodedOperand& operand);
            void emit_store_operand(const tmp_holder& src, const ZydisDecodedOperand& operand);

            void emit_load_sized(op_size sz, const tmp_holder &dst, const tmp_holder& addr);

            void emit_load_memory(op_size sz, const tmp_holder& dst, cpu_segment segment, const tmp_holder& offset_register);
            void emit_load_memory(op_size sz, const tmp_holder& dst, cpu_segment segment, tmp_holder&& offset_register);
            tmp_holder emit_load_memory_tmp(op_size sz, cpu_segment segment, const tmp_holder& offset_register);
            void emit_store_memory(op_size sz, const tmp_holder& src, cpu_segment segment, const tmp_holder& offset_register);

            tmp_holder emit_load_effective_address_tmp(const ZydisDecodedOperand& mem_operand);

            const vixl::aarch64::XRegister& cpu_ctx_reg() {
                return vixl::aarch64::x0;
            }

            const vixl::aarch64::XRegister& guest_base_reg() {
                return vixl::aarch64::x1;
            }

            inline void dump_generated_code() {
                auto begin = masm.GetBuffer()->GetStartAddress<uint8_t*>();
                auto end = masm.GetBuffer()->GetEndAddress<uint8_t*>();
                size_t sz = end - begin;

                std::stringstream ss;
                ss << std::hex;

                for (int i(0); i < sz; ++i)
                    ss << std::setw(2) << std::setfill('0') << (int)begin[i];

                uw_log("generated code dump: %s\n", ss.str().c_str());
            }

        public:
            inline aarch64_code_generator(const cpu_static_context& ctx)
                : ctx(ctx), masm(), start_label() {

                // caller saved registers
                for (int i = 9; i < 16; i++)
                    normal_available_tmp_regs.insert(normal_register_mask | i);

                // caller saved registers
                for (int i = 16; i < 32; i++)
                    float_available_tmp_regs.insert(float_register_mask | i);
            }

            void emit_prologue();
            void emit_epilogue();
            bool emit_instruction(uint32_t guest_eip, const ZydisDecodedInstruction& instr, bool force_terminate_block = false);

            xmem_piece commit();

            inline void return_tmp_reg(int reg) {
                if ((reg & register_kind_mask) == normal_register_mask) {
                    normal_available_tmp_regs.insert(reg);
                } else if ((reg & register_kind_mask) == float_register_mask) {
                    float_available_tmp_regs.insert(reg);
                } else
                    throw std::exception();
                used_tmp_regs.erase(reg);
            }
        };
    }
}
