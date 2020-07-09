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
        };

        enum class cpu_flags {
            CF = 0x1,
            //PF = 0x4,
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
                static_cast<int>(cpu_flags::ID);

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
            nle
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

            cpu_static_context static_context;

            // pointer used here to allow to execute offsetof on the structure
            fpu_context* fpu_ctx;

            inline jptr& host_segment_base(cpu_segment segment) {
                //if (is_segment_static(segment))
                //    return static_context.host_static_segment_bases[static_cast<int>(segment)];
                return host_dynamic_segment_bases[static_cast<int>(segment)];
            }

            inline uint32_t& EAX() { return regs[static_cast<int>(cpu_reg::EAX)]; }
            inline uint32_t& EBX() { return regs[static_cast<int>(cpu_reg::EBX)]; }
            inline uint32_t& ECX() { return regs[static_cast<int>(cpu_reg::ECX)]; }
            inline uint32_t& EDX() { return regs[static_cast<int>(cpu_reg::EDX)]; }
            inline uint32_t& ESI() { return regs[static_cast<int>(cpu_reg::ESI)]; }
            inline uint32_t& EDI() { return regs[static_cast<int>(cpu_reg::EDI)]; }
            inline uint32_t& EBP() { return regs[static_cast<int>(cpu_reg::EBP)]; }
            inline uint32_t& ESP() { return regs[static_cast<int>(cpu_reg::ESP)]; }
            inline uint32_t& EIP() { return regs[static_cast<int>(cpu_reg::EIP)]; }
            inline uint32_t& EFLAGS() { return regs[static_cast<int>(cpu_reg::EFLAGS)]; }

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
        };

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
