//
// Created by dcnick3 on 6/24/20.
//

#ifndef UWIN_SLJIT_H
#define UWIN_SLJIT_H

#include <utility>
#include <exception>
#include <sstream>
#include <iomanip>
#include <uwin/util/log.h>

namespace uwin {
    namespace jit {
        namespace sljit {
#include "sljit/sljitLir.h"
        }

        static_assert(SLJIT_UNALIGNED == 1, "sljit does not allow unaligned access");

        enum class sljit_reg : sljit::sljit_s32 {
            R0 = SLJIT_R0,
            R1 = SLJIT_R1,
            R2 = SLJIT_R2,
            R3 = SLJIT_R3,
            R4 = SLJIT_R4,
            R5 = SLJIT_R5,
            R6 = SLJIT_R6,
            R7 = SLJIT_R7,
            R8 = SLJIT_R8,
            R9 = SLJIT_R9,

            S0 = SLJIT_S0,
            S1 = SLJIT_S1,
            S2 = SLJIT_S2,
            S3 = SLJIT_S3,
            S4 = SLJIT_S4,
            S5 = SLJIT_S5,
            S6 = SLJIT_S6,
            S7 = SLJIT_S7,
            S8 = SLJIT_S8,
            S9 = SLJIT_S9,
        };

        enum class sljit_flagtype {
            EQUAL = SLJIT_EQUAL,
            EQUAL32 = SLJIT_EQUAL32,
            ZERO = SLJIT_ZERO,
            ZERO32 = SLJIT_ZERO32,

            NOT_ZERO = SLJIT_NOT_ZERO,
            NOT_ZERO32 = SLJIT_NOT_ZERO32,

            LESS = SLJIT_LESS,
            LESS32 = SLJIT_LESS32,
            // TODO: define other types
        };

        class sljit_ref {
            sljit::sljit_s32 base;
            sljit::sljit_sw param;
        public:
            inline sljit_ref(sljit::sljit_s32 base, sljit::sljit_sw param) : base(base), param(param) {
            }

            static inline sljit_ref unused() {
                return sljit_ref(SLJIT_UNUSED, 0);
            }
            static inline sljit_ref reg(sljit_reg reg) {
                return sljit_ref(static_cast<sljit::sljit_s32>(reg), 0);
            }
            static inline sljit_ref imm(sljit::sljit_sw val) {
                return sljit_ref(SLJIT_IMM, val);
            }
            static inline sljit_ref mem0(void* addr) {
                return mem0(reinterpret_cast<intptr_t>(addr));
            }
            static inline sljit_ref mem0(sljit::sljit_sw addr) {
                return sljit_ref(SLJIT_MEM0(), addr);
            }
            static inline sljit_ref mem1(sljit_reg reg, sljit::sljit_sw offset) {
                return sljit_ref(SLJIT_MEM1(static_cast<sljit::sljit_s32>(reg)), offset);
            }
            static inline sljit_ref mem2(sljit_reg base_reg, sljit_reg index_reg, sljit::sljit_sw shift) {
                return sljit_ref(SLJIT_MEM2(static_cast<sljit::sljit_s32>(base_reg),
                                            static_cast<sljit::sljit_s32>(index_reg)), shift);
            }

            inline bool is_unused() { return base == SLJIT_UNUSED; }
            inline bool is_reg() {
                return base >= SLJIT_R0 && base <= SLJIT_NUMBER_OF_REGISTERS;
            }
            inline bool is_imm() { return base == SLJIT_IMM; }
            inline bool is_mem0() { return base == SLJIT_MEM; }
            inline bool is_mem1() { return (base & SLJIT_MEM) && ((base & 0xFF) == base); }
            inline bool is_mem2() { return (base & SLJIT_MEM) && ((base & 0xFF) != base); }

            inline sljit::sljit_s32 get_base() {
                return base;
            }
            inline sljit::sljit_sw get_param() {
                return param;
            }
        };

        template<typename F>
        class sljit_code {
            void* code;
            size_t len;
        public:
            sljit_code(const sljit_code& other) = delete;
            void operator=(const sljit_code& other) = delete;
            inline sljit_code(sljit_code&& other) {
                code = other.code;
                other.code = nullptr;
            }

            inline sljit_code(void *code, size_t len) : code(code), len(len) {};
            inline ~sljit_code() {
                if (code != nullptr)
                    sljit::sljit_free_code(code);
            }

            inline F get_pointer() {
                return reinterpret_cast<F>(code);
            }

            inline size_t get_len() {
                return len;
            }

            inline void dump() {
                std::stringstream ss;

                ss << std::hex;

                uint8_t *data = (uint8_t*)get_pointer();

                for (int i(0); i < get_len(); ++i)
                    ss << std::setw(2) << std::setfill('0') << (int)data[i];

                uw_log("sljit dump: %s\n", ss.str().c_str());
            }
        };

        class sljit_compiler {
            struct sljit::sljit_compiler* compiler;

            inline void check_result(sljit::sljit_s32 result) {
                if (result != SLJIT_SUCCESS)
                    throw std::exception();
            }

        public:
            sljit_compiler(const sljit_compiler& other) = delete;
            void operator=(const sljit_compiler& other) = delete;

            inline sljit_compiler() : compiler(sljit::sljit_create_compiler(nullptr)) {
                compiler->verbose = stderr;
            }
            inline ~sljit_compiler() {
                sljit::sljit_free_compiler(compiler);
            };

            inline sljit::sljit_s32 get_error() {
                return sljit::sljit_get_compiler_error(compiler);
            }

            template<typename F>
            inline sljit_code<F> generate_code() {
                void* ptr = sljit::sljit_generate_code(compiler);
                size_t len = sljit::sljit_get_generated_code_size(compiler);
                return sljit_code<F>(ptr, len);
            }

            inline void emit_enter(sljit::sljit_s32 options, sljit::sljit_s32 arg_types,
                    sljit::sljit_s32 scratches, sljit::sljit_s32 saveds,
                    sljit::sljit_s32 fscratches, sljit::sljit_s32 fsaveds,
                    sljit::sljit_s32 local_size) {
                check_result(sljit::sljit_emit_enter(compiler, options, arg_types, scratches, saveds, fscratches, fsaveds, local_size));
            }

            inline void emit_return() {
                check_result(sljit::sljit_emit_return(compiler, SLJIT_UNUSED, 0, 0));
            }

            inline void emit_return(sljit::sljit_s32 op, sljit_ref src) {
                check_result(sljit::sljit_emit_return(compiler, op, src.get_base(), src.get_param()));
            }

            inline void emit_op0(sljit::sljit_s32 op) {
                check_result(sljit::sljit_emit_op0(compiler, op));
            }

            inline void emit_op_src(sljit::sljit_s32 op, sljit_ref src) {
                check_result(sljit::sljit_emit_op_src(compiler, op,
                                  src.get_base(), src.get_param()));
            }

            inline void emit_op1(sljit::sljit_s32 op, sljit_ref dst, sljit_ref src) {
                check_result(sljit::sljit_emit_op1(compiler, op,
                        dst.get_base(), dst.get_param(),
                        src.get_base(), src.get_param()));
            }

            inline void emit_op2(sljit::sljit_s32 op, sljit_ref dst, sljit_ref src1, sljit_ref src2) {
                check_result(sljit::sljit_emit_op2(compiler, op,
                        dst.get_base(), dst.get_param(),
                        src1.get_base(), src1.get_param(),
                        src2.get_base(), src2.get_param()));
            }

            /*inline void emit_cmp(sljit_flagtype type, sljit_ref src1, sljit_ref src2) {
                check_result(sljit::sljit_emit_cmp(compiler, static_cast<sljit::sljit_s32>(type),
                               src1.get_base(), src1.get_param(),
                               src2.get_base(), src2.get_param()));
            }*/

            inline void emit_cmov(sljit_flagtype type, sljit_reg dst_reg, sljit_ref src) {
                check_result(sljit::sljit_emit_cmov(compiler,
                        static_cast<sljit::sljit_s32>(type),
                        static_cast<sljit::sljit_s32>(dst_reg), src.get_base(), src.get_param()));
            }
        };
    }
}


#endif //UWIN_SLJIT_H
