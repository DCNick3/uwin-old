//
// Created by dcnick3 on 6/18/20.
//

#ifndef UWIN_CPU_CONTEXT_H
#define UWIN_CPU_CONTEXT_H


#include <cstdint>
#include "uwin-jit/types.h"
#include "libx87/fpu.h"

#include "uwin/target_abi.h"
#include "uwin/mem.h"

#include <exception>
#include <atomic>

namespace uwin {
    namespace jit {
        struct cpu_context;


        enum class cpu_reg {
            EAX = 0,
            EBX = 1,
            ECX = 2,
            EDX = 3,
            ESI = 4,
            EDI = 5,
            ESP = 6,
            EBP = 7,
            EIP = 8,
            EFLAGS = 9,

            // used to implement flags
            F_RES = 10,
            F_TYPE = 11,
            F_AUX1 = 12,
            F_AUX2 = 13,
            F_AUX3 = 14,
        };

        enum class cpu_flags {
            CF = 0x1,
            PF = 0x4,
            //AF = 0x10,
            ZF = 0x40,
            SF = 0x80,
            //TF = 0x100,
            //IF = 0x200,
            //DF = 0x400,
            OF = 0x800,
            //NT = 0x4000,
            //RF = 0x10000,
            //VM = 0x20000,
            AC = 0x40000,
            //VIF = 0x80000,
            //VIP = 0x100000,
            ID = 0x200000
        };

        static const uint32_t supported_flags_mask =
                static_cast<int>(cpu_flags::CF) |
                static_cast<int>(cpu_flags::ZF) |
                static_cast<int>(cpu_flags::SF) |
                static_cast<int>(cpu_flags::OF) |
                static_cast<int>(cpu_flags::AC) |
                static_cast<int>(cpu_flags::ID) |
                static_cast<int>(cpu_flags::PF);

        static const uint32_t static_flags_mask =
                static_cast<int>(cpu_flags::CF) |
                static_cast<int>(cpu_flags::ZF) |
                static_cast<int>(cpu_flags::SF) |
                static_cast<int>(cpu_flags::OF) |
                static_cast<int>(cpu_flags::PF);

        enum class cpu_condition {
            o,
            no,
            b,
            nb,
            z,
            nz,
            be,
            nbe,
            s,
            ns,
            p,
            np,
            l,
            nl,
            le,
            nle,

            c = b,
            nc = nb,
        };

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

        enum class flags_op {
            add,
            sub,
            adc,
            sbb,
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
            shld,
            shrd,
            rcr,
            bsf,
            load_flags,
        };

        class fpu_context : public libx87::fpu<fpu_context> {
            cpu_context& cpu_ctx;
        public:
            explicit inline fpu_context(cpu_context& cpu_ctx);

            inline void fp_exception() {
                throw std::exception();
            }

            inline void undefined_instruction() {
                throw std::exception();
            }
            inline bool get_cf();
            inline bool get_pf();
            inline bool get_zf();
            inline void set_eflags(uint32_t value);
            inline uint32_t get_eflags();
            template<cpu_flags flg>
            inline void set_flag(bool v);

            inline void set_cf(bool v) {
                set_flag<cpu_flags::CF>(v);
            }
            inline void set_pf(bool v) {
                if (v)
                    throw std::exception();
            }
            inline void set_zf(bool v) {
                set_flag<cpu_flags::ZF>(v);
            }
            inline void set_ax(uint16_t ax);
            inline uint32_t get_cs();
            inline uint32_t get_eip();
            inline bool is_code16() { return false; }
            inline void read32(uint32_t addr, uint32_t& value) { value = *g2hx<uint32_t>(addr); }
            inline void read16(uint32_t addr, uint16_t& value) { value = *g2hx<uint16_t>(addr); }
            inline void write32(uint32_t addr, uint32_t value) { *g2hx<uint32_t>(addr) = value; }
            inline void write16(uint32_t addr, uint16_t value) { *g2hx<uint16_t>(addr) = value; }
            inline void write8(uint32_t addr, uint8_t value) { *g2hx<uint8_t>(addr) = value; }
            inline uint32_t get_seg(uint32_t seg_type);
            inline bool is_protected() { return true; }
        };

        // assuming CS, DS, SS and ES bases are equal to guest_base
#define CPU_SEGMENTS \
            X(CS, 0, true) \
            X(DS, 1, true) \
            X(SS, 2, true) \
            X(ES, 3, true) \
            \
            X(FS, 4, false) \
            X(GS, 5, false) \

        enum class cpu_segment {
#define X(s, i, st) s = i,
            CPU_SEGMENTS
#undef X
        };

        /*inline static bool is_segment_static(cpu_segment seg) {
            switch (seg) {
#define X(s, i, st) case cpu_segment::s: return st;
                CPU_SEGMENTS
#undef X
            }
            throw std::exception();
        }*/

#undef CPU_SEGMENTS

        struct cpu_static_context {
            jptr guest_base = 0;
            //jptr host_static_segment_bases[6];
        };

        struct cpu_context {
            uint32_t regs[16] = { 0 };

            jptr host_dynamic_segment_bases[6] = { 0 };

            volatile std::atomic_bool force_yield;

            cpu_static_context static_context;

            // pointer used here to allow to execute offsetof on the structure
            fpu_context* fpu_ctx;

            inline jptr& host_segment_base(cpu_segment segment) {
                //if (is_segment_static(segment))
                //    return static_context.host_static_segment_bases[static_cast<int>(segment)];
                return host_dynamic_segment_bases[static_cast<int>(segment)];
            }

            template<cpu_reg R>
            inline uint32_t& REG() { return regs[static_cast<int>(R)]; }

            inline uint32_t& EAX() { return REG<cpu_reg::EAX>(); }
            inline uint32_t& EBX() { return REG<cpu_reg::EBX>(); }
            inline uint32_t& ECX() { return REG<cpu_reg::ECX>(); }
            inline uint32_t& EDX() { return REG<cpu_reg::EDX>(); }
            inline uint32_t& ESI() { return REG<cpu_reg::ESI>(); }
            inline uint32_t& EDI() { return REG<cpu_reg::EDI>(); }
            inline uint32_t& EBP() { return REG<cpu_reg::EBP>(); }
            inline uint32_t& ESP() { return REG<cpu_reg::ESP>(); }
            inline uint32_t& EIP() { return REG<cpu_reg::EIP>(); }
            inline uint32_t& EFLAGS() { return REG<cpu_reg::EFLAGS>(); }

            inline uint32_t& F_RES() { return REG<cpu_reg::F_RES>(); }
            // Stores last flags operation in the following format:
            // SP--------------ZZZZZZZZTTTTTTTT
            // S - sign flag negate
            // P - parity flag negate
            // Z - last op size (op_size)
            // T - last op type (flags_op)
            // As such, F_TYPE can be split into two words.
            // When one does not want to update PZS they should keep the upper word of F_TYPE and F_RES the same
            inline uint32_t& F_TYPE() { return REG<cpu_reg::F_TYPE>(); }
            inline uint32_t& F_AUX1() { return REG<cpu_reg::F_AUX1>(); }
            inline uint32_t& F_AUX2() { return REG<cpu_reg::F_AUX2>(); }
            inline uint32_t& F_AUX3() { return REG<cpu_reg::F_AUX3>(); }

            inline flags_op F_TYPE_OP() { return static_cast<flags_op>(F_TYPE() & 0xff); }
            inline op_size F_TYPE_SZ() { return static_cast<op_size>((F_TYPE() >> 8) & 0xff); }
            inline bool F_TYPE_SNEG() { return (F_TYPE() & 0x80000000) != 0; }
            inline bool F_TYPE_PNEG() { return (F_TYPE() & 0x40000000) != 0; }

            /*
            inline jptr& CS() { return host_segment_base(cpu_segment::CS); }
            inline jptr& DS() { return host_segment_base(cpu_segment::DS); }
            inline jptr& ES() { return host_segment_base(cpu_segment::ES); }
            inline jptr& SS() { return host_segment_base(cpu_segment::SS); }
            */

            inline jptr& FS() { return host_segment_base(cpu_segment::FS); }
            inline jptr& GS() { return host_segment_base(cpu_segment::GS); }

            inline cpu_context()
                : fpu_ctx(new fpu_context(*this))
            {
                EFLAGS() = static_cast<int>(cpu_flags::ID); // we support cpuid instruction
            }

            inline ~cpu_context() { delete fpu_ctx; }

            cpu_context(const cpu_context&) = delete;
            void operator=(const cpu_context&) = delete;


            inline bool get_pf() {
                uint8_t v = F_RES();
                auto pf = (1 + (v&1) + ((v>>1)&1) + ((v>>2)&1) + ((v>>3)&1) + ((v>>4)&1) + ((v>>5)&1) + ((v>>6)&1) + ((v>>7)&1)) % 2;
                return pf != F_TYPE_PNEG();
            }

            template<op_size sz, flags_op op, cpu_condition cond>
            static inline bool test_flag_static(cpu_context *ctx)
            {
                // only ZSPCO is supported
                if (cond == cpu_condition::z)
                    return ctx->F_RES() == 0;
                if (cond == cpu_condition::nz)
                    return ctx->F_RES() != 0;
                if (cond == cpu_condition::s)
                    // xor with SNEG
                    return ((ctx->F_RES() ^ ctx->F_TYPE()) & 0x80000000) != 0;
                if (cond == cpu_condition::ns)
                    // xor with SNEG
                    return ((ctx->F_RES() ^ ctx->F_TYPE()) & 0x80000000) == 0;
                if (cond == cpu_condition::p || cond == cpu_condition::np) {
                    if (cond == cpu_condition::p)
                        return ctx->get_pf();
                    else
                        return !ctx->get_pf();
                }
                if (op == flags_op::or_ || op == flags_op::and_ || op == flags_op::xor_) {
                    if (cond == cpu_condition::c || cond == cpu_condition::o)
                        return false; // both CF and OF are zero
                    else
                        return true; // nc or no
                }

                int sz_bits;
                uint32_t sz_mask;
                uint32_t sz_signed_max;
                uint32_t sz_signed_min;
                uint32_t sz_sign_mask;
                switch (sz) {
                    case op_size::U_8:
                        sz_bits = 8;
                        sz_mask = 0xff;
                        sz_signed_max = 0x7f;
                        sz_signed_min = sz_sign_mask = 0x80;
                        break;
                    case op_size::U_16:
                        sz_bits = 16;
                        sz_mask = 0xffff;
                        sz_signed_max = 0x7fff;
                        sz_signed_min = sz_sign_mask = 0x8000;
                        break;
                    case op_size::U_32:
                        sz_bits = 32;
                        sz_mask = 0xffffffff;
                        sz_signed_max = 0x7fffffff;
                        sz_signed_min = sz_sign_mask = 0x80000000;
                        break;
                    default:
                        throw std::exception();
                }

                if (op == flags_op::add || op == flags_op::sub || op == flags_op::adc || op == flags_op::sbb) {

                    uint32_t vr = ctx->F_RES() & sz_mask;
                    uint32_t v1 = ctx->F_AUX1() & sz_mask;
                    uint32_t v2;
                    if (op == flags_op::adc || op == flags_op::sbb) {
                        v2 = ctx->F_AUX2() & sz_mask;
                    } else if (op == flags_op::add) {
                        v2 = (vr - v1) & sz_mask;
                    } else {
                        v2 = (v1 - vr) & sz_mask;
                    }

                    auto s1 = (v1 & sz_sign_mask) != 0;
                    auto s2 = (v2 & sz_sign_mask) != 0;
                    auto sr = (vr & sz_sign_mask) != 0;

                    if (cond == cpu_condition::o || cond == cpu_condition::no) {
                        bool of;
                        if (op == flags_op::add)
                            of = (s1 == s2) && (s1 != sr);
                        else
                            of = (s1 != s2) && (s1 != sr);
                        if (cond == cpu_condition::o)
                            return of;
                        else
                            return !of;
                    }

                    if (cond == cpu_condition::c || cond == cpu_condition::nc) {
                        bool cf;
                        if (op == flags_op::add)
                            cf = vr < v1;
                        else if (op == flags_op::sub)
                            cf = v2 > v1;
                        else if (op == flags_op::adc) {
                            cf = (s1 != s2 && s2 != sr) != s1; // a bit of magic to recreate the carry out having summator inputs and output
                        } else {
                            // sbb
                            cf = (sr != s2 && s1 != s2) != sr;
                        }
                        if (cond == cpu_condition::c)
                            return cf;
                        else
                            return !cf;
                    }
                }

                if (op == flags_op::inc || op == flags_op::dec) {
                    if (cond == cpu_condition::c)
                        return ctx->F_AUX1();
                    if (cond == cpu_condition::nc)
                        return !ctx->F_AUX1();
                    if (cond == cpu_condition::o || cond == cpu_condition::no) {
                        auto vr = ctx->F_RES() & sz_mask;
                        bool of;
                        if (op == flags_op::inc)
                            of = vr == sz_signed_max + 1;
                        else
                            of = vr == sz_signed_min - 1;
                        if (cond == cpu_condition::o)
                            return of;
                        else
                            return !of;
                    }
                }

                if ((cond == cpu_condition::c || cond == cpu_condition::nc) &&
                    (op == flags_op::sar || op == flags_op::shr || op == flags_op::ror || op == flags_op::rcr || op == flags_op::shrd)) {
                    uint32_t v1 = ctx->F_AUX1() & sz_mask;
                    uint32_t v2 = ctx->F_AUX2() & 0x1f;

                    if (op == flags_op::sar) {
                        v1 = ctx->F_AUX1(); // want a sign extension
                    }

                    if (op == flags_op::ror) {
                        v2 = (v2 - 1) % sz_bits + 1;
                    }

                    bool cf = (v1 >> (v2 - 1)) & 1;
                    if (cond == cpu_condition::c)
                        return cf;
                    else
                        return !cf;
                }

                if ((cond == cpu_condition::c || cond == cpu_condition::nc) &&
                    (op == flags_op::shl || op == flags_op::rol || op == flags_op::shld)) {
                    auto v1 = ctx->F_AUX1() & sz_mask;
                    auto v2 = ctx->F_AUX2() & sz_mask;

                    if (op == flags_op::rol) {
                        v2 = (v2 - 1) % sz_bits + 1;
                    }

                    bool cf = (v1 >> (sz_bits - v2)) & 1;
                    if (cond == cpu_condition::c)
                        return cf;
                    else
                        return !cf;
                }

                if (op == flags_op::sar) {
                    if (cond == cpu_condition::o)
                        return false;
                    if (cond == cpu_condition::no)
                        return true;
                }

                if (op == flags_op::shr && (cond == cpu_condition::o || cond == cpu_condition::no)) {
                    bool of = (ctx->F_AUX1() & sz_sign_mask) != 0;
                    if (cond == cpu_condition::o)
                        return of;
                    else
                        return !of;
                }

                if ((op == flags_op::shl || op == flags_op::rol) && (cond == cpu_condition::o || cond == cpu_condition::no)) {
                    auto v1 = ctx->F_AUX1();
                    bool of = ((v1 ^ (v1 << 1)) & sz_sign_mask) != 0; // two most significant bits are not the same
                    if (cond == cpu_condition::o)
                        return of;
                    else
                        return !of;
                }

                if (op == flags_op::mul || op == flags_op::imul) {
                    auto vr = ctx->F_RES();
                    auto v1 = ctx->F_AUX1();

                    bool cf;
                    if (op == flags_op::imul) {
                        auto sr = (vr & 0x80000000) != 0;
                        cf = sr ? v1 != 0xffffffff : v1 != 0;
                    } else
                        cf = v1 != 0;
                    if (cond == cpu_condition::c || cond == cpu_condition::o)
                        return cf;
                    else if (cond == cpu_condition::nc || cond == cpu_condition::no)
                        return !cf;
                }

                if (op == flags_op::ror && (cond == cpu_condition::o || cond == cpu_condition::no)) {
                    auto v1 = ctx->F_AUX1();

                    bool of = ((v1 >> (sz_bits - 1)) ^ v1) & 1;

                    if (cond == cpu_condition::o)
                        return of;
                    else
                        return !of;
                }

                if (op == flags_op::rcr && (cond == cpu_condition::o || cond == cpu_condition::no)) {
                    auto vr = ctx->F_AUX3();

                    bool of = ((vr ^ (vr << 1)) & sz_sign_mask) != 0;

                    if (cond == cpu_condition::o)
                        return of;
                    else
                        return !of;
                }

                if ((op == flags_op::shrd || op == flags_op::shld) && (cond == cpu_condition::o || cond == cpu_condition::no)) {
                    auto v1 = ctx->F_AUX1();
                    auto vr = ctx->F_RES();

                    bool of = ((v1 ^ vr) & sz_sign_mask) != 0; // detect sign change

                    if (cond == cpu_condition::o)
                        return of;
                    else
                        return !of;
                }

                if (op == flags_op::bsf && cond != cpu_condition::z && cond != cpu_condition::nz)
                    return false;

                if (op == flags_op::load_flags) {
                    bool cf = (ctx->EFLAGS() & static_cast<int>(cpu_flags::CF)) != 0;
                    bool of = (ctx->EFLAGS() & static_cast<int>(cpu_flags::OF)) != 0;
                    if (cond == cpu_condition::c)
                        return cf;
                    if (cond == cpu_condition::nc)
                        return !cf;
                    if (cond == cpu_condition::o)
                        return of;
                    if (cond == cpu_condition::no)
                        return !of;
                }

                throw std::exception();
            }

            typedef bool (*flag_helper)(cpu_context *ctx);

            template<cpu_condition cond>
            static inline flag_helper get_static_flag_helper(uint32_t type) {
                auto op = static_cast<flags_op>(type & 0xff);
                auto sz = static_cast<op_size>((type >> 8) & 0xff);

                // I don't know (besides this ugly thing) how to avoid code duplication and still have ability to call template-generated runtime helpers
                if (sz == op_size::U_8) {
                    switch (op) {
                        case flags_op::add:
                            return test_flag_static<op_size::U_8, flags_op::add, cond>;
                        case flags_op::sub:
                            return test_flag_static<op_size::U_8, flags_op::sub, cond>;
                        case flags_op::adc:
                            return test_flag_static<op_size::U_8, flags_op::adc, cond>;
                        case flags_op::sbb:
                            return test_flag_static<op_size::U_8, flags_op::sbb, cond>;
                        case flags_op::or_:
                            return test_flag_static<op_size::U_8, flags_op::or_, cond>;
                        case flags_op::and_:
                            return test_flag_static<op_size::U_8, flags_op::and_, cond>;
                        case flags_op::xor_:
                            return test_flag_static<op_size::U_8, flags_op::xor_, cond>;
                        case flags_op::inc:
                            return test_flag_static<op_size::U_8, flags_op::inc, cond>;
                        case flags_op::dec:
                            return test_flag_static<op_size::U_8, flags_op::dec, cond>;
                        case flags_op::sar:
                            return test_flag_static<op_size::U_8, flags_op::sar, cond>;
                        case flags_op::shr:
                            return test_flag_static<op_size::U_8, flags_op::shr, cond>;
                        case flags_op::shl:
                            return test_flag_static<op_size::U_8, flags_op::shl, cond>;
                        case flags_op::imul:
                            return test_flag_static<op_size::U_8, flags_op::imul, cond>;
                        case flags_op::mul:
                            return test_flag_static<op_size::U_8, flags_op::mul, cond>;
                        case flags_op::rol:
                            return test_flag_static<op_size::U_8, flags_op::rol, cond>;
                        case flags_op::ror:
                            return test_flag_static<op_size::U_8, flags_op::ror, cond>;
                        case flags_op::shld:
                            return test_flag_static<op_size::U_8, flags_op::shld, cond>;
                        case flags_op::shrd:
                            return test_flag_static<op_size::U_8, flags_op::shrd, cond>;
                        case flags_op::rcr:
                            return test_flag_static<op_size::U_8, flags_op::rcr, cond>;
                        case flags_op::bsf:
                            return test_flag_static<op_size::U_8, flags_op::bsf, cond>;
                        case flags_op::load_flags:
                            return test_flag_static<op_size::U_8, flags_op::load_flags, cond>;
                    }
                } else if (sz == op_size::U_16) {
                    switch (op) {
                        case flags_op::add:
                            return test_flag_static<op_size::U_16, flags_op::add, cond>;
                        case flags_op::sub:
                            return test_flag_static<op_size::U_16, flags_op::sub, cond>;
                        case flags_op::adc:
                            return test_flag_static<op_size::U_16, flags_op::adc, cond>;
                        case flags_op::sbb:
                            return test_flag_static<op_size::U_16, flags_op::sbb, cond>;
                        case flags_op::or_:
                            return test_flag_static<op_size::U_16, flags_op::or_, cond>;
                        case flags_op::and_:
                            return test_flag_static<op_size::U_16, flags_op::and_, cond>;
                        case flags_op::xor_:
                            return test_flag_static<op_size::U_16, flags_op::xor_, cond>;
                        case flags_op::inc:
                            return test_flag_static<op_size::U_16, flags_op::inc, cond>;
                        case flags_op::dec:
                            return test_flag_static<op_size::U_16, flags_op::dec, cond>;
                        case flags_op::sar:
                            return test_flag_static<op_size::U_16, flags_op::sar, cond>;
                        case flags_op::shr:
                            return test_flag_static<op_size::U_16, flags_op::shr, cond>;
                        case flags_op::shl:
                            return test_flag_static<op_size::U_16, flags_op::shl, cond>;
                        case flags_op::imul:
                            return test_flag_static<op_size::U_16, flags_op::imul, cond>;
                        case flags_op::mul:
                            return test_flag_static<op_size::U_16, flags_op::mul, cond>;
                        case flags_op::rol:
                            return test_flag_static<op_size::U_16, flags_op::rol, cond>;
                        case flags_op::ror:
                            return test_flag_static<op_size::U_16, flags_op::ror, cond>;
                        case flags_op::shld:
                            return test_flag_static<op_size::U_16, flags_op::shld, cond>;
                        case flags_op::shrd:
                            return test_flag_static<op_size::U_16, flags_op::shrd, cond>;
                        case flags_op::rcr:
                            return test_flag_static<op_size::U_16, flags_op::rcr, cond>;
                        case flags_op::bsf:
                            return test_flag_static<op_size::U_16, flags_op::bsf, cond>;
                        case flags_op::load_flags:
                            return test_flag_static<op_size::U_16, flags_op::load_flags, cond>;
                    }
                } else if (sz == op_size::U_32) {
                    switch (op) {
                        case flags_op::add:
                            return test_flag_static<op_size::U_32, flags_op::add, cond>;
                        case flags_op::sub:
                            return test_flag_static<op_size::U_32, flags_op::sub, cond>;
                        case flags_op::adc:
                            return test_flag_static<op_size::U_32, flags_op::adc, cond>;
                        case flags_op::sbb:
                            return test_flag_static<op_size::U_32, flags_op::sbb, cond>;
                        case flags_op::or_:
                            return test_flag_static<op_size::U_32, flags_op::or_, cond>;
                        case flags_op::and_:
                            return test_flag_static<op_size::U_32, flags_op::and_, cond>;
                        case flags_op::xor_:
                            return test_flag_static<op_size::U_32, flags_op::xor_, cond>;
                        case flags_op::inc:
                            return test_flag_static<op_size::U_32, flags_op::inc, cond>;
                        case flags_op::dec:
                            return test_flag_static<op_size::U_32, flags_op::dec, cond>;
                        case flags_op::sar:
                            return test_flag_static<op_size::U_32, flags_op::sar, cond>;
                        case flags_op::shr:
                            return test_flag_static<op_size::U_32, flags_op::shr, cond>;
                        case flags_op::shl:
                            return test_flag_static<op_size::U_32, flags_op::shl, cond>;
                        case flags_op::imul:
                            return test_flag_static<op_size::U_32, flags_op::imul, cond>;
                        case flags_op::mul:
                            return test_flag_static<op_size::U_32, flags_op::mul, cond>;
                        case flags_op::rol:
                            return test_flag_static<op_size::U_32, flags_op::rol, cond>;
                        case flags_op::ror:
                            return test_flag_static<op_size::U_32, flags_op::ror, cond>;
                        case flags_op::shld:
                            return test_flag_static<op_size::U_32, flags_op::shld, cond>;
                        case flags_op::shrd:
                            return test_flag_static<op_size::U_32, flags_op::shrd, cond>;
                        case flags_op::rcr:
                            return test_flag_static<op_size::U_32, flags_op::rcr, cond>;
                        case flags_op::bsf:
                            return test_flag_static<op_size::U_32, flags_op::bsf, cond>;
                        case flags_op::load_flags:
                            return test_flag_static<op_size::U_32, flags_op::load_flags, cond>;
                    }
                }

                throw std::exception();
            }

            template<cpu_condition cond>
            static inline bool test_flag_dynamic(cpu_context *ctx)
            {
                if (cond == cpu_condition::z)
                    return ctx->F_RES() == 0;
                if (cond == cpu_condition::nz)
                    return ctx->F_RES() != 0;
                if (cond == cpu_condition::s)
                    // xor with SNEG
                    return ((ctx->F_RES() ^ ctx->F_TYPE()) & 0x80000000) != 0;
                if (cond == cpu_condition::ns)
                    // xor with SNEG
                    return ((ctx->F_RES() ^ ctx->F_TYPE()) & 0x80000000) == 0;
                if (cond == cpu_condition::p)
                    return ctx->get_pf();
                if (cond == cpu_condition::np)
                    return !ctx->get_pf();

                return get_static_flag_helper<cond>(ctx->F_TYPE())(ctx);
            }

            template<cpu_condition cond>
            inline bool test_flag_dynamic()
            {
                return test_flag_dynamic<cond>(this);
            }

            inline uint32_t compute_eflags()
            {
                uint32_t res = EFLAGS() & (~static_flags_mask);
                res |= test_flag_dynamic<cpu_condition::c>() ? static_cast<int>(cpu_flags::CF) : 0;
                res |= test_flag_dynamic<cpu_condition::p>() ? static_cast<int>(cpu_flags::PF) : 0;
                res |= test_flag_dynamic<cpu_condition::z>() ? static_cast<int>(cpu_flags::ZF) : 0;
                res |= test_flag_dynamic<cpu_condition::s>() ? static_cast<int>(cpu_flags::SF) : 0;
                res |= test_flag_dynamic<cpu_condition::o>() ? static_cast<int>(cpu_flags::OF) : 0;
                return res;
            }
        };

        static_assert(sizeof(bool) == 1, "bool size in not one byte");

        fpu_context::fpu_context(cpu_context &cpu_ctx)
            : cpu_ctx(cpu_ctx) {}

        bool fpu_context::get_cf() {
            return cpu_ctx.EFLAGS() & static_cast<int>(cpu_flags::CF);
        }

        bool fpu_context::get_pf() {
            return false;
        }

        bool fpu_context::get_zf() {
            return cpu_ctx.EFLAGS() & static_cast<int>(cpu_flags::ZF);
        }

        void fpu_context::set_eflags(uint32_t value) {
            cpu_ctx.EFLAGS() = value;
        }

        uint32_t fpu_context::get_eflags() {
            return cpu_ctx.EFLAGS();
        }

        template<cpu_flags flg>
        void fpu_context::set_flag(bool v) {
            if (v)
                cpu_ctx.EFLAGS() |= static_cast<int>(flg);
            else
                cpu_ctx.EFLAGS() &= static_cast<int>(flg);
        }

        void fpu_context::set_ax(uint16_t ax) {
            cpu_ctx.EAX() = cpu_ctx.EAX() & 0xffff0000 | ax;
        }

        uint32_t fpu_context::get_cs() {
            return __USER_CS;
        }

        uint32_t fpu_context::get_eip() {
            return cpu_ctx.EIP();
        }

        uint32_t fpu_context::get_seg(uint32_t seg_type) {
            auto seg = static_cast<cpu_segment>(seg_type);
            if (seg == cpu_segment::CS)
                return __USER_CS;
            if (seg == cpu_segment::DS || seg == cpu_segment::ES || seg == cpu_segment::SS)
                return __USER_DS;
            if (seg == cpu_segment::FS)
                return __USER_FS;
            if (seg == cpu_segment::GS)
                return __USER_GS;
            throw std::exception();
        }

    }
}


#endif //UWIN_CPU_CONTEXT_H
