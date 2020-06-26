//
// Created by dcnick3 on 6/18/20.
//

#ifndef UWIN_CPU_CONTEXT_H
#define UWIN_CPU_CONTEXT_H

#include <cstdint>

#include "uwin-jit/sljit.h"

namespace uwin {
    namespace jit {
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
            PF = 0x4,
            //AF = 0x10,
            ZF = 0x40,
            SF = 0x80,
            //TF = 0x100,
            //IF = 0x200,
            DF = 0x400,
            OF = 0x800,
            //NT = 0x4000,
            //RF = 0x10000,
            //VM = 0x20000,
            //AC = 0x40000,
            //VIF = 0x80000,
            //VIP = 0x100000,
            //ID = 0x200000
        };

        static_assert(sizeof(sljit::sljit_uw) >= sizeof(uint32_t), "size of sljit_uw is too low");

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

        inline static bool is_segment_static(cpu_segment seg) {
            switch (seg) {
#define X(s, i, st) case cpu_segment::s: return st;
                CPU_SEGMENTS
#undef X
            }
            throw std::exception();
        }

#undef CPU_SEGMENTS

        struct cpu_static_context {
            sljit::sljit_p guest_base;
            sljit::sljit_p host_static_segment_bases[6];
        };

        struct cpu_context {
            sljit::sljit_u32 regs[16];

            sljit::sljit_p host_dynamic_segment_bases[6];

            cpu_static_context static_context;

            inline sljit::sljit_p& host_segment_base(cpu_segment segment) {
                if (is_segment_static(segment))
                    return static_context.host_static_segment_bases[static_cast<int>(segment)];
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

            inline sljit::sljit_p& CS() { return host_segment_base(cpu_segment::CS); }
            inline sljit::sljit_p& DS() { return host_segment_base(cpu_segment::DS); }
            inline sljit::sljit_p& ES() { return host_segment_base(cpu_segment::ES); }
            inline sljit::sljit_p& SS() { return host_segment_base(cpu_segment::SS); }
            inline sljit::sljit_p& FS() { return host_segment_base(cpu_segment::FS); }
            inline sljit::sljit_p& GS() { return host_segment_base(cpu_segment::GS); }
        };
    }
}


#endif //UWIN_CPU_CONTEXT_H
