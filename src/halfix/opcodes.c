#include "halfix/cpu/opcodes.h"
#include "halfix/cpu/cpu.h"
#include "halfix/cpu/fpu.h"
#include "halfix/cpu/instrument.h"
#include "halfix/cpu/ops.h"
#include "halfix/cpu/simd.h"
#include "halfix/cpuapi.h"
#include "halfix/state.h"

#define EXCEPTION_HANDLER EXCEP()

#ifdef INSTRUMENT
#define INSTRUMENT_INSN() cpu_instrument_execute()
#else
#define INSTRUMENT_INSN() NOP()
#endif

#define UNUSED2(x) x

#define NEXT(flags)                 \
    do {                            \
        thread_cpu.phys_eip += flags & 15; \
        INSTRUMENT_INSN();          \
        return i + 1;               \
    } while (1)
#define NEXT2(flags)           \
    do {                       \
        thread_cpu.phys_eip += flags; \
        INSTRUMENT_INSN();     \
        return i + 1;          \
    } while (1)
// Stops the trace and moves onto next one
#define STOP()                  \
    do {                        \
        INSTRUMENT_INSN();      \
        return cpu_get_trace(); \
    } while (0)
#define EXCEP()                 \
    do {                        \
        thread_cpu.cycles_to_run++;    \
        return cpu_get_trace(); \
    } while (0)
#define STOP2() return i
#define R8(i) thread_cpu.reg8[i]
#define R16(i) thread_cpu.reg16[i]
#define R32(i) thread_cpu.reg32[i]

#define push16(a)        \
    if (cpu_push16((a))) \
        EXCEPTION_HANDLER;
#define push32(a)        \
    if (cpu_push32((a))) \
        EXCEPTION_HANDLER;
#define pop16(a)        \
    if (cpu_pop16((a))) \
        EXCEPTION_HANDLER;
#define pop32(a)        \
    if (cpu_pop32((a))) \
        EXCEPTION_HANDLER;

// Bunch of macros for repeated operations
#define arith_re(sz, func)                                          \
    uint32_t flags = i->flags, linaddr = cpu_get_linaddr(flags, i); \
    uint##sz##_t res;                                               \
    cpu_read##sz(linaddr, res, thread_cpu.tlb_shift_read);                 \
    func(I_OP(flags), &R##sz(I_REG(flags)), res);                   \
    NEXT(flags)
#define arith_rmw(sz, func, ...)                                                   \
    uint32_t flags = i->flags,                                                     \
             linaddr = cpu_get_linaddr(flags, i),                                  \
             tlb_shift = thread_cpu.tlb_tags[linaddr >> 12],                              \
             shift = thread_cpu.tlb_shift_write;                                          \
    uint##sz##_t* ptr;                                                             \
    if (TLB_ENTRY_INVALID##sz(linaddr, tlb_shift, shift)) {                        \
        if (cpu_access_read##sz(linaddr, tlb_shift >> shift, shift))               \
            EXCEP();                                                               \
        func(I_OP(flags), (void*)&thread_cpu.read_result, ##__VA_ARGS__);                 \
        cpu_access_write##sz(linaddr, thread_cpu.read_result, tlb_shift >> shift, shift); \
    } else {                                                                       \
        ptr = thread_cpu.tlb[linaddr >> 12] + linaddr;                                    \
        func(I_OP(flags), ptr, ##__VA_ARGS__);                                     \
    }                                                                              \
    NEXT(flags)
#define arith_rmw2(sz, func, ...)                                                  \
    uint32_t flags = i->flags,                                                     \
             linaddr = cpu_get_linaddr(flags, i),                                  \
             tlb_shift = thread_cpu.tlb_tags[linaddr >> 12],                              \
             shift = thread_cpu.tlb_shift_write;                                          \
    uint##sz##_t* ptr;                                                             \
    if (TLB_ENTRY_INVALID##sz(linaddr, tlb_shift, shift)) {                        \
        if (cpu_access_read##sz(linaddr, tlb_shift >> shift, shift))               \
            EXCEP();                                                               \
        func((void*)&thread_cpu.read_result, ##__VA_ARGS__);                              \
        cpu_access_write##sz(linaddr, thread_cpu.read_result, tlb_shift >> shift, shift); \
    } else {                                                                       \
        ptr = thread_cpu.tlb[linaddr >> 12] + linaddr;                                    \
        func(ptr, ##__VA_ARGS__);                                                  \
    }                                                                              \
    NEXT(flags)
#define arith_rmw3(sz, func, offset, ...)                                          \
    uint32_t flags = i->flags,                                                     \
             linaddr = cpu_get_linaddr(flags, i) + offset,                         \
             tlb_shift = thread_cpu.tlb_tags[linaddr >> 12],                              \
             shift = thread_cpu.tlb_shift_write;                                          \
    uint##sz##_t* ptr;                                                             \
    if (TLB_ENTRY_INVALID##sz(linaddr, tlb_shift, shift)) {                        \
        if (cpu_access_read##sz(linaddr, tlb_shift >> shift, shift))               \
            EXCEP();                                                               \
        func((void*)&thread_cpu.read_result, ##__VA_ARGS__);                              \
        cpu_access_write##sz(linaddr, thread_cpu.read_result, tlb_shift >> shift, shift); \
    } else {                                                                       \
        ptr = thread_cpu.tlb[linaddr >> 12] + linaddr;                                    \
        func(ptr, ##__VA_ARGS__);                                                  \
    }                                                                              \
    NEXT(flags)
#define jcc16(cond)                                                  \
    int flags = i->flags;                                            \
    uint32_t virt = VIRT_EIP();                                      \
    if (cond) {                                                      \
        thread_cpu.phys_eip += ((virt + flags + i->imm32) & 0xFFFF) - virt; \
        STOP();                                                      \
    } else                                                           \
        NEXT2(flags);
#define jcc32(cond)                       \
    int flags = i->flags;                 \
    if (cond) {                           \
        thread_cpu.phys_eip += flags + i->imm32; \
        STOP();                           \
    } else                                \
        NEXT2(flags);
static void interrupt_guard(void)
{
    // Update halfix.cycles to have the right value
    thread_cpu.cycles += cpu_get_cycles() - thread_cpu.cycles;
    if (thread_cpu.cycles_to_run != 1) {
        thread_cpu.refill_counter = thread_cpu.cycles_to_run - 2;
        thread_cpu.cycles_to_run = 2;
        thread_cpu.cycle_offset = 2;
    } else {
        thread_cpu.cycles_to_run = 1;
        thread_cpu.cycle_offset = 1;
        thread_cpu.refill_counter = 0;
        thread_cpu.interrupts_blocked = 1;
    }
}

static __thread union {
    uint8_t d8;
    uint16_t d16;
    uint32_t d32;
} temp;

#define FAST_BRANCHLESS_MASK(addr, i) (addr & ((i << 12 & 65536) - 1))
static inline uint32_t cpu_get_linaddr(uint32_t i, struct decoded_instruction* j)
{
    uint32_t addr = thread_cpu.reg32[I_BASE(i)];
    addr += thread_cpu.reg32[I_INDEX(i)] << (I_SCALE(i));
    addr += j->disp32;
    return FAST_BRANCHLESS_MASK(addr, i) + thread_cpu.seg_base[I_SEG_BASE(i)];
}
static inline uint32_t cpu_get_virtaddr(uint32_t i, struct decoded_instruction* j)
{
    uint32_t addr = thread_cpu.reg32[I_BASE(i)];
    addr += thread_cpu.reg32[I_INDEX(i)] << (I_SCALE(i));
    addr += j->disp32;
    return FAST_BRANCHLESS_MASK(addr, i);
}

void cpu_execute(void)
{
    struct decoded_instruction* i = cpu_get_trace();
    do {
        i = i->handler(i);
        if (!--thread_cpu.cycles_to_run)
            break;
    } while (1);
}

OPTYPE op_fatal_error(struct decoded_instruction* i)
{
    CPU_LOG("The CPU decoder encountered an internal fatal error while translating code\n");
    exit(0);
    UNUSED(i);
}
OPTYPE op_ud_exception(struct decoded_instruction* i)
{
    UNUSED(i);
    EXCEPTION_UD();
}
OPTYPE op_trace_end(struct decoded_instruction* i)
{
    UNUSED(i);
    //halfix.cycles_to_run++;
    EXCEP(); // Don't call instrumentation callbacks since there's no instruction being executed here.
}

OPTYPE op_nop(struct decoded_instruction* i)
{
    UNUSED(i);
    NEXT2(i->flags);
}

OPTYPE op_jmp_r16(struct decoded_instruction* i)
{
    uint32_t dest = R16(I_RM(i->flags));
    if(dest >= thread_cpu.seg_limit[CS]) EXCEPTION_GP(0);
    SET_VIRT_EIP(dest);
    STOP();
}
OPTYPE op_jmp_r32(struct decoded_instruction* i)
{
    uint32_t dest = R32(I_RM(i->flags));
    if(dest >= thread_cpu.seg_limit[CS]) EXCEPTION_GP(0);
    SET_VIRT_EIP(dest);
    STOP();
}
OPTYPE op_jmp_e16(struct decoded_instruction* i)
{
    uint32_t flags = i->flags, linaddr = cpu_get_linaddr(flags, i);
    uint16_t src;
    cpu_read16(linaddr, src, thread_cpu.tlb_shift_read);
    SET_VIRT_EIP(src);
    STOP();
}
OPTYPE op_jmp_e32(struct decoded_instruction* i)
{
    uint32_t flags = i->flags, linaddr = cpu_get_linaddr(flags, i), src;
    cpu_read32(linaddr, src, thread_cpu.tlb_shift_read);
    SET_VIRT_EIP(src);
    STOP();
}
OPTYPE op_call_r16(struct decoded_instruction* i)
{
    uint32_t flags = i->flags;
    push16(I_LENGTH(flags) + VIRT_EIP());
    SET_VIRT_EIP(R16(I_RM(flags)));
    STOP();
}
OPTYPE op_call_r32(struct decoded_instruction* i)
{
    uint32_t flags = i->flags;
    push32(I_LENGTH(flags) + VIRT_EIP());
    SET_VIRT_EIP(R32(I_RM(flags)));
    STOP();
}
OPTYPE op_call_e16(struct decoded_instruction* i)
{
    uint32_t flags = i->flags, linaddr = cpu_get_linaddr(flags, i);
    uint16_t src;
    cpu_read16(linaddr, src, thread_cpu.tlb_shift_read);
    push16(VIRT_EIP() + I_LENGTH(flags));
    SET_VIRT_EIP(src);
    STOP();
}
OPTYPE op_call_e32(struct decoded_instruction* i)
{
    uint32_t flags = i->flags, linaddr = cpu_get_linaddr(flags, i), src;
    cpu_read32(linaddr, src, thread_cpu.tlb_shift_read);
    push32(VIRT_EIP() + I_LENGTH(flags));
    SET_VIRT_EIP(src);
    STOP();
}
OPTYPE op_jmp_rel32(struct decoded_instruction* i)
{
    thread_cpu.phys_eip += i->flags + i->imm32;
    STOP();
}
OPTYPE op_jmp_rel16(struct decoded_instruction* i)
{
    uint32_t virt = VIRT_EIP();
    thread_cpu.phys_eip += ((virt + i->flags + i->imm32) & 0xFFFF) - virt;
    STOP();
}
OPTYPE op_jmpf(struct decoded_instruction* i)
{
    if (jmpf(i->imm32, i->disp16, VIRT_EIP() + I_LENGTH2(i->flags)))
        EXCEP();
    STOP();
}
OPTYPE op_jmpf_e16(struct decoded_instruction* i)
{
    uint32_t flags = i->flags, linaddr = cpu_get_linaddr(flags, i), eip = 0, cs = 0;
    cpu_read16(linaddr, eip, thread_cpu.tlb_shift_read);
    cpu_read16(linaddr + 2, cs, thread_cpu.tlb_shift_read);
    if (jmpf(eip, cs, VIRT_EIP() + I_LENGTH(flags)))
        EXCEP();
    STOP();
}
OPTYPE op_jmpf_e32(struct decoded_instruction* i)
{
    uint32_t flags = i->flags, linaddr = cpu_get_linaddr(flags, i), eip, cs;
    cpu_read32(linaddr, eip, thread_cpu.tlb_shift_read);
    cpu_read16(linaddr + 4, cs, thread_cpu.tlb_shift_read);
    if (jmpf(eip, cs, VIRT_EIP() + I_LENGTH(flags)))
        EXCEP();
    STOP();
}
OPTYPE op_callf16_ap(struct decoded_instruction* i)
{
    uint32_t eip = i->imm32, cs = i->disp16,
             eip_after = VIRT_EIP() + I_LENGTH2(i->flags);
    if (callf(eip, cs, eip_after, 0))
        EXCEP();
    STOP();
}
OPTYPE op_callf32_ap(struct decoded_instruction* i)
{
    uint32_t eip = i->imm32, cs = i->disp32,
             eip_after = VIRT_EIP() + I_LENGTH2(i->flags);
    if (callf(eip, cs, eip_after, 1))
        EXCEP();
    STOP();
}
OPTYPE op_callf_e16(struct decoded_instruction* i)
{
    uint32_t flags = i->flags, linaddr = cpu_get_linaddr(flags, i), eip = 0, cs = 0;
    cpu_read16(linaddr, eip, thread_cpu.tlb_shift_read);
    cpu_read16(linaddr + 2, cs, thread_cpu.tlb_shift_read);
    if (callf(eip, cs, VIRT_EIP() + I_LENGTH(flags), 0))
        EXCEP();
    STOP();
}
OPTYPE op_callf_e32(struct decoded_instruction* i)
{
    uint32_t flags = i->flags, linaddr = cpu_get_linaddr(flags, i), eip, cs;
    cpu_read32(linaddr, eip, thread_cpu.tlb_shift_read);
    cpu_read16(linaddr + 4, cs, thread_cpu.tlb_shift_read);
    if (callf(eip, cs, VIRT_EIP() + I_LENGTH(flags), 1))
        EXCEP();
    STOP();
}
OPTYPE op_retf16(struct decoded_instruction* i)
{
    if (retf(i->imm16, 0))
        EXCEP();
    STOP();
}
OPTYPE op_retf32(struct decoded_instruction* i)
{
    if (retf(i->imm16, 1))
        EXCEP();
    STOP();
}
OPTYPE op_iret16(struct decoded_instruction* i)
{
    if (iret(VIRT_EIP() + i->flags, 0))
        EXCEP();
    STOP();
}
OPTYPE op_iret32(struct decoded_instruction* i)
{
    if (iret(VIRT_EIP() + i->flags, 1))
        EXCEP();
    STOP();
}

OPTYPE op_loop_rel16(struct decoded_instruction* i)
{
    uint32_t mask = i->disp32, cond = (thread_cpu.reg32[ECX] - 1) & mask, virt = VIRT_EIP();
    thread_cpu.reg32[ECX] = cond | (thread_cpu.reg32[ECX] & ~mask);
    if (cond) {
        thread_cpu.phys_eip += ((virt + i->flags + i->imm32) & 0xFFFF) - virt;
        STOP();
    }
    NEXT2(i->flags);
}
OPTYPE op_loop_rel32(struct decoded_instruction* i)
{
    uint32_t mask = i->disp32, cond = (thread_cpu.reg32[ECX] - 1) & mask;
    thread_cpu.reg32[ECX] = cond | (thread_cpu.reg32[ECX] & ~mask);
    if (cond) {
        thread_cpu.phys_eip += i->flags + i->imm32;
        STOP();
    }
    NEXT2(i->flags);
}
OPTYPE op_loopz_rel16(struct decoded_instruction* i)
{
    uint32_t mask = i->disp32, cond = (thread_cpu.reg32[ECX] - 1) & mask, virt = VIRT_EIP();
    thread_cpu.reg32[ECX] = cond | (thread_cpu.reg32[ECX] & ~mask);
    if (cond && cpu_get_zf()) {
        thread_cpu.phys_eip += ((virt + i->flags + i->imm32) & 0xFFFF) - virt;
        STOP();
    }
    NEXT2(i->flags);
}
OPTYPE op_loopz_rel32(struct decoded_instruction* i)
{
    uint32_t mask = i->disp32, cond = (thread_cpu.reg32[ECX] - 1) & mask;
    thread_cpu.reg32[ECX] = cond | (thread_cpu.reg32[ECX] & ~mask);
    if (cond && cpu_get_zf()) {
        thread_cpu.phys_eip += i->flags + i->imm32;
        STOP();
    }
    NEXT2(i->flags);
}
OPTYPE op_loopnz_rel16(struct decoded_instruction* i)
{
    uint32_t mask = i->disp32, cond = (thread_cpu.reg32[ECX] - 1) & mask, virt = VIRT_EIP();
    thread_cpu.reg32[ECX] = cond | (thread_cpu.reg32[ECX] & ~mask);
    if (cond && !cpu_get_zf()) {
        thread_cpu.phys_eip += ((virt + i->flags + i->imm32) & 0xFFFF) - virt;
        STOP();
    }
    NEXT2(i->flags);
}
OPTYPE op_loopnz_rel32(struct decoded_instruction* i)
{
    uint32_t mask = i->disp32, cond = (thread_cpu.reg32[ECX] - 1) & mask;
    thread_cpu.reg32[ECX] = cond | (thread_cpu.reg32[ECX] & ~mask);
    if (cond && !cpu_get_zf()) {
        thread_cpu.phys_eip += i->flags + i->imm32;
        STOP();
    }
    NEXT2(i->flags);
}
OPTYPE op_jecxz_rel16(struct decoded_instruction* i)
{
    uint32_t virt = VIRT_EIP();
    if (!(thread_cpu.reg32[ECX] & i->disp32)) {
        thread_cpu.phys_eip += ((virt + i->flags + i->imm32) & 0xFFFF) - virt;
        STOP();
    }
    NEXT2(i->flags);
}
OPTYPE op_jecxz_rel32(struct decoded_instruction* i)
{
    if (!(thread_cpu.reg32[ECX] & i->disp32)) {
        thread_cpu.phys_eip += i->flags + i->imm32;
        STOP();
    }
    NEXT2(i->flags);
}

// <<< BEGIN AUTOGENERATE "jcc" >>>
// Auto-generated on Mon Sep 16 2019 23:27:23 GMT-0700 (PDT)
OPTYPE op_jo16(struct decoded_instruction* i)
{
    jcc16(cpu_get_of());
}
OPTYPE op_jo32(struct decoded_instruction* i)
{
    jcc32(cpu_get_of());
}
OPTYPE op_jno16(struct decoded_instruction* i)
{
    jcc16(!cpu_get_of());
}
OPTYPE op_jno32(struct decoded_instruction* i)
{
    jcc32(!cpu_get_of());
}
OPTYPE op_jb16(struct decoded_instruction* i)
{
    jcc16(cpu_get_cf());
}
OPTYPE op_jb32(struct decoded_instruction* i)
{
    jcc32(cpu_get_cf());
}
OPTYPE op_jnb16(struct decoded_instruction* i)
{
    jcc16(!cpu_get_cf());
}
OPTYPE op_jnb32(struct decoded_instruction* i)
{
    jcc32(!cpu_get_cf());
}
OPTYPE op_jz16(struct decoded_instruction* i)
{
    jcc16(cpu_get_zf());
}
OPTYPE op_jz32(struct decoded_instruction* i)
{
    jcc32(cpu_get_zf());
}
OPTYPE op_jnz16(struct decoded_instruction* i)
{
    jcc16(!cpu_get_zf());
}
OPTYPE op_jnz32(struct decoded_instruction* i)
{
    jcc32(!cpu_get_zf());
}
OPTYPE op_jbe16(struct decoded_instruction* i)
{
    jcc16(cpu_get_zf() || cpu_get_cf());
}
OPTYPE op_jbe32(struct decoded_instruction* i)
{
    jcc32(cpu_get_zf() || cpu_get_cf());
}
OPTYPE op_jnbe16(struct decoded_instruction* i)
{
    jcc16(!(cpu_get_zf() || cpu_get_cf()));
}
OPTYPE op_jnbe32(struct decoded_instruction* i)
{
    jcc32(!(cpu_get_zf() || cpu_get_cf()));
}
OPTYPE op_js16(struct decoded_instruction* i)
{
    jcc16(cpu_get_sf());
}
OPTYPE op_js32(struct decoded_instruction* i)
{
    jcc32(cpu_get_sf());
}
OPTYPE op_jns16(struct decoded_instruction* i)
{
    jcc16(!cpu_get_sf());
}
OPTYPE op_jns32(struct decoded_instruction* i)
{
    jcc32(!cpu_get_sf());
}
OPTYPE op_jp16(struct decoded_instruction* i)
{
    jcc16(cpu_get_pf());
}
OPTYPE op_jp32(struct decoded_instruction* i)
{
    jcc32(cpu_get_pf());
}
OPTYPE op_jnp16(struct decoded_instruction* i)
{
    jcc16(!cpu_get_pf());
}
OPTYPE op_jnp32(struct decoded_instruction* i)
{
    jcc32(!cpu_get_pf());
}
OPTYPE op_jl16(struct decoded_instruction* i)
{
    jcc16(cpu_get_sf() != cpu_get_of());
}
OPTYPE op_jl32(struct decoded_instruction* i)
{
    jcc32(cpu_get_sf() != cpu_get_of());
}
OPTYPE op_jnl16(struct decoded_instruction* i)
{
    jcc16(cpu_get_sf() == cpu_get_of());
}
OPTYPE op_jnl32(struct decoded_instruction* i)
{
    jcc32(cpu_get_sf() == cpu_get_of());
}
OPTYPE op_jle16(struct decoded_instruction* i)
{
    jcc16(cpu_get_zf() || (cpu_get_sf() != cpu_get_of()));
}
OPTYPE op_jle32(struct decoded_instruction* i)
{
    jcc32(cpu_get_zf() || (cpu_get_sf() != cpu_get_of()));
}
OPTYPE op_jnle16(struct decoded_instruction* i)
{
    jcc16(!cpu_get_zf() && (cpu_get_sf() == cpu_get_of()));
}
OPTYPE op_jnle32(struct decoded_instruction* i)
{
    jcc32(!cpu_get_zf() && (cpu_get_sf() == cpu_get_of()));
}
// <<< END AUTOGENERATE "jcc" >>>

OPTYPE op_call_j16(struct decoded_instruction* i)
{
    uint32_t virt_base = VIRT_EIP(), virt = virt_base + i->flags;
    push16(virt);
    thread_cpu.phys_eip += ((virt + i->imm32) & 0xFFFF) - virt_base;
    STOP();
}
OPTYPE op_call_j32(struct decoded_instruction* i)
{
    uint32_t flags = i->flags, virt = VIRT_EIP() + flags;
    push32(virt);
    thread_cpu.phys_eip += flags + i->imm32;
    STOP();
}
OPTYPE op_ret16(struct decoded_instruction* i)
{
    UNUSED(i);
    pop16(&temp.d16);
    SET_VIRT_EIP(temp.d16);
    STOP();
}
OPTYPE op_ret32(struct decoded_instruction* i)
{
    UNUSED(i);
    pop32(&temp.d32);
    SET_VIRT_EIP(temp.d32);
    STOP();
}
OPTYPE op_ret16_iw(struct decoded_instruction* i)
{
    UNUSED(i);
    pop16(&temp.d16);
    SET_VIRT_EIP(temp.d16);
    thread_cpu.reg32[ESP] = ((thread_cpu.reg32[ESP] + i->imm16) & thread_cpu.esp_mask) | (thread_cpu.reg32[ESP] & ~thread_cpu.esp_mask);
    STOP();
}
OPTYPE op_ret32_iw(struct decoded_instruction* i)
{
    UNUSED(i);
    pop32(&temp.d32);
    SET_VIRT_EIP(temp.d32);
    thread_cpu.reg32[ESP] = ((thread_cpu.reg32[ESP] + i->imm16) & thread_cpu.esp_mask) | (thread_cpu.reg32[ESP] & ~thread_cpu.esp_mask);
    STOP();
}

OPTYPE op_int(struct decoded_instruction* i)
{
    if (cpu_interrupt(i->imm8, 0, INTERRUPT_TYPE_SOFTWARE, VIRT_EIP() + i->flags))
        EXCEP();
    STOP();
}
OPTYPE op_into(struct decoded_instruction* i)
{
    if (cpu_get_of()) {
        if (cpu_interrupt(4, 0, INTERRUPT_TYPE_SOFTWARE, VIRT_EIP() + i->flags))
            EXCEP();
        STOP();
    }
    NEXT2(i->flags);
}

// Stack
OPTYPE op_push_r16(struct decoded_instruction* i)
{
    int flags = i->flags;
    push16(R16(I_RM(flags)));
    NEXT(flags);
}
OPTYPE op_push_i16(struct decoded_instruction* i)
{
    int flags = i->flags;
    push16(i->imm16);
    NEXT(flags);
}
OPTYPE op_push_e16(struct decoded_instruction* i)
{
    // Order of operations:
    //  - Compute linear address using *pre-push* ESP if needed
    //  - Read from address
    //  - Compute push destination address
    //  - Write to address
    //  - Commit real ESP
    int flags = i->flags, linaddr = cpu_get_linaddr(flags, i);
    uint16_t src;
    cpu_read16(linaddr, src, thread_cpu.tlb_shift_read);
    push16(src);
    NEXT(flags);
}
OPTYPE op_push_r32(struct decoded_instruction* i)
{
    int flags = i->flags;
    push32(R32(I_RM(flags)));
    NEXT(flags);
}
OPTYPE op_push_i32(struct decoded_instruction* i)
{
    int flags = i->flags;
    push32(i->imm32);
    NEXT(flags);
}
OPTYPE op_push_e32(struct decoded_instruction* i)
{
    int flags = i->flags, linaddr = cpu_get_linaddr(flags, i);
    uint32_t src;
    cpu_read32(linaddr, src, thread_cpu.tlb_shift_read);
    push32(src);
    NEXT(flags);
}
OPTYPE op_pop_r16(struct decoded_instruction* i)
{
    int flags = i->flags;
    // Nasty hack to make "pop sp" work
    pop16(&temp.d16);
    R16(I_RM(flags)) = temp.d16;
    NEXT(flags);
}
OPTYPE op_pop_e16(struct decoded_instruction* i)
{
    // Order of operations:
    //  - Compute pop source address
    //  - Read from address
    //  - Compute destination address using *post-pop* ESP
    //  - Write to address
    // Nasty hack ahead
    uint32_t prev_esp = thread_cpu.reg32[ESP], linaddr, flags = i->flags, temp_esp;

    pop16(&temp.d16);
    linaddr = cpu_get_linaddr(flags, i);

    // Just in case we fault here, have ESP set to the right value
    temp_esp = thread_cpu.reg32[ESP];
    thread_cpu.reg32[ESP] = prev_esp;
    cpu_write16(linaddr, temp.d16, thread_cpu.tlb_shift_write);
    thread_cpu.reg32[ESP] = temp_esp;

    NEXT(flags);
}
OPTYPE op_pop_e32(struct decoded_instruction* i)
{
    uint32_t prev_esp = thread_cpu.reg32[ESP], linaddr, flags = i->flags, temp_esp;
    pop32(&temp.d32);
    linaddr = cpu_get_linaddr(flags, i);

    temp_esp = thread_cpu.reg32[ESP];
    thread_cpu.reg32[ESP] = prev_esp;
    cpu_write32(linaddr, temp.d32, thread_cpu.tlb_shift_write);
    thread_cpu.reg32[ESP] = temp_esp;

    NEXT(flags);
}

OPTYPE op_pop_r32(struct decoded_instruction* i)
{
    int flags = i->flags;
    pop32(&temp.d32);
    R32(I_RM(flags)) = temp.d32;
    NEXT(flags);
}
OPTYPE op_push_s16(struct decoded_instruction* i)
{
    int flags = i->flags;
    push16(thread_cpu.seg[I_RM(flags)]);
    NEXT(flags);
}
OPTYPE op_push_s32(struct decoded_instruction* i)
{
    int flags = i->flags;
    uint32_t esp = thread_cpu.reg32[ESP], esp_mask = thread_cpu.esp_mask, esp_minus_four = (esp - 4) & esp_mask;
    cpu_write16(esp_minus_four + thread_cpu.seg_base[SS], thread_cpu.seg[I_RM(flags)], thread_cpu.tlb_shift_write);
    thread_cpu.reg32[ESP] = esp_minus_four | (esp & ~esp_mask);
    NEXT(flags);
}
OPTYPE op_pop_s16(struct decoded_instruction* i)
{
    // This instruction must be treated very carefully
    // We cannot use pop16 for this operation
    int flags = i->flags, seg_dest = I_RM(flags);
    uint16_t dest;
    cpu_read16((thread_cpu.reg32[ESP] & thread_cpu.esp_mask) + thread_cpu.seg_base[SS], dest, thread_cpu.tlb_shift_read);
    if (cpu_load_seg_value_mov(seg_dest, dest))
        EXCEP();
    thread_cpu.reg32[ESP] = ((thread_cpu.reg32[ESP] + 2) & thread_cpu.esp_mask) | (thread_cpu.reg32[ESP] & ~thread_cpu.esp_mask);
    if (seg_dest == SS)
        interrupt_guard();
    NEXT(flags);
}
OPTYPE op_pop_s32(struct decoded_instruction* i)
{
    // Identical to above except ESP is incremented by 4
    int flags = i->flags, seg_dest = I_RM(flags);
    uint16_t dest;
    cpu_read16((thread_cpu.reg32[ESP] & thread_cpu.esp_mask) + thread_cpu.seg_base[SS], dest, thread_cpu.tlb_shift_read);
    if (cpu_load_seg_value_mov(seg_dest, dest))
        EXCEP();
    thread_cpu.reg32[ESP] = ((thread_cpu.reg32[ESP] + 4) & thread_cpu.esp_mask) | (thread_cpu.reg32[ESP] & ~thread_cpu.esp_mask);
    if (seg_dest == SS)
        interrupt_guard();
    NEXT(flags);
}
OPTYPE op_pusha(struct decoded_instruction* i)
{
    if (cpu_pusha())
        EXCEP();
    NEXT(i->flags);
}
OPTYPE op_pushad(struct decoded_instruction* i)
{
    if (cpu_pushad())
        EXCEP();
    NEXT(i->flags);
}
OPTYPE op_popa(struct decoded_instruction* i)
{
    if (cpu_popa())
        EXCEP();
    NEXT(i->flags);
}
OPTYPE op_popad(struct decoded_instruction* i)
{
    if (cpu_popad())
        EXCEP();
    NEXT(i->flags);
}

// Arithmetic
OPTYPE op_arith_r8r8(struct decoded_instruction* i)
{
    int flags = i->flags;
    cpu_arith8(I_OP(flags), &R8(I_RM(flags)), R8(I_REG(flags)));
    NEXT(flags);
}
OPTYPE op_arith_r8i8(struct decoded_instruction* i)
{
    int flags = i->flags;
    cpu_arith8(I_OP(flags), &R8(I_RM(flags)), i->imm8);
    NEXT(flags);
}
OPTYPE op_arith_r8e8(struct decoded_instruction* i)
{
    arith_re(8, cpu_arith8);
}
OPTYPE op_arith_e8r8(struct decoded_instruction* i)
{
    arith_rmw(8, cpu_arith8, R8(I_REG(flags)));
}
OPTYPE op_arith_e8i8(struct decoded_instruction* i)
{
    arith_rmw(8, cpu_arith8, i->imm8);
}

OPTYPE op_arith_r16r16(struct decoded_instruction* i)
{
    int flags = i->flags;
    cpu_arith16(I_OP(flags), &R16(I_RM(flags)), R16(I_REG(flags)));
    NEXT(flags);
}
OPTYPE op_arith_r16i16(struct decoded_instruction* i)
{
    int flags = i->flags;
    cpu_arith16(I_OP(flags), &R16(I_RM(flags)), i->imm16);
    NEXT(flags);
}
OPTYPE op_arith_r16e16(struct decoded_instruction* i)
{
    arith_re(16, cpu_arith16);
}
OPTYPE op_arith_e16r16(struct decoded_instruction* i)
{
    arith_rmw(16, cpu_arith16, R16(I_REG(flags)));
}
OPTYPE op_arith_e16i16(struct decoded_instruction* i)
{
    arith_rmw(16, cpu_arith16, i->imm16);
}
OPTYPE op_arith_r32r32(struct decoded_instruction* i)
{
    int flags = i->flags;
    cpu_arith32(I_OP(flags), &R32(I_RM(flags)), R32(I_REG(flags)));
    NEXT(flags);
}
OPTYPE op_arith_r32i32(struct decoded_instruction* i)
{
    int flags = i->flags;
    cpu_arith32(I_OP(flags), &R32(I_RM(flags)), i->imm32);
    NEXT(flags);
}
OPTYPE op_arith_r32e32(struct decoded_instruction* i)
{
    arith_re(32, cpu_arith32);
}
OPTYPE op_arith_e32r32(struct decoded_instruction* i)
{
    arith_rmw(32, cpu_arith32, R32(I_REG(flags)));
}
OPTYPE op_arith_e32i32(struct decoded_instruction* i)
{
    arith_rmw(32, cpu_arith32, i->imm32);
}

OPTYPE op_shift_r8cl(struct decoded_instruction* i)
{
    int flags = i->flags;
    cpu_shift8(I_OP(flags), &R8(I_RM(flags)), thread_cpu.reg8[CL]);
    NEXT(flags);
}
OPTYPE op_shift_r8i8(struct decoded_instruction* i)
{
    int flags = i->flags;
    cpu_shift8(I_OP(flags), &R8(I_RM(flags)), i->imm8);
    NEXT(flags);
}
OPTYPE op_shift_e8cl(struct decoded_instruction* i)
{
    arith_rmw(8, cpu_shift8, thread_cpu.reg8[CL]);
}
OPTYPE op_shift_e8i8(struct decoded_instruction* i)
{
    arith_rmw(8, cpu_shift8, i->imm8);
}
OPTYPE op_shift_r16cl(struct decoded_instruction* i)
{
    int flags = i->flags;
    cpu_shift16(I_OP(flags), &R16(I_RM(flags)), thread_cpu.reg8[CL]);
    NEXT(flags);
}
OPTYPE op_shift_r16i16(struct decoded_instruction* i)
{
    int flags = i->flags;
    cpu_shift16(I_OP(flags), &R16(I_RM(flags)), i->imm8);
    NEXT(flags);
}
OPTYPE op_shift_e16cl(struct decoded_instruction* i)
{
    arith_rmw(16, cpu_shift16, thread_cpu.reg8[CL]);
}
OPTYPE op_shift_e16i16(struct decoded_instruction* i)
{
    arith_rmw(16, cpu_shift16, i->imm8);
}
OPTYPE op_shift_r32cl(struct decoded_instruction* i)
{
    int flags = i->flags;
    cpu_shift32(I_OP(flags), &R32(I_RM(flags)), thread_cpu.reg8[CL]);
    NEXT(flags);
}
OPTYPE op_shift_r32i32(struct decoded_instruction* i)
{
    int flags = i->flags;
    cpu_shift32(I_OP(flags), &R32(I_RM(flags)), i->imm8);
    NEXT(flags);
}
OPTYPE op_shift_e32cl(struct decoded_instruction* i)
{
    arith_rmw(32, cpu_shift32, thread_cpu.reg8[CL]);
}
OPTYPE op_shift_e32i32(struct decoded_instruction* i)
{
    arith_rmw(32, cpu_shift32, i->imm8);
}

OPTYPE op_cmp_e8r8(struct decoded_instruction* i)
{
    uint32_t flags = i->flags;
    uint8_t src;
    cpu_read8(cpu_get_linaddr(flags, i), src, thread_cpu.tlb_shift_read);
    thread_cpu.lop2 = R8(I_REG(flags));
    thread_cpu.lr = (int8_t)(src - thread_cpu.lop2);
    thread_cpu.laux = SUB8;
    NEXT(flags);
}
OPTYPE op_cmp_r8r8(struct decoded_instruction* i)
{
    uint32_t flags = i->flags;
    thread_cpu.lop2 = R8(I_REG(flags));
    thread_cpu.lr = (int8_t)(R8(I_RM(flags)) - thread_cpu.lop2);
    thread_cpu.laux = SUB8;
    NEXT(flags);
}
OPTYPE op_cmp_r8e8(struct decoded_instruction* i)
{
    uint32_t flags = i->flags;
    uint8_t src;
    cpu_read8(cpu_get_linaddr(flags, i), src, thread_cpu.tlb_shift_read);
    thread_cpu.lop2 = src;
    thread_cpu.lr = (int8_t)(R8(I_REG(flags)) - src);
    thread_cpu.laux = SUB8;
    NEXT(flags);
}
OPTYPE op_cmp_r8i8(struct decoded_instruction* i)
{
    uint32_t flags = i->flags;
    thread_cpu.lop2 = i->imm8;
    thread_cpu.lr = (int8_t)(R8(I_RM(flags)) - thread_cpu.lop2);
    thread_cpu.laux = SUB8;
    NEXT(flags);
}
OPTYPE op_cmp_e8i8(struct decoded_instruction* i)
{
    uint32_t flags = i->flags;
    uint8_t src;
    cpu_read8(cpu_get_linaddr(flags, i), src, thread_cpu.tlb_shift_read);
    thread_cpu.lop2 = i->imm8;
    thread_cpu.lr = (int8_t)(src - thread_cpu.lop2);
    thread_cpu.laux = SUB8;
    NEXT(flags);
}

OPTYPE op_cmp_e16r16(struct decoded_instruction* i)
{
    uint32_t flags = i->flags;
    uint16_t src;
    cpu_read16(cpu_get_linaddr(flags, i), src, thread_cpu.tlb_shift_read);
    thread_cpu.lop2 = R16(I_REG(flags));
    thread_cpu.lr = (int16_t)(src - thread_cpu.lop2);
    thread_cpu.laux = SUB16;
    NEXT(flags);
}
OPTYPE op_cmp_r16r16(struct decoded_instruction* i)
{
    uint32_t flags = i->flags;
    thread_cpu.lop2 = R16(I_REG(flags));
    thread_cpu.lr = (int16_t)(R16(I_RM(flags)) - thread_cpu.lop2);
    thread_cpu.laux = SUB16;
    NEXT(flags);
}
OPTYPE op_cmp_r16e16(struct decoded_instruction* i)
{
    uint32_t flags = i->flags;
    uint16_t src;
    cpu_read16(cpu_get_linaddr(flags, i), src, thread_cpu.tlb_shift_read);
    thread_cpu.lop2 = src;
    thread_cpu.lr = (int16_t)(R16(I_REG(flags)) - src);
    thread_cpu.laux = SUB16;
    NEXT(flags);
}
OPTYPE op_cmp_r16i16(struct decoded_instruction* i)
{
    uint32_t flags = i->flags;
    thread_cpu.lop2 = i->imm16;
    thread_cpu.lr = (int16_t)(R16(I_RM(flags)) - thread_cpu.lop2);
    thread_cpu.laux = SUB16;
    NEXT(flags);
}
OPTYPE op_cmp_e16i16(struct decoded_instruction* i)
{
    uint32_t flags = i->flags;
    uint16_t src;
    cpu_read16(cpu_get_linaddr(flags, i), src, thread_cpu.tlb_shift_read);
    thread_cpu.lop2 = i->imm16;
    thread_cpu.lr = (int16_t)(src - thread_cpu.lop2);
    thread_cpu.laux = SUB16;
    NEXT(flags);
}
OPTYPE op_cmp_e32r32(struct decoded_instruction* i)
{
    uint32_t flags = i->flags;
    uint32_t src;
    cpu_read32(cpu_get_linaddr(flags, i), src, thread_cpu.tlb_shift_read);
    thread_cpu.lop2 = R32(I_REG(flags));
    thread_cpu.lr = (int32_t)(src - thread_cpu.lop2);
    thread_cpu.laux = SUB32;
    NEXT(flags);
}
OPTYPE op_cmp_r32r32(struct decoded_instruction* i)
{
    uint32_t flags = i->flags;
    thread_cpu.lop2 = R32(I_REG(flags));
    thread_cpu.lr = (int32_t)(R32(I_RM(flags)) - thread_cpu.lop2);
    thread_cpu.laux = SUB32;
    NEXT(flags);
}
OPTYPE op_cmp_r32e32(struct decoded_instruction* i)
{
    uint32_t flags = i->flags;
    uint32_t src;
    cpu_read32(cpu_get_linaddr(flags, i), src, thread_cpu.tlb_shift_read);
    thread_cpu.lop2 = src;
    thread_cpu.lr = (int32_t)(R32(I_REG(flags)) - src);
    thread_cpu.laux = SUB32;
    NEXT(flags);
}
OPTYPE op_cmp_r32i32(struct decoded_instruction* i)
{
    uint32_t flags = i->flags;
    thread_cpu.lop2 = i->imm32;
    thread_cpu.lr = (int32_t)(R32(I_RM(flags)) - thread_cpu.lop2);
    thread_cpu.laux = SUB32;
    NEXT(flags);
}
OPTYPE op_cmp_e32i32(struct decoded_instruction* i)
{
    uint32_t flags = i->flags;
    uint32_t src;
    cpu_read32(cpu_get_linaddr(flags, i), src, thread_cpu.tlb_shift_read);
    thread_cpu.lop2 = i->imm32;
    thread_cpu.lr = (int32_t)(src - thread_cpu.lop2);
    thread_cpu.laux = SUB32;
    NEXT(flags);
}

OPTYPE op_test_e8r8(struct decoded_instruction* i)
{
    uint32_t flags = i->flags;
    uint8_t src;
    cpu_read8(cpu_get_linaddr(flags, i), src, thread_cpu.tlb_shift_read);
    thread_cpu.lr = (int8_t)(src & R8(I_REG(flags)));
    thread_cpu.laux = BIT;
    NEXT(flags);
}
OPTYPE op_test_r8r8(struct decoded_instruction* i)
{
    uint32_t flags = i->flags;
    thread_cpu.lr = (int8_t)(R8(I_RM(flags)) & R8(I_REG(flags)));
    thread_cpu.laux = BIT;
    NEXT(flags);
}
OPTYPE op_test_r8e8(struct decoded_instruction* i)
{
    uint32_t flags = i->flags;
    uint8_t src;
    cpu_read8(cpu_get_linaddr(flags, i), src, thread_cpu.tlb_shift_read);
    thread_cpu.lr = (int8_t)(src & R8(I_REG(flags)));
    thread_cpu.laux = BIT;
    NEXT(flags);
}
OPTYPE op_test_r8i8(struct decoded_instruction* i)
{
    uint32_t flags = i->flags;
    thread_cpu.lr = (int8_t)(R8(I_RM(flags)) & i->imm8);
    thread_cpu.laux = BIT;
    NEXT(flags);
}
OPTYPE op_test_e8i8(struct decoded_instruction* i)
{
    uint32_t flags = i->flags;
    uint8_t src;
    cpu_read8(cpu_get_linaddr(flags, i), src, thread_cpu.tlb_shift_read);
    thread_cpu.lr = (int8_t)(i->imm8 & src);
    thread_cpu.laux = BIT;
    NEXT(flags);
}
OPTYPE op_test_e16r16(struct decoded_instruction* i)
{
    uint32_t flags = i->flags;
    uint16_t src;
    cpu_read16(cpu_get_linaddr(flags, i), src, thread_cpu.tlb_shift_read);
    thread_cpu.lr = (int16_t)(src & R16(I_REG(flags)));
    thread_cpu.laux = BIT;
    NEXT(flags);
}
OPTYPE op_test_r16r16(struct decoded_instruction* i)
{
    uint32_t flags = i->flags;
    thread_cpu.lr = (int16_t)(R16(I_RM(flags)) & R16(I_REG(flags)));
    thread_cpu.laux = BIT;
    NEXT(flags);
}
OPTYPE op_test_r16e16(struct decoded_instruction* i)
{
    uint32_t flags = i->flags;
    uint16_t src;
    cpu_read16(cpu_get_linaddr(flags, i), src, thread_cpu.tlb_shift_read);
    thread_cpu.lr = (int16_t)(src & R16(I_REG(flags)));
    thread_cpu.laux = BIT;
    NEXT(flags);
}
OPTYPE op_test_r16i16(struct decoded_instruction* i)
{
    uint32_t flags = i->flags;
    thread_cpu.lr = (int16_t)(R16(I_RM(flags)) & i->imm16);
    thread_cpu.laux = BIT;
    NEXT(flags);
}
OPTYPE op_test_e16i16(struct decoded_instruction* i)
{
    uint32_t flags = i->flags;
    uint16_t src;
    cpu_read16(cpu_get_linaddr(flags, i), src, thread_cpu.tlb_shift_read);
    thread_cpu.lr = (int16_t)(i->imm16 & src);
    thread_cpu.laux = BIT;
    NEXT(flags);
}
OPTYPE op_test_e32r32(struct decoded_instruction* i)
{
    uint32_t flags = i->flags;
    uint32_t src;
    cpu_read32(cpu_get_linaddr(flags, i), src, thread_cpu.tlb_shift_read);
    thread_cpu.lr = (int32_t)(src & R32(I_REG(flags)));
    thread_cpu.laux = BIT;
    NEXT(flags);
}
OPTYPE op_test_r32r32(struct decoded_instruction* i)
{
    uint32_t flags = i->flags;
    thread_cpu.lr = (int32_t)(R32(I_RM(flags)) & R32(I_REG(flags)));
    thread_cpu.laux = BIT;
    NEXT(flags);
}
OPTYPE op_test_r32e32(struct decoded_instruction* i)
{
    uint32_t flags = i->flags;
    uint32_t src;
    cpu_read32(cpu_get_linaddr(flags, i), src, thread_cpu.tlb_shift_read);
    thread_cpu.lr = (int32_t)(src & R32(I_REG(flags)));
    thread_cpu.laux = BIT;
    NEXT(flags);
}
OPTYPE op_test_r32i32(struct decoded_instruction* i)
{
    uint32_t flags = i->flags;
    thread_cpu.lr = (int32_t)(R32(I_RM(flags)) & i->imm32);
    thread_cpu.laux = BIT;
    NEXT(flags);
}
OPTYPE op_test_e32i32(struct decoded_instruction* i)
{
    uint32_t flags = i->flags;
    uint32_t src;
    cpu_read32(cpu_get_linaddr(flags, i), src, thread_cpu.tlb_shift_read);
    thread_cpu.lr = (int32_t)(i->imm32 & src);
    thread_cpu.laux = BIT;
    NEXT(flags);
}

OPTYPE op_inc_r8(struct decoded_instruction* i)
{
    uint32_t flags = i->flags;
    cpu_inc8(&R8(I_RM(flags)));
    NEXT(flags);
}
OPTYPE op_inc_e8(struct decoded_instruction* i)
{
    arith_rmw2(8, cpu_inc8);
}
OPTYPE op_inc_r16(struct decoded_instruction* i)
{
    uint32_t flags = i->flags;
    cpu_inc16(&R16(I_RM(flags)));
    NEXT(flags);
}
OPTYPE op_inc_e16(struct decoded_instruction* i)
{
    arith_rmw2(16, cpu_inc16);
}
OPTYPE op_inc_r32(struct decoded_instruction* i)
{
    uint32_t flags = i->flags;
    cpu_inc32(&R32(I_RM(flags)));
    NEXT(flags);
}
OPTYPE op_inc_e32(struct decoded_instruction* i)
{
    arith_rmw2(32, cpu_inc32);
}
OPTYPE op_dec_r8(struct decoded_instruction* i)
{
    uint32_t flags = i->flags;
    cpu_dec8(&R8(I_RM(flags)));
    NEXT(flags);
}
OPTYPE op_dec_e8(struct decoded_instruction* i)
{
    arith_rmw2(8, cpu_dec8);
}
OPTYPE op_dec_r16(struct decoded_instruction* i)
{
    uint32_t flags = i->flags;
    cpu_dec16(&R16(I_RM(flags)));
    NEXT(flags);
}
OPTYPE op_dec_e16(struct decoded_instruction* i)
{
    arith_rmw2(16, cpu_dec16);
}
OPTYPE op_dec_r32(struct decoded_instruction* i)
{
    uint32_t flags = i->flags;
    cpu_dec32(&R32(I_RM(flags)));
    NEXT(flags);
}
OPTYPE op_dec_e32(struct decoded_instruction* i)
{
    arith_rmw2(32, cpu_dec32);
}

OPTYPE op_not_r8(struct decoded_instruction* i)
{
    int flags = i->flags, rm = I_RM(flags);
    R8(rm) = ~R8(rm);
    NEXT(flags);
}
OPTYPE op_not_e8(struct decoded_instruction* i)
{
    arith_rmw2(8, cpu_not8);
}
OPTYPE op_not_r16(struct decoded_instruction* i)
{
    int flags = i->flags, rm = I_RM(flags);
    R16(rm) = ~R16(rm);
    NEXT(flags);
}
OPTYPE op_not_e16(struct decoded_instruction* i)
{
    arith_rmw2(16, cpu_not16);
}
OPTYPE op_not_r32(struct decoded_instruction* i)
{
    int flags = i->flags, rm = I_RM(flags);
    R32(rm) = ~R32(rm);
    NEXT(flags);
}
OPTYPE op_not_e32(struct decoded_instruction* i)
{
    arith_rmw2(32, cpu_not32);
}
OPTYPE op_neg_r8(struct decoded_instruction* i)
{
    int flags = i->flags, rm = I_RM(flags);
    cpu_neg8(&R8(rm));
    NEXT(flags);
}
OPTYPE op_neg_e8(struct decoded_instruction* i)
{
    arith_rmw2(8, cpu_neg8);
}
OPTYPE op_neg_r16(struct decoded_instruction* i)
{
    int flags = i->flags, rm = I_RM(flags);
    cpu_neg16(&R16(rm));
    NEXT(flags);
}
OPTYPE op_neg_e16(struct decoded_instruction* i)
{
    arith_rmw2(16, cpu_neg16);
}
OPTYPE op_neg_r32(struct decoded_instruction* i)
{
    int flags = i->flags, rm = I_RM(flags);
    cpu_neg32(&R32(rm));
    NEXT(flags);
}
OPTYPE op_neg_e32(struct decoded_instruction* i)
{
    arith_rmw2(32, cpu_neg32);
}

OPTYPE op_muldiv_r8(struct decoded_instruction* i)
{
    uint32_t flags = i->flags;
    if (cpu_muldiv8(I_OP(flags), R8(I_RM(flags))))
        EXCEP();
    NEXT(flags);
}
OPTYPE op_muldiv_e8(struct decoded_instruction* i)
{
    uint32_t flags = i->flags, linaddr = cpu_get_linaddr(flags, i);
    uint8_t src;
    cpu_read8(linaddr, src, thread_cpu.tlb_shift_read);
    if (cpu_muldiv8(I_OP(flags), src))
        EXCEP();
    NEXT(flags);
}
OPTYPE op_muldiv_r16(struct decoded_instruction* i)
{
    uint32_t flags = i->flags;
    if (cpu_muldiv16(I_OP(flags), R16(I_RM(flags))))
        EXCEP();
    NEXT(flags);
}
OPTYPE op_muldiv_e16(struct decoded_instruction* i)
{
    uint32_t flags = i->flags, linaddr = cpu_get_linaddr(flags, i);
    uint16_t src;
    cpu_read16(linaddr, src, thread_cpu.tlb_shift_read);
    if (cpu_muldiv16(I_OP(flags), src))
        EXCEP();
    NEXT(flags);
}
OPTYPE op_muldiv_r32(struct decoded_instruction* i)
{
    uint32_t flags = i->flags;
    if (cpu_muldiv32(I_OP(flags), R32(I_RM(flags))))
        EXCEP();
    NEXT(flags);
}
OPTYPE op_muldiv_e32(struct decoded_instruction* i)
{
    uint32_t flags = i->flags, linaddr = cpu_get_linaddr(flags, i);
    uint32_t src;
    cpu_read32(linaddr, src, thread_cpu.tlb_shift_read);
    if (cpu_muldiv32(I_OP(flags), src))
        EXCEP();
    NEXT(flags);
}

OPTYPE op_imul_r16r16i16(struct decoded_instruction* i)
{
    uint32_t flags = i->flags;
    R16(I_REG(flags)) = cpu_imul16(R16(I_RM(flags)), i->imm16);
    NEXT(flags);
}
OPTYPE op_imul_r16e16i16(struct decoded_instruction* i)
{
    uint32_t flags = i->flags, linaddr = cpu_get_linaddr(flags, i);
    uint16_t src;
    cpu_read16(linaddr, src, thread_cpu.tlb_shift_read);
    R16(I_REG(flags)) = cpu_imul16(src, i->imm16);
    NEXT(flags);
}
OPTYPE op_imul_r32r32i32(struct decoded_instruction* i)
{
    uint32_t flags = i->flags;
    R32(I_REG(flags)) = cpu_imul32(R32(I_RM(flags)), i->imm32);
    NEXT(flags);
}
OPTYPE op_imul_r32e32i32(struct decoded_instruction* i)
{
    uint32_t flags = i->flags, linaddr = cpu_get_linaddr(flags, i);
    uint32_t src;
    cpu_read32(linaddr, src, thread_cpu.tlb_shift_read);
    R32(I_REG(flags)) = cpu_imul32(src, i->imm32);
    NEXT(flags);
}

OPTYPE op_imul_r16r16(struct decoded_instruction* i)
{
    uint32_t flags = i->flags;
    R16(I_REG(flags)) = cpu_imul16(R16(I_REG(flags)), R16(I_RM(flags)));
    NEXT(flags);
}
OPTYPE op_imul_r32r32(struct decoded_instruction* i)
{
    uint32_t flags = i->flags;
    R32(I_REG(flags)) = cpu_imul32(R32(I_REG(flags)), R32(I_RM(flags)));
    NEXT(flags);
}
OPTYPE op_imul_r16e16(struct decoded_instruction* i)
{
    uint32_t flags = i->flags, linaddr = cpu_get_linaddr(flags, i);
    uint16_t src;
    cpu_read16(linaddr, src, thread_cpu.tlb_shift_read);
    R16(I_REG(flags)) = cpu_imul16(R16(I_REG(flags)), src);
    NEXT(flags);
}
OPTYPE op_imul_r32e32(struct decoded_instruction* i)
{
    uint32_t flags = i->flags, linaddr = cpu_get_linaddr(flags, i);
    uint32_t src;
    cpu_read32(linaddr, src, thread_cpu.tlb_shift_read);
    R32(I_REG(flags)) = cpu_imul32(R32(I_REG(flags)), src);
    NEXT(flags);
}

OPTYPE op_shrd_r16r16i8(struct decoded_instruction* i)
{
    int flags = i->flags;
    cpu_shrd16(&R16(I_RM(flags)), R16(I_REG(flags)), i->imm8);
    NEXT(flags);
}
OPTYPE op_shrd_r32r32i8(struct decoded_instruction* i)
{
    int flags = i->flags;
    cpu_shrd32(&R32(I_RM(flags)), R32(I_REG(flags)), i->imm8);
    NEXT(flags);
}
OPTYPE op_shrd_r16r16cl(struct decoded_instruction* i)
{
    int flags = i->flags;
    cpu_shrd16(&R16(I_RM(flags)), R16(I_REG(flags)), thread_cpu.reg8[CL]);
    NEXT(flags);
}
OPTYPE op_shrd_r32r32cl(struct decoded_instruction* i)
{
    int flags = i->flags;
    cpu_shrd32(&R32(I_RM(flags)), R32(I_REG(flags)), thread_cpu.reg8[CL]);
    NEXT(flags);
}
OPTYPE op_shrd_e16r16i8(struct decoded_instruction* i)
{
    arith_rmw2(16, cpu_shrd16, R16(I_REG(flags)), i->imm8);
    NEXT(flags);
}
OPTYPE op_shrd_e32r32i8(struct decoded_instruction* i)
{
    arith_rmw2(32, cpu_shrd32, R32(I_REG(flags)), i->imm8);
    NEXT(flags);
}
OPTYPE op_shrd_e16r16cl(struct decoded_instruction* i)
{
    arith_rmw2(16, cpu_shrd16, R16(I_REG(flags)), thread_cpu.reg8[CL]);
    NEXT(flags);
}
OPTYPE op_shrd_e32r32cl(struct decoded_instruction* i)
{
    arith_rmw2(32, cpu_shrd32, R32(I_REG(flags)), thread_cpu.reg8[CL]);
    NEXT(flags);
}
OPTYPE op_shld_r16r16i8(struct decoded_instruction* i)
{
    int flags = i->flags;
    cpu_shld16(&R16(I_RM(flags)), R16(I_REG(flags)), i->imm8);
    NEXT(flags);
}
OPTYPE op_shld_r32r32i8(struct decoded_instruction* i)
{
    int flags = i->flags;
    cpu_shld32(&R32(I_RM(flags)), R32(I_REG(flags)), i->imm8);
    NEXT(flags);
}
OPTYPE op_shld_r16r16cl(struct decoded_instruction* i)
{
    int flags = i->flags;
    cpu_shld16(&R16(I_RM(flags)), R16(I_REG(flags)), thread_cpu.reg8[CL]);
    NEXT(flags);
}
OPTYPE op_shld_r32r32cl(struct decoded_instruction* i)
{
    int flags = i->flags;
    cpu_shld32(&R32(I_RM(flags)), R32(I_REG(flags)), thread_cpu.reg8[CL]);
    NEXT(flags);
}
OPTYPE op_shld_e16r16i8(struct decoded_instruction* i)
{
    arith_rmw2(16, cpu_shld16, R16(I_REG(flags)), i->imm8);
    NEXT(flags);
}
OPTYPE op_shld_e32r32i8(struct decoded_instruction* i)
{
    arith_rmw2(32, cpu_shld32, R32(I_REG(flags)), i->imm8);
    NEXT(flags);
}
OPTYPE op_shld_e16r16cl(struct decoded_instruction* i)
{
    arith_rmw2(16, cpu_shld16, R16(I_REG(flags)), thread_cpu.reg8[CL]);
    NEXT(flags);
}
OPTYPE op_shld_e32r32cl(struct decoded_instruction* i)
{
    arith_rmw2(32, cpu_shld32, R32(I_REG(flags)), thread_cpu.reg8[CL]);
    NEXT(flags);
}

// I/O opcodes

OPTYPE op_out_i8al(struct decoded_instruction* i)
{
    UNUSED(i);
    abort();
}
OPTYPE op_out_i8ax(struct decoded_instruction* i)
{
    UNUSED(i);
    abort();
}
OPTYPE op_out_i8eax(struct decoded_instruction* i)
{
    UNUSED(i);
    abort();
}
OPTYPE op_in_i8al(struct decoded_instruction* i)
{
    UNUSED(i);
    abort();
}
OPTYPE op_in_i8ax(struct decoded_instruction* i)
{
    UNUSED(i);
    abort();
}
OPTYPE op_in_i8eax(struct decoded_instruction* i)
{
    UNUSED(i);
    abort();
}

OPTYPE op_out_dxal(struct decoded_instruction* i)
{
    UNUSED(i);
    abort();
}
OPTYPE op_out_dxax(struct decoded_instruction* i)
{
    UNUSED(i);
    abort();
}
OPTYPE op_out_dxeax(struct decoded_instruction* i)
{
    UNUSED(i);
    abort();
}
OPTYPE op_in_dxal(struct decoded_instruction* i)
{
    UNUSED(i);
    abort();
}
OPTYPE op_in_dxax(struct decoded_instruction* i)
{
    UNUSED(i);
    abort();
}
OPTYPE op_in_dxeax(struct decoded_instruction* i)
{
    UNUSED(i);
    abort();
}

// Data Transfer
OPTYPE op_mov_r8i8(struct decoded_instruction* i)
{
    int flags = i->flags;
    R8(I_RM(flags)) = i->imm8;
    NEXT(flags);
}
OPTYPE op_mov_r16i16(struct decoded_instruction* i)
{
    int flags = i->flags;
    R16(I_RM(flags)) = i->imm16;
    NEXT(flags);
}
OPTYPE op_mov_r32i32(struct decoded_instruction* i)
{
    int flags = i->flags;
    R32(I_RM(flags)) = i->imm32;
    NEXT(flags);
}
OPTYPE op_mov_r8e8(struct decoded_instruction* i)
{
    uint32_t flags = i->flags, linaddr = cpu_get_linaddr(flags, i);
    cpu_read8(linaddr, R8(I_REG(flags)), thread_cpu.tlb_shift_read);
    NEXT(flags);
}
OPTYPE op_mov_r8r8(struct decoded_instruction* i)
{
    uint32_t flags = i->flags;
    R8(I_RM(flags)) = R8(I_REG(flags));
    NEXT(flags);
}
OPTYPE op_mov_e8r8(struct decoded_instruction* i)
{
    uint32_t flags = i->flags, linaddr = cpu_get_linaddr(flags, i);
    cpu_write8(linaddr, R8(I_REG(flags)), thread_cpu.tlb_shift_write);
    NEXT(flags);
}
OPTYPE op_mov_e8i8(struct decoded_instruction* i)
{
    uint32_t flags = i->flags, linaddr = cpu_get_linaddr(flags, i);
    cpu_write8(linaddr, i->imm8, thread_cpu.tlb_shift_write);
    NEXT(flags);
}
OPTYPE op_mov_r16e16(struct decoded_instruction* i)
{
    uint32_t flags = i->flags, linaddr = cpu_get_linaddr(flags, i);
    cpu_read16(linaddr, R16(I_REG(flags)), thread_cpu.tlb_shift_read);
    NEXT(flags);
}
OPTYPE op_mov_r16r16(struct decoded_instruction* i)
{
    uint32_t flags = i->flags;
    R16(I_RM(flags)) = R16(I_REG(flags));
    NEXT(flags);
}
OPTYPE op_mov_e16r16(struct decoded_instruction* i)
{
    uint32_t flags = i->flags, linaddr = cpu_get_linaddr(flags, i);
    cpu_write16(linaddr, R16(I_REG(flags)), thread_cpu.tlb_shift_write);
    NEXT(flags);
}
OPTYPE op_mov_e16i16(struct decoded_instruction* i)
{
    uint32_t flags = i->flags, linaddr = cpu_get_linaddr(flags, i);
    cpu_write16(linaddr, i->imm16, thread_cpu.tlb_shift_write);
    NEXT(flags);
}
OPTYPE op_mov_r32e32(struct decoded_instruction* i)
{
    uint32_t flags = i->flags, linaddr = cpu_get_linaddr(flags, i);
    cpu_read32(linaddr, R32(I_REG(flags)), thread_cpu.tlb_shift_read);
    NEXT(flags);
}
OPTYPE op_mov_r32r32(struct decoded_instruction* i)
{
    uint32_t flags = i->flags;
    R32(I_RM(flags)) = R32(I_REG(flags));
    NEXT(flags);
}
OPTYPE op_mov_e32r32(struct decoded_instruction* i)
{
    uint32_t flags = i->flags, linaddr = cpu_get_linaddr(flags, i);
    cpu_write32(linaddr, R32(I_REG(flags)), thread_cpu.tlb_shift_write);
    NEXT(flags);
}
OPTYPE op_mov_e32i32(struct decoded_instruction* i)
{
    uint32_t flags = i->flags, linaddr = cpu_get_linaddr(flags, i);
    cpu_write32(linaddr, i->imm32, thread_cpu.tlb_shift_write);
    NEXT(flags);
}

OPTYPE op_mov_s16r16(struct decoded_instruction* i)
{
    int flags = i->flags, dest = I_REG(flags);
    if (cpu_load_seg_value_mov(dest, R16(I_RM(flags))))
        EXCEP();
    if (dest == SS)
        interrupt_guard();
    NEXT(flags);
}
OPTYPE op_mov_s16e16(struct decoded_instruction* i)
{
    uint32_t flags = i->flags, dest = I_REG(flags), linaddr = cpu_get_linaddr(flags, i);
    uint16_t src;
    cpu_read16(linaddr, src, thread_cpu.tlb_shift_read);
    if (cpu_load_seg_value_mov(dest, src))
        EXCEP();
    if (dest == SS)
        interrupt_guard();
    NEXT(flags);
}
OPTYPE op_mov_e16s16(struct decoded_instruction* i)
{
    uint32_t flags = i->flags, linaddr = cpu_get_linaddr(flags, i);
    cpu_write16(linaddr, thread_cpu.seg[I_REG(flags)], thread_cpu.tlb_shift_write);
    NEXT(flags);
}
OPTYPE op_mov_r16s16(struct decoded_instruction* i)
{
    uint32_t flags = i->flags;
    R16(I_RM(flags)) = thread_cpu.seg[I_REG(flags)];
    NEXT(flags);
}
OPTYPE op_mov_r32s16(struct decoded_instruction* i)
{
    uint32_t flags = i->flags;
    R32(I_RM(flags)) = thread_cpu.seg[I_REG(flags)];
    NEXT(flags);
}

OPTYPE op_mov_eaxm32(struct decoded_instruction* i)
{
    int flags = i->flags;
    cpu_read32(i->imm32 + thread_cpu.seg_base[I_SEG_BASE(flags)], thread_cpu.reg32[EAX], thread_cpu.tlb_shift_read);
    NEXT(flags);
}
OPTYPE op_mov_axm16(struct decoded_instruction* i)
{
    int flags = i->flags;
    cpu_read16(i->imm32 + thread_cpu.seg_base[I_SEG_BASE(flags)], thread_cpu.reg16[AX], thread_cpu.tlb_shift_read);
    NEXT(flags);
}
OPTYPE op_mov_alm8(struct decoded_instruction* i)
{
    int flags = i->flags;
    cpu_read8(i->imm32 + thread_cpu.seg_base[I_SEG_BASE(flags)], thread_cpu.reg8[EAX], thread_cpu.tlb_shift_read);
    NEXT(flags);
}
OPTYPE op_mov_m32eax(struct decoded_instruction* i)
{
    int flags = i->flags;
    cpu_write32(i->imm32 + thread_cpu.seg_base[I_SEG_BASE(flags)], thread_cpu.reg32[EAX], thread_cpu.tlb_shift_write);
    NEXT(flags);
}
OPTYPE op_mov_m16ax(struct decoded_instruction* i)
{
    int flags = i->flags;
    cpu_write16(i->imm32 + thread_cpu.seg_base[I_SEG_BASE(flags)], thread_cpu.reg16[AX], thread_cpu.tlb_shift_write);
    NEXT(flags);
}
OPTYPE op_mov_m8al(struct decoded_instruction* i)
{
    int flags = i->flags;
    cpu_write8(i->imm32 + thread_cpu.seg_base[I_SEG_BASE(flags)], thread_cpu.reg8[AX], thread_cpu.tlb_shift_write);
    NEXT(flags);
}

// Conditional moves access memory, regardless of whether condition is true or not.
OPTYPE op_cmov_r16e16(struct decoded_instruction* i)
{
    int flags = i->flags, linaddr = cpu_get_linaddr(flags, i), dest;
    cpu_read16(linaddr, dest, thread_cpu.tlb_shift_read);
    if (cpu_cond(I_OP3(flags)))
        R16(I_REG(flags)) = dest;
    NEXT(flags);
}
OPTYPE op_cmov_r16r16(struct decoded_instruction* i)
{
    int flags = i->flags;
    if (cpu_cond(I_OP3(flags)))
        R16(I_REG(flags)) = R16(I_RM(flags));
    NEXT(flags);
}
OPTYPE op_cmov_r32e32(struct decoded_instruction* i)
{
    int flags = i->flags, linaddr = cpu_get_linaddr(flags, i), dest;
    cpu_read32(linaddr, dest, thread_cpu.tlb_shift_read);
    if (cpu_cond(I_OP3(flags)))
        R32(I_REG(flags)) = dest;
    NEXT(flags);
}
OPTYPE op_cmov_r32r32(struct decoded_instruction* i)
{
    int flags = i->flags;
    if (cpu_cond(I_OP3(flags)))
        R32(I_REG(flags)) = R32(I_RM(flags));
    NEXT(flags);
}
OPTYPE op_setcc_e8(struct decoded_instruction* i)
{
    int flags = i->flags, linaddr = cpu_get_linaddr(flags, i);
    cpu_write8(linaddr, cpu_cond(I_OP3(flags)), thread_cpu.tlb_shift_write);
    NEXT(flags);
}
OPTYPE op_setcc_r8(struct decoded_instruction* i)
{
    int flags = i->flags;
    R8(I_RM(flags)) = cpu_cond(I_OP3(flags));
    NEXT(flags);
}

OPTYPE op_lea_r16e16(struct decoded_instruction* i)
{
    uint32_t flags = i->flags;
    R16(I_REG(flags)) = cpu_get_virtaddr(flags, i);
    NEXT(flags);
}
OPTYPE op_lea_r32e32(struct decoded_instruction* i)
{
    uint32_t flags = i->flags;
    R32(I_REG(flags)) = cpu_get_virtaddr(flags, i);
    NEXT(flags);
}

OPTYPE op_lds_r16e16(struct decoded_instruction* i)
{
    uint32_t flags = i->flags, linaddr = cpu_get_linaddr(flags, i), data;
    cpu_read16(linaddr + 2, data, thread_cpu.tlb_shift_read);
    if (cpu_load_seg_value_mov(DS, data))
        EXCEP();
    cpu_read16(linaddr, data, thread_cpu.tlb_shift_read);
    R16(I_REG(flags)) = data;
    NEXT(flags);
}
OPTYPE op_lds_r32e32(struct decoded_instruction* i)
{
    uint32_t flags = i->flags, linaddr = cpu_get_linaddr(flags, i), data;
    cpu_read16(linaddr + 4, data, thread_cpu.tlb_shift_read);
    if (cpu_load_seg_value_mov(DS, data))
        EXCEP();
    cpu_read32(linaddr, data, thread_cpu.tlb_shift_read);
    R32(I_REG(flags)) = data;
    NEXT(flags);
}
OPTYPE op_les_r16e16(struct decoded_instruction* i)
{
    uint32_t flags = i->flags, linaddr = cpu_get_linaddr(flags, i), data;
    cpu_read16(linaddr + 2, data, thread_cpu.tlb_shift_read);
    if (cpu_load_seg_value_mov(ES, data))
        EXCEP();
    cpu_read16(linaddr, data, thread_cpu.tlb_shift_read);
    R16(I_REG(flags)) = data;
    NEXT(flags);
}
OPTYPE op_les_r32e32(struct decoded_instruction* i)
{
    uint32_t flags = i->flags, linaddr = cpu_get_linaddr(flags, i), data;
    cpu_read16(linaddr + 4, data, thread_cpu.tlb_shift_read);
    if (cpu_load_seg_value_mov(ES, data))
        EXCEP();
    cpu_read32(linaddr, data, thread_cpu.tlb_shift_read);
    R32(I_REG(flags)) = data;
    NEXT(flags);
}
OPTYPE op_lss_r16e16(struct decoded_instruction* i)
{
    uint32_t flags = i->flags, linaddr = cpu_get_linaddr(flags, i), data;
    cpu_read16(linaddr + 2, data, thread_cpu.tlb_shift_read);
    if (cpu_load_seg_value_mov(SS, data))
        EXCEP();
    cpu_read16(linaddr, data, thread_cpu.tlb_shift_read);
    R16(I_REG(flags)) = data;
    NEXT(flags);
}
OPTYPE op_lss_r32e32(struct decoded_instruction* i)
{
    uint32_t flags = i->flags, linaddr = cpu_get_linaddr(flags, i), data;
    cpu_read16(linaddr + 4, data, thread_cpu.tlb_shift_read);
    if (cpu_load_seg_value_mov(SS, data))
        EXCEP();
    cpu_read32(linaddr, data, thread_cpu.tlb_shift_read);
    R32(I_REG(flags)) = data;
    NEXT(flags);
}
OPTYPE op_lfs_r16e16(struct decoded_instruction* i)
{
    uint32_t flags = i->flags, linaddr = cpu_get_linaddr(flags, i), data;
    cpu_read16(linaddr + 2, data, thread_cpu.tlb_shift_read);
    if (cpu_load_seg_value_mov(FS, data))
        EXCEP();
    cpu_read16(linaddr, data, thread_cpu.tlb_shift_read);
    R16(I_REG(flags)) = data;
    NEXT(flags);
}
OPTYPE op_lfs_r32e32(struct decoded_instruction* i)
{
    uint32_t flags = i->flags, linaddr = cpu_get_linaddr(flags, i), data;
    cpu_read16(linaddr + 4, data, thread_cpu.tlb_shift_read);
    if (cpu_load_seg_value_mov(FS, data))
        EXCEP();
    cpu_read32(linaddr, data, thread_cpu.tlb_shift_read);
    R32(I_REG(flags)) = data;
    NEXT(flags);
}
OPTYPE op_lgs_r16e16(struct decoded_instruction* i)
{
    uint32_t flags = i->flags, linaddr = cpu_get_linaddr(flags, i), data;
    cpu_read16(linaddr + 2, data, thread_cpu.tlb_shift_read);
    if (cpu_load_seg_value_mov(GS, data))
        EXCEP();
    cpu_read16(linaddr, data, thread_cpu.tlb_shift_read);
    R16(I_REG(flags)) = data;
    NEXT(flags);
}
OPTYPE op_lgs_r32e32(struct decoded_instruction* i)
{
    uint32_t flags = i->flags, linaddr = cpu_get_linaddr(flags, i), data;
    cpu_read16(linaddr + 4, data, thread_cpu.tlb_shift_read);
    if (cpu_load_seg_value_mov(GS, data))
        EXCEP();
    cpu_read32(linaddr, data, thread_cpu.tlb_shift_read);
    R32(I_REG(flags)) = data;
    NEXT(flags);
}

OPTYPE op_xchg_r8r8(struct decoded_instruction* i)
{
    uint32_t flags = i->flags;
    uint8_t temp = R8(I_RM(flags));
    R8(I_RM(flags)) = R8(I_REG(flags));
    R8(I_REG(flags)) = temp;
    NEXT(flags);
}
OPTYPE op_xchg_r16r16(struct decoded_instruction* i)
{
    uint32_t flags = i->flags;
    uint16_t temp = R16(I_RM(flags));
    R16(I_RM(flags)) = R16(I_REG(flags));
    R16(I_REG(flags)) = temp;
    NEXT(flags);
}
OPTYPE op_xchg_r32r32(struct decoded_instruction* i)
{
    uint32_t flags = i->flags;
    uint32_t temp = R32(I_RM(flags));
    R32(I_RM(flags)) = R32(I_REG(flags));
    R32(I_REG(flags)) = temp;
    NEXT(flags);
}
OPTYPE op_xchg_r8e8(struct decoded_instruction* i)
{
    uint32_t flags = i->flags, linaddr = cpu_get_linaddr(flags, i);
    int tlb_info = thread_cpu.tlb_tags[linaddr >> 12];
    uint8_t* ptr;
    if (TLB_ENTRY_INVALID8(linaddr, tlb_info, thread_cpu.tlb_shift_write)) {
        if (cpu_access_read8(linaddr, tlb_info, thread_cpu.tlb_shift_write))
            EXCEP();
        UNUSED2(cpu_access_write8(linaddr, R8(I_REG(flags)), tlb_info, thread_cpu.tlb_shift_write));
        R8(I_REG(flags)) = thread_cpu.read_result;
    } else {
        ptr = thread_cpu.tlb[linaddr >> 12] + linaddr;
        uint8_t tmp = *ptr;
        *ptr = R8(I_REG(flags));
        R8(I_REG(flags)) = tmp;
    }
    NEXT(flags);
}
OPTYPE op_xchg_r16e16(struct decoded_instruction* i)
{
    uint32_t flags = i->flags, linaddr = cpu_get_linaddr(flags, i);
    int tlb_info = thread_cpu.tlb_tags[linaddr >> 12];
    uint16_t* ptr;
    if (TLB_ENTRY_INVALID16(linaddr, tlb_info, thread_cpu.tlb_shift_write)) {
        tlb_info >>= thread_cpu.tlb_shift_write;
        if (cpu_access_read16(linaddr, tlb_info, thread_cpu.tlb_shift_write))
            EXCEP();
        UNUSED2(cpu_access_write16(linaddr, R16(I_REG(flags)), tlb_info, thread_cpu.tlb_shift_write));
        R16(I_REG(flags)) = thread_cpu.read_result;
    } else {
        ptr = thread_cpu.tlb[linaddr >> 12] + linaddr;
        uint16_t tmp = *ptr;
        *ptr = R16(I_REG(flags));
        R16(I_REG(flags)) = tmp;
    }
    NEXT(flags);
}
OPTYPE op_xchg_r32e32(struct decoded_instruction* i)
{
    uint32_t flags = i->flags, linaddr = cpu_get_linaddr(flags, i);
    int tlb_info = thread_cpu.tlb_tags[linaddr >> 12];
    uint32_t* ptr;
    if (TLB_ENTRY_INVALID32(linaddr, tlb_info, thread_cpu.tlb_shift_write)) {
        tlb_info >>= thread_cpu.tlb_shift_write;
        if (cpu_access_read32(linaddr, tlb_info, thread_cpu.tlb_shift_write))
            EXCEP();
        UNUSED2(cpu_access_write32(linaddr, R32(I_REG(flags)), tlb_info, thread_cpu.tlb_shift_write));
        R32(I_REG(flags)) = thread_cpu.read_result;
    } else {
        ptr = thread_cpu.tlb[linaddr >> 12] + linaddr;
        uint32_t tmp = *ptr;
        *ptr = R32(I_REG(flags));
        R32(I_REG(flags)) = tmp;
    }
    NEXT(flags);
}
OPTYPE op_cmpxchg_r8r8(struct decoded_instruction* i)
{
    uint32_t flags = i->flags;
    cpu_cmpxchg8(&R8(I_RM(flags)), R8(I_REG(flags)));
    NEXT(flags);
}
OPTYPE op_cmpxchg_e8r8(struct decoded_instruction* i)
{
    arith_rmw2(8, cpu_cmpxchg8, R8(I_REG(flags)));
}
OPTYPE op_cmpxchg_r16r16(struct decoded_instruction* i)
{
    uint32_t flags = i->flags;
    cpu_cmpxchg16(&R16(I_RM(flags)), R16(I_REG(flags)));
    NEXT(flags);
}
OPTYPE op_cmpxchg_e16r16(struct decoded_instruction* i)
{
    arith_rmw2(16, cpu_cmpxchg16, R16(I_REG(flags)));
}
OPTYPE op_cmpxchg_r32r32(struct decoded_instruction* i)
{
    uint32_t flags = i->flags;
    cpu_cmpxchg32(&R32(I_RM(flags)), R32(I_REG(flags)));
    NEXT(flags);
}
OPTYPE op_cmpxchg_e32r32(struct decoded_instruction* i)
{
    arith_rmw2(32, cpu_cmpxchg32, R32(I_REG(flags)));
}
OPTYPE op_cmpxchg8b_e32(struct decoded_instruction* i)
{
    uint32_t flags = i->flags, low64, high64, linaddr = cpu_get_linaddr(flags, i);
    cpu_read32(linaddr, low64, thread_cpu.tlb_shift_write);
    cpu_read32(linaddr + 4, high64, thread_cpu.tlb_shift_write);
    if (thread_cpu.reg32[EAX] == low64 && thread_cpu.reg32[EDX] == high64) {
        cpu_set_zf(1);
        cpu_write32(linaddr, thread_cpu.reg32[EBX], thread_cpu.tlb_shift_write);
        cpu_write32(linaddr + 4, thread_cpu.reg32[ECX], thread_cpu.tlb_shift_write);
    } else {
        cpu_set_zf(0);
        thread_cpu.reg32[EAX] = low64;
        thread_cpu.reg32[EDX] = high64;
    }
    NEXT(flags);
}

OPTYPE op_xadd_r8r8(struct decoded_instruction* i)
{
    uint32_t flags = i->flags;
    xadd8(&R8(I_RM(flags)), &R8(I_REG(flags)));
    NEXT(flags);
}
OPTYPE op_xadd_r8e8(struct decoded_instruction* i)
{
    arith_rmw2(8, xadd8, &R8(I_REG(flags)));
}
OPTYPE op_xadd_r16r16(struct decoded_instruction* i)
{
    uint32_t flags = i->flags;
    xadd16(&R16(I_RM(flags)), &R16(I_REG(flags)));
    NEXT(flags);
}
OPTYPE op_xadd_r16e16(struct decoded_instruction* i)
{
    arith_rmw2(16, xadd16, &R16(I_REG(flags)));
}
OPTYPE op_xadd_r32r32(struct decoded_instruction* i)
{
    uint32_t flags = i->flags;
    xadd32(&R32(I_RM(flags)), &R32(I_REG(flags)));
    NEXT(flags);
}
OPTYPE op_xadd_r32e32(struct decoded_instruction* i)
{
    arith_rmw2(32, xadd32, &R32(I_REG(flags)));
}

OPTYPE op_bound_r16e16(struct decoded_instruction* i)
{
    uint32_t flags = i->flags, linaddr = cpu_get_linaddr(flags, i);
    int16_t index16 = R16(I_REG(flags)), low, hi;
    cpu_read16(linaddr, low, thread_cpu.tlb_shift_read);
    cpu_read16(linaddr + 2, hi, thread_cpu.tlb_shift_read);
    if (index16 < low || index16 > hi)
        EXCEPTION(5);
    NEXT(flags);
}
OPTYPE op_bound_r32e32(struct decoded_instruction* i)
{
    uint32_t flags = i->flags, linaddr = cpu_get_linaddr(flags, i);
    int32_t index32 = R32(I_REG(flags)), low, hi;
    cpu_read32(linaddr, low, thread_cpu.tlb_shift_read);
    cpu_read32(linaddr + 4, hi, thread_cpu.tlb_shift_read);
    if (index32 < low || index32 > hi)
        EXCEPTION(5);
    NEXT(flags);
}

// BCD operations
OPTYPE op_daa(struct decoded_instruction* i)
{
    uint8_t old_al = thread_cpu.reg8[AL], old_cf = cpu_get_cf(), al = old_al /*, cf = 0*/;
    int cond = (al & 0x0F) > 9 || cpu_get_af();
    if (cond) {
        al += 6; // Note: wraps around
        //cf = old_cf | (al < old_al); // (255 + 6) & 0xFF < 255
    }
    cpu_set_af(cond);

    cond = (old_al > 0x99) || old_cf;
    if (cond)
        al += 0x60; // Note: wraps around
    cpu_set_cf(cond);

    // Set SZP flags
    thread_cpu.lr = (int8_t)al;
    thread_cpu.reg8[AL] = al;
    NEXT2(i->flags);
}
OPTYPE op_das(struct decoded_instruction* i)
{
    uint8_t old_al = thread_cpu.reg8[AL], old_cf = cpu_get_cf(), cf = 0, al = old_al;

    int cond = (old_al & 0x0F) > 9 || cpu_get_af();
    if (cond) {
        al -= 6; // Note: wraps around (5 --> 255)
        cf = old_cf | (al > old_al);
    }
    cpu_set_af(cond);

    if (old_al > 0x99 || old_cf == 1) {
        al -= 0x60;
        cf = 1;
    }
    cpu_set_cf(cf);
    thread_cpu.lr = (int8_t)al;
    thread_cpu.reg8[AL] = al;
    NEXT2(i->flags);
}
OPTYPE op_aaa(struct decoded_instruction* i)
{
    int cond = 0;
    if ((thread_cpu.reg8[AL] & 15) > 9 || cpu_get_af()) {
        thread_cpu.reg16[AX] += 0x106;
        cond = 1;
    }
    thread_cpu.reg8[AL] &= 15;
    thread_cpu.laux = BIT;
    thread_cpu.lr = (int8_t)thread_cpu.reg8[AL];
    cpu_set_af(cond);
    cpu_set_cf(cond);
    NEXT2(i->flags);
}
OPTYPE op_aas(struct decoded_instruction* i)
{
    int cond = (thread_cpu.reg8[AL] & 0x0F) > 9 || cpu_get_af();
    if (cond) {
        thread_cpu.reg16[AX] -= 6;
        thread_cpu.reg8[AH] -= 1;
    }
    thread_cpu.reg8[AL] &= 0x0F;
    thread_cpu.laux = BIT;
    thread_cpu.lr = (int8_t)thread_cpu.reg8[AL];
    cpu_set_af(cond);
    cpu_set_cf(cond);
    NEXT2(i->flags);
}
OPTYPE op_aam(struct decoded_instruction* i)
{
    if (i->imm8 == 0)
        EXCEPTION_DE();
    uint8_t temp_al = thread_cpu.reg8[AL];
    thread_cpu.reg8[AH] = temp_al / i->imm8;
    thread_cpu.lr = (int8_t)(thread_cpu.reg8[AL] = temp_al % i->imm8);
    thread_cpu.laux = BIT;
    NEXT2(i->flags);
}
OPTYPE op_aad(struct decoded_instruction* i)
{
    uint8_t temp_al = thread_cpu.reg8[AL];
    uint8_t temp_ah = thread_cpu.reg8[AH];
    thread_cpu.lr = (int8_t)(thread_cpu.reg8[AL] = ((temp_al + (temp_ah * i->imm8)) & 0xFF));
    thread_cpu.reg8[AH] = 0;
    thread_cpu.laux = BIT; // This is the simplest option -- set it to anything besides EFLAGS_FULL_UPDATE
    NEXT2(i->flags);
}

// Bit test and <action> operation
// <<< BEGIN AUTOGENERATE "bit" >>>
// Auto-generated on Thu Oct 03 2019 14:47:14 GMT-0700 (PDT)
OPTYPE op_bt_r16(struct decoded_instruction* i)
{
    uint32_t flags = i->flags;
    bt16(R16(I_RM(flags)), (R16(I_REG(flags)) & i->disp16) + i->imm8);
    NEXT(flags);
}
OPTYPE op_bts_r16(struct decoded_instruction* i)
{
    uint32_t flags = i->flags;
    bts16(&R16(I_RM(flags)), (R16(I_REG(flags)) & i->disp16) + i->imm8);
    NEXT(flags);
}
OPTYPE op_btc_r16(struct decoded_instruction* i)
{
    uint32_t flags = i->flags;
    btc16(&R16(I_RM(flags)), (R16(I_REG(flags)) & i->disp16) + i->imm8);
    NEXT(flags);
}
OPTYPE op_btr_r16(struct decoded_instruction* i)
{
    uint32_t flags = i->flags;
    btr16(&R16(I_RM(flags)), (R16(I_REG(flags)) & i->disp16) + i->imm8);
    NEXT(flags);
}
OPTYPE op_bt_r32(struct decoded_instruction* i)
{
    uint32_t flags = i->flags;
    bt32(R32(I_RM(flags)), (R32(I_REG(flags)) & i->disp32) + i->imm8);
    NEXT(flags);
}
OPTYPE op_bts_r32(struct decoded_instruction* i)
{
    uint32_t flags = i->flags;
    bts32(&R32(I_RM(flags)), (R32(I_REG(flags)) & i->disp32) + i->imm8);
    NEXT(flags);
}
OPTYPE op_btc_r32(struct decoded_instruction* i)
{
    uint32_t flags = i->flags;
    btc32(&R32(I_RM(flags)), (R32(I_REG(flags)) & i->disp32) + i->imm8);
    NEXT(flags);
}
OPTYPE op_btr_r32(struct decoded_instruction* i)
{
    uint32_t flags = i->flags;
    btr32(&R32(I_RM(flags)), (R32(I_REG(flags)) & i->disp32) + i->imm8);
    NEXT(flags);
}
OPTYPE op_bt_e16(struct decoded_instruction* i)
{
    uint32_t flags = i->flags, x = I_OP2(flags) ? i->imm8 : R16(I_REG(flags)), linaddr = cpu_get_linaddr(flags, i), dest;
    cpu_read16(linaddr + ((x / 16) * 2), dest, thread_cpu.tlb_shift_read);
    bt16(dest, x);
    NEXT(flags);
}
OPTYPE op_bt_e32(struct decoded_instruction* i)
{
    uint32_t flags = i->flags, x = I_OP2(flags) ? i->imm8 : R32(I_REG(flags)), linaddr = cpu_get_linaddr(flags, i), dest;
    cpu_read32(linaddr + ((x / 32) * 4), dest, thread_cpu.tlb_shift_read);
    bt32(dest, x);
    NEXT(flags);
}
OPTYPE op_bts_e16(struct decoded_instruction* i)
{
    uint32_t x = I_OP2(i->flags) ? i->imm8 : R16(I_REG(i->flags));
    arith_rmw3(16, bts16, ((x / 16) * 2), x);
    NEXT(flags);
}
OPTYPE op_btc_e16(struct decoded_instruction* i)
{
    uint32_t x = I_OP2(i->flags) ? i->imm8 : R16(I_REG(i->flags));
    arith_rmw3(16, btc16, ((x / 16) * 2), x);
    NEXT(flags);
}
OPTYPE op_btr_e16(struct decoded_instruction* i)
{
    uint32_t x = I_OP2(i->flags) ? i->imm8 : R16(I_REG(i->flags));
    arith_rmw3(16, btr16, ((x / 16) * 2), x);
    NEXT(flags);
}
OPTYPE op_bts_e32(struct decoded_instruction* i)
{
    uint32_t x = I_OP2(i->flags) ? i->imm8 : R32(I_REG(i->flags));
    arith_rmw3(32, bts32, ((x / 32) * 4), x);
    NEXT(flags);
}
OPTYPE op_btc_e32(struct decoded_instruction* i)
{
    uint32_t x = I_OP2(i->flags) ? i->imm8 : R32(I_REG(i->flags));
    arith_rmw3(32, btc32, ((x / 32) * 4), x);
    NEXT(flags);
}
OPTYPE op_btr_e32(struct decoded_instruction* i)
{
    uint32_t x = I_OP2(i->flags) ? i->imm8 : R32(I_REG(i->flags));
    arith_rmw3(32, btr32, ((x / 32) * 4), x);
    NEXT(flags);
}

// <<< END AUTOGENERATE "bit" >>>

OPTYPE op_bsf_r16r16(struct decoded_instruction* i)
{
    int flags = i->flags;
    R16(I_REG(flags)) = bsf16(R16(I_RM(flags)), R16(I_REG(flags)));
    NEXT(flags);
}
OPTYPE op_bsf_r16e16(struct decoded_instruction* i)
{
    int flags = i->flags, linaddr = cpu_get_linaddr(flags, i), data;
    cpu_read16(linaddr, data, thread_cpu.tlb_shift_read);
    R16(I_REG(flags)) = bsf16(data, R16(I_REG(flags)));
    NEXT(flags);
}
OPTYPE op_bsf_r32r32(struct decoded_instruction* i)
{
    int flags = i->flags;
    R32(I_REG(flags)) = bsf32(R32(I_RM(flags)), R32(I_REG(flags)));
    NEXT(flags);
}
OPTYPE op_bsf_r32e32(struct decoded_instruction* i)
{
    int flags = i->flags, linaddr = cpu_get_linaddr(flags, i), data;
    cpu_read32(linaddr, data, thread_cpu.tlb_shift_read);
    R32(I_REG(flags)) = bsf32(data, R32(I_REG(flags)));
    NEXT(flags);
}
OPTYPE op_bsr_r16r16(struct decoded_instruction* i)
{
    int flags = i->flags;
    R16(I_REG(flags)) = bsr16(R16(I_RM(flags)), R16(I_REG(flags)));
    NEXT(flags);
}
OPTYPE op_bsr_r16e16(struct decoded_instruction* i)
{
    int flags = i->flags, linaddr = cpu_get_linaddr(flags, i), data;
    cpu_read16(linaddr, data, thread_cpu.tlb_shift_read);
    R16(I_REG(flags)) = bsr16(data, R16(I_REG(flags)));
    NEXT(flags);
}
OPTYPE op_bsr_r32r32(struct decoded_instruction* i)
{
    int flags = i->flags;
    R32(I_REG(flags)) = bsr32(R32(I_RM(flags)), R32(I_REG(flags)));
    NEXT(flags);
}
OPTYPE op_bsr_r32e32(struct decoded_instruction* i)
{
    int flags = i->flags, linaddr = cpu_get_linaddr(flags, i), data;
    cpu_read32(linaddr, data, thread_cpu.tlb_shift_read);
    R32(I_REG(flags)) = bsr32(data, R32(I_REG(flags)));
    NEXT(flags);
}

// Miscellaneous operations
OPTYPE op_cli(struct decoded_instruction* i)
{
    // Basic operation: https://pdos.csail.mit.edu/6.828/2006/readings/i386/CLI.htm
    // VME operation: https://www.felixcloutier.com/x86/cli
    if ((unsigned int)thread_cpu.cpl > get_iopl()) {
        if (thread_cpu.cr[4] & CR4_VME)
            thread_cpu.eflags &= ~EFLAGS_VIF;
        else
            EXCEPTION_GP(0);
    } else
        thread_cpu.eflags &= ~EFLAGS_IF;
    NEXT2(i->flags);
}
OPTYPE op_sti(struct decoded_instruction* i)
{
    // https://pdos.csail.mit.edu/6.828/2006/readings/i386/STI.htm
    // VME: https://www.felixcloutier.com/x86/sti
    if ((unsigned int)thread_cpu.cpl > get_iopl()) {
        if (thread_cpu.cr[4] & CR4_VME && !(thread_cpu.eflags & EFLAGS_VIP))
            thread_cpu.eflags |= EFLAGS_VIF;
        else
            EXCEPTION_GP(0);
    } else
        thread_cpu.eflags |= EFLAGS_IF;

    // Check if there are interrupts pending, and if so enable interrupts after the next instruction
    interrupt_guard();
    NEXT2(i->flags);
}
OPTYPE op_cld(struct decoded_instruction* i)
{
    thread_cpu.eflags &= ~EFLAGS_DF;
    NEXT2(i->flags);
}
OPTYPE op_std(struct decoded_instruction* i)
{
    thread_cpu.eflags |= EFLAGS_DF;
    NEXT2(i->flags);
}
OPTYPE op_cmc(struct decoded_instruction* i)
{
    cpu_set_cf(cpu_get_cf() ^ 1);
    NEXT2(i->flags);
}
OPTYPE op_clc(struct decoded_instruction* i)
{
    cpu_set_cf(0);
    NEXT2(i->flags);
}
OPTYPE op_stc(struct decoded_instruction* i)
{
    cpu_set_cf(1);
    NEXT2(i->flags);
}
OPTYPE op_hlt(struct decoded_instruction* i)
{
    if (thread_cpu.cpl != 0)
        EXCEPTION_GP(0);

    // Request a fast return from the CPU loop.
    thread_cpu.cycles += cpu_get_cycles() - thread_cpu.cycles;
    thread_cpu.cycles_to_run = 1;
    thread_cpu.cycle_offset = 1;
    thread_cpu.refill_counter = 0;
    // Don't block interrupts if STI has just been called (OS/2 does this)
    thread_cpu.interrupts_blocked = 0;

    thread_cpu.exit_reason = EXIT_STATUS_HLT;
    if ((thread_cpu.eflags & EFLAGS_IF) == 0)
        CPU_LOG("HLT called with IF=0");
    NEXT2(i->flags);
}
OPTYPE op_cpuid(struct decoded_instruction* i)
{
    cpuid();
    NEXT2(i->flags);
}
OPTYPE op_rdmsr(struct decoded_instruction* i)
{
    if (rdmsr(thread_cpu.reg32[ECX], &thread_cpu.reg32[EDX], &thread_cpu.reg32[EAX]))
        EXCEP();
    NEXT2(i->flags);
}
OPTYPE op_wrmsr(struct decoded_instruction* i)
{
    if (wrmsr(thread_cpu.reg32[ECX], thread_cpu.reg32[EDX], thread_cpu.reg32[EAX]))
        EXCEP();
    NEXT2(i->flags);
}
OPTYPE op_rdtsc(struct decoded_instruction* i)
{
    if (!(thread_cpu.cr[4] & CR4_TSD) || (thread_cpu.cpl == 0) || !(thread_cpu.cr[0] & CR0_PE)) {
        uint64_t tsc = cpu_get_cycles() - thread_cpu.tsc_fudge;
        thread_cpu.reg32[EAX] = tsc;
        thread_cpu.reg32[EDX] = tsc >> 32;
//halfix.reg32[EAX] = halfix.cycles & 0xFFFFFFFF;
//halfix.reg32[EDX] = halfix.cycles >> 32L & 0xFFFFFFFF;
#ifdef INSTRUMENT
        cpu_instrument_rdtsc(halfix.reg32[EAX], halfix.reg32[EDX]);
#endif
    } else
        EXCEPTION_GP(0);
    NEXT2(i->flags);
}
OPTYPE op_pushf(struct decoded_instruction* i)
{
    if (pushf())
        EXCEP();
    NEXT2(i->flags);
}
OPTYPE op_pushfd(struct decoded_instruction* i)
{
    if (pushfd())
        EXCEP();
    NEXT2(i->flags);
}
OPTYPE op_popf(struct decoded_instruction* i)
{
    if (popf())
        EXCEP();
    NEXT2(i->flags);
}
OPTYPE op_popfd(struct decoded_instruction* i)
{
    if (popfd())
        EXCEP();
    NEXT2(i->flags);
}
OPTYPE op_cbw(struct decoded_instruction* i)
{
    thread_cpu.reg16[AX] = (int8_t)thread_cpu.reg8[AL];
    NEXT2(i->flags);
}
OPTYPE op_cwde(struct decoded_instruction* i)
{
    thread_cpu.reg32[EAX] = (int16_t)thread_cpu.reg16[AX];
    NEXT2(i->flags);
}
OPTYPE op_cwd(struct decoded_instruction* i)
{
    thread_cpu.reg16[DX] = (int16_t)thread_cpu.reg16[AX] >> 15;
    NEXT2(i->flags);
}
OPTYPE op_cdq(struct decoded_instruction* i)
{
    thread_cpu.reg32[EDX] = (int32_t)thread_cpu.reg32[EAX] >> 31;
    NEXT2(i->flags);
}
OPTYPE op_lahf(struct decoded_instruction* i)
{
    thread_cpu.reg8[AH] = cpu_get_eflags();
    NEXT2(i->flags);
}
OPTYPE op_sahf(struct decoded_instruction* i)
{
    // TODO: (cpu_get_of() * EFLAGS_OF) | (halfix.eflags & ~0xFF)
    // That would make this opcode more efficient
    cpu_set_eflags(thread_cpu.reg8[AH] | (cpu_get_eflags() & ~0xFF));
    NEXT2(i->flags);
}
OPTYPE op_enter16(struct decoded_instruction* i)
{
    uint16_t alloc_size = i->imm16;
    uint8_t nesting_level = i->disp8;
    uint32_t frame_temp, ebp, res;

    nesting_level &= 0x1F;
    push16(thread_cpu.reg16[BP]);
    frame_temp = thread_cpu.reg16[SP];
    if (nesting_level != 0) {
        ebp = thread_cpu.reg32[EBP];
        while (nesting_level > 1) {
            ebp = ((ebp - 2) & thread_cpu.esp_mask) | (thread_cpu.reg32[EBP] & ~thread_cpu.esp_mask);
            res = 0;
            cpu_read16(thread_cpu.reg16[BP] + thread_cpu.seg_base[SS], res, thread_cpu.tlb_shift_read);
            push16(res);
            nesting_level--;
        }
        push16(frame_temp);
    }
    thread_cpu.reg16[BP] = frame_temp;
    thread_cpu.reg16[SP] -= alloc_size;
    NEXT2(i->flags);
}
OPTYPE op_enter32(struct decoded_instruction* i)
{
    uint16_t alloc_size = i->imm16;
    uint8_t nesting_level = i->disp8;
    uint32_t frame_temp, ebp, res;

    nesting_level &= 0x1F;
    push32(thread_cpu.reg32[EBP]);
    frame_temp = thread_cpu.reg32[ESP];
    if (nesting_level != 0) {
        ebp = thread_cpu.reg32[EBP];
        while (nesting_level > 1) {
            ebp = ((ebp - 4) & thread_cpu.esp_mask) | (thread_cpu.reg32[EBP] & ~thread_cpu.esp_mask);
            res = 0;
            cpu_read32(thread_cpu.reg32[EBP] + thread_cpu.seg_base[SS], res, thread_cpu.tlb_shift_read);
            push32(res);
            nesting_level--;
        }
        push32(frame_temp);
    }
    thread_cpu.reg32[EBP] = frame_temp;
    thread_cpu.reg32[ESP] -= alloc_size;
    NEXT2(i->flags);
}
OPTYPE op_leave16(struct decoded_instruction* i)
{
    uint32_t ebp = thread_cpu.reg32[EBP], ss_ebp = (ebp & thread_cpu.esp_mask) + thread_cpu.seg_base[SS];
    cpu_read16(ss_ebp, thread_cpu.reg16[BP], thread_cpu.tlb_shift_read);
    thread_cpu.reg32[ESP] = ((ebp + 2) & thread_cpu.esp_mask) | (thread_cpu.reg32[ESP] & ~thread_cpu.esp_mask);
    NEXT2(i->flags);
}
OPTYPE op_leave32(struct decoded_instruction* i)
{
    uint32_t ebp = thread_cpu.reg32[EBP], ss_ebp = (ebp & thread_cpu.esp_mask) + thread_cpu.seg_base[SS];
    cpu_read32(ss_ebp, thread_cpu.reg32[EBP], thread_cpu.tlb_shift_read);
    thread_cpu.reg32[ESP] = ((ebp + 4) & thread_cpu.esp_mask) | (thread_cpu.reg32[ESP] & ~thread_cpu.esp_mask);
    NEXT2(i->flags);
}

// Protected mode opcodes
OPTYPE op_sgdt_e32(struct decoded_instruction* i)
{
    uint32_t flags = i->flags, linaddr = cpu_get_linaddr(flags, i);
    cpu_write16(linaddr, thread_cpu.seg_limit[SEG_GDTR], thread_cpu.tlb_shift_write);
    cpu_write32(linaddr + 2, thread_cpu.seg_base[SEG_GDTR], thread_cpu.tlb_shift_write);
    NEXT(flags);
}
OPTYPE op_sidt_e32(struct decoded_instruction* i)
{
    uint32_t flags = i->flags, linaddr = cpu_get_linaddr(flags, i);
    cpu_write16(linaddr, thread_cpu.seg_limit[SEG_IDTR], thread_cpu.tlb_shift_write);
    cpu_write32(linaddr + 2, thread_cpu.seg_base[SEG_IDTR], thread_cpu.tlb_shift_write);
    NEXT(flags);
}

// An "one size fits all" function for STR/SLDT
OPTYPE op_str_sldt_e16(struct decoded_instruction* i)
{
    if (thread_cpu.cr[4] & CR4_UMIP && thread_cpu.cpl > 0)
        EXCEPTION_GP(0);
    uint32_t flags = i->flags, linaddr = cpu_get_linaddr(flags, i);
    cpu_write16(linaddr, thread_cpu.seg[i->imm8], thread_cpu.tlb_shift_write);
    NEXT(flags);
}
OPTYPE op_str_sldt_r16(struct decoded_instruction* i)
{
    if (thread_cpu.cr[4] & CR4_UMIP && thread_cpu.cpl > 0)
        EXCEPTION_GP(0);
    uint32_t flags = i->flags;
    // "When the destination operand is a 32-bit register, the sixteen-bit segment selector is copied into the lower 16 bits of the register and the upper 16 bits of the register are cleared"
    R32(I_RM(flags)) = (thread_cpu.seg[i->imm8] & i->disp32) | (R32(I_RM(flags)) & ~i->disp32);
    NEXT(flags);
}

OPTYPE op_lgdt_e16(struct decoded_instruction* i)
{
    if (thread_cpu.cpl != 0)
        EXCEPTION_GP(0);

    uint32_t flags = i->flags, linaddr = cpu_get_linaddr(flags, i), base;
    uint16_t limit;
    cpu_read16(linaddr, limit, thread_cpu.tlb_shift_read);
    cpu_read32(linaddr + 2, base, thread_cpu.tlb_shift_read);
    base &= 0x00FFFFFF;
    thread_cpu.seg_limit[SEG_GDTR] = limit;
    thread_cpu.seg_base[SEG_GDTR] = base;
    NEXT(flags);
}
OPTYPE op_lgdt_e32(struct decoded_instruction* i)
{
    if (thread_cpu.cpl != 0)
        EXCEPTION_GP(0);

    uint32_t flags = i->flags, linaddr = cpu_get_linaddr(flags, i), base;
    uint16_t limit;
    cpu_read16(linaddr, limit, thread_cpu.tlb_shift_read);
    cpu_read32(linaddr + 2, base, thread_cpu.tlb_shift_read);
    thread_cpu.seg_limit[SEG_GDTR] = limit;
    thread_cpu.seg_base[SEG_GDTR] = base;
    NEXT(flags);
}
OPTYPE op_lidt_e16(struct decoded_instruction* i)
{
    if (thread_cpu.cpl != 0)
        EXCEPTION_GP(0);

    uint32_t flags = i->flags, linaddr = cpu_get_linaddr(flags, i), base;
    uint16_t limit;
    cpu_read16(linaddr, limit, thread_cpu.tlb_shift_read);
    cpu_read32(linaddr + 2, base, thread_cpu.tlb_shift_read);
    base &= 0x00FFFFFF;
    thread_cpu.seg_limit[SEG_IDTR] = limit;
    thread_cpu.seg_base[SEG_IDTR] = base;
    NEXT(flags);
}
OPTYPE op_lidt_e32(struct decoded_instruction* i)
{
    if (thread_cpu.cpl != 0)
        EXCEPTION_GP(0);

    uint32_t flags = i->flags, linaddr = cpu_get_linaddr(flags, i), base;
    uint16_t limit;
    cpu_read16(linaddr, limit, thread_cpu.tlb_shift_read);
    cpu_read32(linaddr + 2, base, thread_cpu.tlb_shift_read);
    thread_cpu.seg_limit[SEG_IDTR] = limit;
    thread_cpu.seg_base[SEG_IDTR] = base;
    NEXT(flags);
}

OPTYPE op_smsw_r16(struct decoded_instruction* i)
{
    uint32_t flags = i->flags;
    R16(I_RM(flags)) = thread_cpu.cr[0];
    NEXT(flags);
}
OPTYPE op_smsw_r32(struct decoded_instruction* i)
{
    uint32_t flags = i->flags;
    R32(I_RM(flags)) = thread_cpu.cr[0];
    NEXT(flags);
}
OPTYPE op_smsw_e16(struct decoded_instruction* i)
{
    uint32_t flags = i->flags, linaddr = cpu_get_linaddr(flags, i);
    cpu_write16(linaddr, thread_cpu.cr[0], thread_cpu.tlb_shift_write);
    NEXT(flags);
}
OPTYPE op_lmsw_r16(struct decoded_instruction* i)
{
    if (thread_cpu.cpl != 0)
        EXCEPTION_GP(0);

    uint32_t flags = i->flags, v = (thread_cpu.cr[0] & ~0xF) | (R16(I_RM(flags)) & 0xF);
    cpu_prot_set_cr(0, v);
    NEXT(flags);
}
OPTYPE op_lmsw_e16(struct decoded_instruction* i)
{
    if (thread_cpu.cpl != 0)
        EXCEPTION_GP(0);

    uint32_t flags = i->flags, linaddr = cpu_get_linaddr(flags, i), v;
    uint16_t src;
    cpu_read16(linaddr, src, thread_cpu.tlb_shift_write);
    v = (thread_cpu.cr[0] & ~0xFFFF) | (src & 0xFFFF);
    cpu_prot_set_cr(0, v);
    NEXT(flags);
}
OPTYPE op_invlpg_e8(struct decoded_instruction* i)
{
    if (thread_cpu.cpl != 0)
        EXCEPTION_GP(0);
    uint32_t flags = i->flags;
    cpu_mmu_tlb_invalidate(cpu_get_linaddr(flags, i));
    NEXT(flags);
}
OPTYPE op_mov_r32cr(struct decoded_instruction* i)
{
    uint32_t flags = i->flags;
    R32(I_RM(flags)) = thread_cpu.cr[I_REG(flags)];
    thread_cpu.phys_eip += flags & 15;
    // XXX - don't do this here
    thread_cpu.last_phys_eip = thread_cpu.phys_eip - 4096;
    STOP();
}
OPTYPE op_mov_crr32(struct decoded_instruction* i)
{
    uint32_t flags = i->flags;
    cpu_prot_set_cr(I_REG(flags), R32(I_RM(flags)));
    thread_cpu.phys_eip += flags & 15;
    // Force a page refresh, required during Windows 8 boot.
    thread_cpu.last_phys_eip = thread_cpu.phys_eip - 4096;
    STOP();
}
OPTYPE op_mov_r32dr(struct decoded_instruction* i)
{
    uint32_t flags = i->flags;
    R32(I_RM(flags)) = thread_cpu.dr[I_REG(flags)];
    NEXT(flags);
}
OPTYPE op_mov_drr32(struct decoded_instruction* i)
{
    uint32_t flags = i->flags;
    cpu_prot_set_dr(I_REG(flags), R32(I_RM(flags)));
    NEXT(flags);
}
OPTYPE op_ltr_e16(struct decoded_instruction* i)
{
    if ((thread_cpu.cr[0] & CR0_PE) == 0 || thread_cpu.eflags & EFLAGS_VM)
        EXCEPTION_UD();
    if (thread_cpu.cpl != 0)
        EXCEPTION_GP(0);
    uint32_t flags = i->flags, linaddr = cpu_get_linaddr(flags, i), tr;
    cpu_read16(linaddr, tr, thread_cpu.tlb_shift_read);
    if (ltr(tr))
        EXCEP();
    NEXT(flags);
}
OPTYPE op_ltr_r16(struct decoded_instruction* i)
{
    if ((thread_cpu.cr[0] & CR0_PE) == 0 || thread_cpu.eflags & EFLAGS_VM)
        EXCEPTION_UD();
    if (thread_cpu.cpl != 0)
        EXCEPTION_GP(0);
    uint32_t flags = i->flags;
    if (ltr(R32(I_RM(flags)) & 0xFFFF))
        EXCEP();
    NEXT(flags);
}
OPTYPE op_lldt_e16(struct decoded_instruction* i)
{
    if ((thread_cpu.cr[0] & CR0_PE) == 0 || thread_cpu.eflags & EFLAGS_VM)
        EXCEPTION_UD();
    if (thread_cpu.cpl != 0)
        EXCEPTION_GP(0);
    uint32_t flags = i->flags, linaddr = cpu_get_linaddr(flags, i), ldtr;
    cpu_read16(linaddr, ldtr, thread_cpu.tlb_shift_read);
    if (lldt(ldtr))
        EXCEP();
    NEXT(flags);
}
OPTYPE op_lldt_r16(struct decoded_instruction* i)
{
    if ((thread_cpu.cr[0] & CR0_PE) == 0 || thread_cpu.eflags & EFLAGS_VM)
        EXCEPTION_UD();
    if (thread_cpu.cpl != 0)
        EXCEPTION_GP(0);
    uint32_t flags = i->flags;
    if (lldt(R32(I_RM(flags)) & 0xFFFF))
        EXCEP();
    NEXT(flags);
}
OPTYPE op_lar_r16e16(struct decoded_instruction* i)
{
    if ((thread_cpu.cr[0] & CR0_PE) == 0 || thread_cpu.eflags & EFLAGS_VM)
        EXCEPTION_UD();
    uint32_t flags = i->flags, linaddr = cpu_get_linaddr(flags, i), op1;
    cpu_read16(linaddr, op1, thread_cpu.tlb_shift_read);
    R16(I_REG(flags)) = lar(op1, R16(I_REG(flags)));
    NEXT(flags);
}
OPTYPE op_lar_r32e32(struct decoded_instruction* i)
{
    if ((thread_cpu.cr[0] & CR0_PE) == 0 || thread_cpu.eflags & EFLAGS_VM)
        EXCEPTION_UD();
    uint32_t flags = i->flags, linaddr = cpu_get_linaddr(flags, i), op1;
    cpu_read32(linaddr, op1, thread_cpu.tlb_shift_read);
    R32(I_REG(flags)) = lar(op1, R32(I_REG(flags)));
    NEXT(flags);
}
OPTYPE op_lar_r16r16(struct decoded_instruction* i)
{
    if ((thread_cpu.cr[0] & CR0_PE) == 0 || thread_cpu.eflags & EFLAGS_VM)
        EXCEPTION_UD();
    uint32_t flags = i->flags;
    R16(I_REG(flags)) = lar(R16(I_RM(flags)), R16(I_REG(flags)));
    NEXT(flags);
}
OPTYPE op_lar_r32r32(struct decoded_instruction* i)
{
    if ((thread_cpu.cr[0] & CR0_PE) == 0 || thread_cpu.eflags & EFLAGS_VM)
        EXCEPTION_UD();
    uint32_t flags = i->flags;
    R32(I_REG(flags)) = lar(R32(I_RM(flags)) & 0xFFFF, R32(I_REG(flags)));
    NEXT(flags);
}
OPTYPE op_lsl_r16e16(struct decoded_instruction* i)
{
    if ((thread_cpu.cr[0] & CR0_PE) == 0 || thread_cpu.eflags & EFLAGS_VM)
        EXCEPTION_UD();
    uint32_t flags = i->flags, linaddr = cpu_get_linaddr(flags, i), op1;
    cpu_read16(linaddr, op1, thread_cpu.tlb_shift_read);
    R16(I_REG(flags)) = lsl(op1, R16(I_REG(flags)));
    NEXT(flags);
}
OPTYPE op_lsl_r32e32(struct decoded_instruction* i)
{
    if ((thread_cpu.cr[0] & CR0_PE) == 0 || thread_cpu.eflags & EFLAGS_VM)
        EXCEPTION_UD();
    uint32_t flags = i->flags, linaddr = cpu_get_linaddr(flags, i), op1;
    cpu_read32(linaddr, op1, thread_cpu.tlb_shift_read);
    R32(I_REG(flags)) = lsl(op1, R32(I_REG(flags)));
    NEXT(flags);
}
OPTYPE op_lsl_r16r16(struct decoded_instruction* i)
{
    if ((thread_cpu.cr[0] & CR0_PE) == 0 || thread_cpu.eflags & EFLAGS_VM)
        EXCEPTION_UD();
    uint32_t flags = i->flags;
    R16(I_REG(flags)) = lsl(R16(I_RM(flags)), R16(I_REG(flags)));
    NEXT(flags);
}
OPTYPE op_lsl_r32r32(struct decoded_instruction* i)
{
    if ((thread_cpu.cr[0] & CR0_PE) == 0 || thread_cpu.eflags & EFLAGS_VM)
        EXCEPTION_UD();
    uint32_t flags = i->flags;
    R32(I_REG(flags)) = lsl(R32(I_RM(flags)) & 0xFFFF, R32(I_REG(flags)));
    NEXT(flags);
}
OPTYPE op_arpl_e16(struct decoded_instruction* i)
{
    if ((thread_cpu.cr[0] & CR0_PE) == 0 || thread_cpu.eflags & EFLAGS_VM)
        EXCEPTION_UD();
    arith_rmw2(16, arpl, R16(I_REG(flags)));
}
OPTYPE op_arpl_r16(struct decoded_instruction* i)
{
    if ((thread_cpu.cr[0] & CR0_PE) == 0 || thread_cpu.eflags & EFLAGS_VM)
        EXCEPTION_UD();
    int flags = i->flags;
    arpl(&R16(I_RM(flags)), R16(I_REG(flags)));
    NEXT(flags);
}
OPTYPE op_verr_e16(struct decoded_instruction* i)
{
    if ((thread_cpu.cr[0] & CR0_PE) == 0 || thread_cpu.eflags & EFLAGS_VM)
        EXCEPTION_UD();
    uint32_t flags = i->flags, linaddr = cpu_get_linaddr(flags, i), temp;
    cpu_read16(linaddr, temp, thread_cpu.tlb_shift_read);
    verify_segment_access(temp, 0);
    NEXT(flags);
}
OPTYPE op_verr_r16(struct decoded_instruction* i)
{
    if ((thread_cpu.cr[0] & CR0_PE) == 0 || thread_cpu.eflags & EFLAGS_VM)
        EXCEPTION_UD();
    uint32_t flags = i->flags;
    verify_segment_access(R16(I_RM(flags)), 0);
    NEXT(flags);
}
OPTYPE op_verw_e16(struct decoded_instruction* i)
{
    if ((thread_cpu.cr[0] & CR0_PE) == 0 || thread_cpu.eflags & EFLAGS_VM)
        EXCEPTION_UD();
    uint32_t flags = i->flags, linaddr = cpu_get_linaddr(flags, i), temp;
    cpu_read16(linaddr, temp, thread_cpu.tlb_shift_read);
    verify_segment_access(temp, 1);
    NEXT(flags);
}
OPTYPE op_verw_r16(struct decoded_instruction* i)
{
    if ((thread_cpu.cr[0] & CR0_PE) == 0 || thread_cpu.eflags & EFLAGS_VM)
        EXCEPTION_UD();
    uint32_t flags = i->flags;
    verify_segment_access(R16(I_RM(flags)), 1);
    NEXT(flags);
}
OPTYPE op_clts(struct decoded_instruction* i)
{
    if (thread_cpu.cpl != 0)
        EXCEPTION_GP(0);
    thread_cpu.cr[0] &= ~CR0_TS;
    NEXT2(i->flags);
}
OPTYPE op_wbinvd(struct decoded_instruction* i)
{
    if (thread_cpu.cpl != 0)
        EXCEPTION_GP(0);
    NEXT2(i->flags);
}
OPTYPE op_prefetchh(struct decoded_instruction* i)
{
    // idk what to do here
    NEXT2(i->flags);
}

// Sign/Zero extend
OPTYPE op_movzx_r16r8(struct decoded_instruction* i)
{
    int flags = i->flags;
    R16(I_REG(flags)) = R8(I_RM(flags));
    NEXT(flags);
}
OPTYPE op_movzx_r32r8(struct decoded_instruction* i)
{
    int flags = i->flags;
    R32(I_REG(flags)) = R8(I_RM(flags));
    NEXT(flags);
}
OPTYPE op_movzx_r32r16(struct decoded_instruction* i)
{
    int flags = i->flags;
    R32(I_REG(flags)) = R32(I_RM(flags)) & 0xFFFF;
    NEXT(flags);
}
OPTYPE op_movzx_r16e8(struct decoded_instruction* i)
{
    uint32_t flags = i->flags, linaddr = cpu_get_linaddr(flags, i), src = 0;
    cpu_read8(linaddr, src, thread_cpu.tlb_shift_read);
    R16(I_REG(flags)) = src;
    NEXT(flags);
}
OPTYPE op_movzx_r32e8(struct decoded_instruction* i)
{
    uint32_t flags = i->flags, linaddr = cpu_get_linaddr(flags, i), src = 0;
    cpu_read8(linaddr, src, thread_cpu.tlb_shift_read);
    R32(I_REG(flags)) = src;
    NEXT(flags);
}
OPTYPE op_movzx_r32e16(struct decoded_instruction* i)
{
    uint32_t flags = i->flags, linaddr = cpu_get_linaddr(flags, i), src = 0;
    cpu_read16(linaddr, src, thread_cpu.tlb_shift_read);
    R32(I_REG(flags)) = src;
    NEXT(flags);
}

OPTYPE op_movsx_r16r8(struct decoded_instruction* i)
{
    int flags = i->flags;
    R16(I_REG(flags)) = (int8_t)R8(I_RM(flags));
    NEXT(flags);
}
OPTYPE op_movsx_r32r8(struct decoded_instruction* i)
{
    int flags = i->flags;
    R32(I_REG(flags)) = (int8_t)R8(I_RM(flags));
    NEXT(flags);
}
OPTYPE op_movsx_r32r16(struct decoded_instruction* i)
{
    int flags = i->flags;
    R32(I_REG(flags)) = (int16_t)R32(I_RM(flags));
    NEXT(flags);
}
OPTYPE op_movsx_r16e8(struct decoded_instruction* i)
{
    uint32_t flags = i->flags, linaddr = cpu_get_linaddr(flags, i), src;
    cpu_read8(linaddr, src, thread_cpu.tlb_shift_read);
    R16(I_REG(flags)) = (int8_t)src;
    NEXT(flags);
}
OPTYPE op_movsx_r32e8(struct decoded_instruction* i)
{
    uint32_t flags = i->flags, linaddr = cpu_get_linaddr(flags, i), src;
    cpu_read8(linaddr, src, thread_cpu.tlb_shift_read);
    R32(I_REG(flags)) = (int8_t)src;
    NEXT(flags);
}
OPTYPE op_movsx_r32e16(struct decoded_instruction* i)
{
    uint32_t flags = i->flags, linaddr = cpu_get_linaddr(flags, i), src;
    cpu_read16(linaddr, src, thread_cpu.tlb_shift_read);
    R32(I_REG(flags)) = (int16_t)src;
    NEXT(flags);
}

OPTYPE op_xlat16(struct decoded_instruction* i)
{
    uint32_t flags = i->flags, linaddr = (thread_cpu.reg16[BX] + thread_cpu.reg8[AL]) & 0xFFFF, src;
    cpu_read8(linaddr + thread_cpu.seg_base[I_SEG_BASE(flags)], src, thread_cpu.tlb_shift_read);
    thread_cpu.reg8[AL] = src;
    NEXT(flags);
}
OPTYPE op_xlat32(struct decoded_instruction* i)
{
    uint32_t flags = i->flags, linaddr = thread_cpu.reg32[EBX] + thread_cpu.reg8[AL], src;
    cpu_read8(linaddr + thread_cpu.seg_base[I_SEG_BASE(flags)], src, thread_cpu.tlb_shift_read);
    thread_cpu.reg8[AL] = src;
    NEXT(flags);
}

OPTYPE op_bswap_r16(struct decoded_instruction* i)
{
    int flags = i->flags;
    // "Undefined Behavior"
    R16(I_RM(flags)) = 0;
    NEXT(flags);
}
OPTYPE op_bswap_r32(struct decoded_instruction* i)
{
    int flags = i->flags;
    uint32_t reg = R32(I_RM(flags));
    reg = (reg & 0xFF) << 24 | (reg & 0xFF00) << 8 | (reg & 0xFF0000) >> 8 | reg >> 24;
    R32(I_RM(flags)) = reg;
    NEXT(flags);
}

OPTYPE op_fpu_mem(struct decoded_instruction* i)
{
    // We cannot give linaddr by itself since the FPU stores the seg:offs value of the last accessed FPU value.
    int flags = i->flags;
    if (fpu_mem_op(i, cpu_get_virtaddr(flags, i), I_SEG_BASE(flags)))
        EXCEP();
    NEXT(flags);
}
OPTYPE op_fpu_reg(struct decoded_instruction* i)
{
    int flags = i->flags;
    if (fpu_reg_op(i, i->flags))
        EXCEP();
    NEXT(flags);
}
OPTYPE op_fwait(struct decoded_instruction* i)
{
    if (thread_cpu.cr[0] & (CR0_EM | CR0_TS))
        EXCEPTION_NM();
    if (fpu_fwait())
        EXCEP();
    NEXT2(i->flags);
}

OPTYPE op_sysenter(struct decoded_instruction* i){
    UNUSED(i);
    if(sysenter()) EXCEP();
    STOP();
}
OPTYPE op_sysexit(struct decoded_instruction* i){
    UNUSED(i);
    if(sysexit()) EXCEP();
    STOP();
}
OPTYPE op_sse_10_17(struct decoded_instruction* i){
    if(execute_0F10_17(i)) EXCEP();
    NEXT(i->flags);
}
OPTYPE op_sse_28_2F(struct decoded_instruction* i){
    if(execute_0F28_2F(i)) EXCEP();
    NEXT(i->flags);
}
OPTYPE op_sse_50_57(struct decoded_instruction* i){
    if(execute_0F50_57(i)) EXCEP();
    NEXT(i->flags);
}
OPTYPE op_sse_58_5F(struct decoded_instruction* i){
    if(execute_0F58_5F(i)) EXCEP();
    NEXT(i->flags);
}
OPTYPE op_sse_60_67(struct decoded_instruction* i){
    if(execute_0F60_67(i)) EXCEP();
    NEXT(i->flags);
}
OPTYPE op_sse_68_6F(struct decoded_instruction* i){
    if(execute_0F68_6F(i)) EXCEP();
    NEXT(i->flags);
}
OPTYPE op_sse_70_76(struct decoded_instruction* i){
    if(execute_0F70_76(i)) EXCEP();
    NEXT(i->flags);
}
OPTYPE op_sse_7E_7F(struct decoded_instruction* i){
    if(execute_0F7E_7F(i)) EXCEP();
    NEXT(i->flags);
}
OPTYPE op_sse_C2_C6(struct decoded_instruction* i){
    if(execute_0FC2_C6(i)) EXCEP();
    NEXT(i->flags);
}
OPTYPE op_sse_D0_D7(struct decoded_instruction* i){
    if(execute_0FD0_D7(i)) EXCEP();
    NEXT(i->flags);
}
OPTYPE op_sse_D8_DF(struct decoded_instruction* i){
    if(execute_0FD8_DF(i)) EXCEP();
    NEXT(i->flags);
}
OPTYPE op_sse_E0_E7(struct decoded_instruction* i){
    if(execute_0FE0_E7(i)) EXCEP();
    NEXT(i->flags);
}
OPTYPE op_sse_E8_EF(struct decoded_instruction* i){
    if(execute_0FE8_EF(i)) EXCEP();
    NEXT(i->flags);
}
OPTYPE op_sse_F1_F7(struct decoded_instruction* i){
    if(execute_0FF1_F7(i)) EXCEP();
    NEXT(i->flags);
}
OPTYPE op_sse_F8_FE(struct decoded_instruction* i){
    if(execute_0FF8_FE(i)) EXCEP();
    NEXT(i->flags);
}

#define CHECK_SSE if(cpu_sse_exception()) EXCEP()
OPTYPE op_ldmxcsr(struct decoded_instruction* i)
{
    CHECK_SSE;
    uint32_t flags = i->flags, linaddr = cpu_get_linaddr(flags, i), mxcsr;
    cpu_read32(linaddr, mxcsr, thread_cpu.tlb_shift_read);
    if (mxcsr & ~MXCSR_MASK)
        EXCEPTION_GP(0);
    thread_cpu.mxcsr = mxcsr;
    cpu_update_mxcsr();
    NEXT(flags);
}
OPTYPE op_stmxcsr(struct decoded_instruction* i)
{
    CHECK_SSE;
    uint32_t flags = i->flags, linaddr = cpu_get_linaddr(flags, i);
    cpu_write32(linaddr, thread_cpu.mxcsr, thread_cpu.tlb_shift_read);
    NEXT(flags);
}

OPTYPE op_mfence(struct decoded_instruction* i)
{
    // Does nothing at the moment
    NEXT(i->flags);
}
OPTYPE op_fxsave(struct decoded_instruction* i)
{
    uint32_t flags = i->flags, linaddr = cpu_get_linaddr(flags, i);
    if (fpu_fxsave(linaddr))
        EXCEP();
    NEXT(flags);
}
OPTYPE op_fxrstor(struct decoded_instruction* i)
{
    uint32_t flags = i->flags, linaddr = cpu_get_linaddr(flags, i);
    if (fpu_fxrstor(linaddr))
        EXCEP();
    NEXT(flags);
}
OPTYPE op_emms(struct decoded_instruction* i)
{
    if(cpu_emms())
        EXCEP();
    NEXT2(i->flags);
}

// String operations
// <<< BEGIN AUTOGENERATE "string" >>>
OPTYPE op_movsb16(struct decoded_instruction* i)
{
    int flags = i->flags, result = movsb16(flags);
    if (result == 0)
        NEXT(flags);
#ifdef INSTRUMENT
    else if (result == 1)
        STOP2();
    else
#endif
        EXCEP();
}
OPTYPE op_movsb32(struct decoded_instruction* i)
{
    int flags = i->flags, result = movsb32(flags);
    if (result == 0)
        NEXT(flags);
#ifdef INSTRUMENT
    else if (result == 1)
        STOP2();
    else
#endif
        EXCEP();
}
OPTYPE op_movsw16(struct decoded_instruction* i)
{
    int flags = i->flags, result = movsw16(flags);
    if (result == 0)
        NEXT(flags);
#ifdef INSTRUMENT
    else if (result == 1)
        STOP2();
    else
#endif
        EXCEP();
}
OPTYPE op_movsw32(struct decoded_instruction* i)
{
    int flags = i->flags, result = movsw32(flags);
    if (result == 0)
        NEXT(flags);
#ifdef INSTRUMENT
    else if (result == 1)
        STOP2();
    else
#endif
        EXCEP();
}
OPTYPE op_movsd16(struct decoded_instruction* i)
{
    int flags = i->flags, result = movsd16(flags);
    if (result == 0)
        NEXT(flags);
#ifdef INSTRUMENT
    else if (result == 1)
        STOP2();
    else
#endif
        EXCEP();
}
OPTYPE op_movsd32(struct decoded_instruction* i)
{
    int flags = i->flags, result = movsd32(flags);
    if (result == 0)
        NEXT(flags);
#ifdef INSTRUMENT
    else if (result == 1)
        STOP2();
    else
#endif
        EXCEP();
}
OPTYPE op_stosb16(struct decoded_instruction* i)
{
    int flags = i->flags, result = stosb16(flags);
    if (result == 0)
        NEXT(flags);
#ifdef INSTRUMENT
    else if (result == 1)
        STOP2();
    else
#endif
        EXCEP();
}
OPTYPE op_stosb32(struct decoded_instruction* i)
{
    int flags = i->flags, result = stosb32(flags);
    if (result == 0)
        NEXT(flags);
#ifdef INSTRUMENT
    else if (result == 1)
        STOP2();
    else
#endif
        EXCEP();
}
OPTYPE op_stosw16(struct decoded_instruction* i)
{
    int flags = i->flags, result = stosw16(flags);
    if (result == 0)
        NEXT(flags);
#ifdef INSTRUMENT
    else if (result == 1)
        STOP2();
    else
#endif
        EXCEP();
}
OPTYPE op_stosw32(struct decoded_instruction* i)
{
    int flags = i->flags, result = stosw32(flags);
    if (result == 0)
        NEXT(flags);
#ifdef INSTRUMENT
    else if (result == 1)
        STOP2();
    else
#endif
        EXCEP();
}
OPTYPE op_stosd16(struct decoded_instruction* i)
{
    int flags = i->flags, result = stosd16(flags);
    if (result == 0)
        NEXT(flags);
#ifdef INSTRUMENT
    else if (result == 1)
        STOP2();
    else
#endif
        EXCEP();
}
OPTYPE op_stosd32(struct decoded_instruction* i)
{
    int flags = i->flags, result = stosd32(flags);
    if (result == 0)
        NEXT(flags);
#ifdef INSTRUMENT
    else if (result == 1)
        STOP2();
    else
#endif
        EXCEP();
}
OPTYPE op_scasb16(struct decoded_instruction* i)
{
    int flags = i->flags, result = scasb16(flags);
    if (result == 0)
        NEXT(flags);
#ifdef INSTRUMENT
    else if (result == 1)
        STOP2();
    else
#endif
        EXCEP();
}
OPTYPE op_scasb32(struct decoded_instruction* i)
{
    int flags = i->flags, result = scasb32(flags);
    if (result == 0)
        NEXT(flags);
#ifdef INSTRUMENT
    else if (result == 1)
        STOP2();
    else
#endif
        EXCEP();
}
OPTYPE op_scasw16(struct decoded_instruction* i)
{
    int flags = i->flags, result = scasw16(flags);
    if (result == 0)
        NEXT(flags);
#ifdef INSTRUMENT
    else if (result == 1)
        STOP2();
    else
#endif
        EXCEP();
}
OPTYPE op_scasw32(struct decoded_instruction* i)
{
    int flags = i->flags, result = scasw32(flags);
    if (result == 0)
        NEXT(flags);
#ifdef INSTRUMENT
    else if (result == 1)
        STOP2();
    else
#endif
        EXCEP();
}
OPTYPE op_scasd16(struct decoded_instruction* i)
{
    int flags = i->flags, result = scasd16(flags);
    if (result == 0)
        NEXT(flags);
#ifdef INSTRUMENT
    else if (result == 1)
        STOP2();
    else
#endif
        EXCEP();
}
OPTYPE op_scasd32(struct decoded_instruction* i)
{
    int flags = i->flags, result = scasd32(flags);
    if (result == 0)
        NEXT(flags);
#ifdef INSTRUMENT
    else if (result == 1)
        STOP2();
    else
#endif
        EXCEP();
}
OPTYPE op_insb16(struct decoded_instruction* i)
{
    int flags = i->flags, result = insb16(flags);
    if (result == 0)
        NEXT(flags);
#ifdef INSTRUMENT
    else if (result == 1)
        STOP2();
    else
#endif
        EXCEP();
}
OPTYPE op_insb32(struct decoded_instruction* i)
{
    int flags = i->flags, result = insb32(flags);
    if (result == 0)
        NEXT(flags);
#ifdef INSTRUMENT
    else if (result == 1)
        STOP2();
    else
#endif
        EXCEP();
}
OPTYPE op_insw16(struct decoded_instruction* i)
{
    int flags = i->flags, result = insw16(flags);
    if (result == 0)
        NEXT(flags);
#ifdef INSTRUMENT
    else if (result == 1)
        STOP2();
    else
#endif
        EXCEP();
}
OPTYPE op_insw32(struct decoded_instruction* i)
{
    int flags = i->flags, result = insw32(flags);
    if (result == 0)
        NEXT(flags);
#ifdef INSTRUMENT
    else if (result == 1)
        STOP2();
    else
#endif
        EXCEP();
}
OPTYPE op_insd16(struct decoded_instruction* i)
{
    int flags = i->flags, result = insd16(flags);
    if (result == 0)
        NEXT(flags);
#ifdef INSTRUMENT
    else if (result == 1)
        STOP2();
    else
#endif
        EXCEP();
}
OPTYPE op_insd32(struct decoded_instruction* i)
{
    int flags = i->flags, result = insd32(flags);
    if (result == 0)
        NEXT(flags);
#ifdef INSTRUMENT
    else if (result == 1)
        STOP2();
    else
#endif
        EXCEP();
}
OPTYPE op_outsb16(struct decoded_instruction* i)
{
    int flags = i->flags, result = outsb16(flags);
    if (result == 0)
        NEXT(flags);
#ifdef INSTRUMENT
    else if (result == 1)
        STOP2();
    else
#endif
        EXCEP();
}
OPTYPE op_outsb32(struct decoded_instruction* i)
{
    int flags = i->flags, result = outsb32(flags);
    if (result == 0)
        NEXT(flags);
#ifdef INSTRUMENT
    else if (result == 1)
        STOP2();
    else
#endif
        EXCEP();
}
OPTYPE op_outsw16(struct decoded_instruction* i)
{
    int flags = i->flags, result = outsw16(flags);
    if (result == 0)
        NEXT(flags);
#ifdef INSTRUMENT
    else if (result == 1)
        STOP2();
    else
#endif
        EXCEP();
}
OPTYPE op_outsw32(struct decoded_instruction* i)
{
    int flags = i->flags, result = outsw32(flags);
    if (result == 0)
        NEXT(flags);
#ifdef INSTRUMENT
    else if (result == 1)
        STOP2();
    else
#endif
        EXCEP();
}
OPTYPE op_outsd16(struct decoded_instruction* i)
{
    int flags = i->flags, result = outsd16(flags);
    if (result == 0)
        NEXT(flags);
#ifdef INSTRUMENT
    else if (result == 1)
        STOP2();
    else
#endif
        EXCEP();
}
OPTYPE op_outsd32(struct decoded_instruction* i)
{
    int flags = i->flags, result = outsd32(flags);
    if (result == 0)
        NEXT(flags);
#ifdef INSTRUMENT
    else if (result == 1)
        STOP2();
    else
#endif
        EXCEP();
}
OPTYPE op_cmpsb16(struct decoded_instruction* i)
{
    int flags = i->flags, result = cmpsb16(flags);
    if (result == 0)
        NEXT(flags);
#ifdef INSTRUMENT
    else if (result == 1)
        STOP2();
    else
#endif
        EXCEP();
}
OPTYPE op_cmpsb32(struct decoded_instruction* i)
{
    int flags = i->flags, result = cmpsb32(flags);
    if (result == 0)
        NEXT(flags);
#ifdef INSTRUMENT
    else if (result == 1)
        STOP2();
    else
#endif
        EXCEP();
}
OPTYPE op_cmpsw16(struct decoded_instruction* i)
{
    int flags = i->flags, result = cmpsw16(flags);
    if (result == 0)
        NEXT(flags);
#ifdef INSTRUMENT
    else if (result == 1)
        STOP2();
    else
#endif
        EXCEP();
}
OPTYPE op_cmpsw32(struct decoded_instruction* i)
{
    int flags = i->flags, result = cmpsw32(flags);
    if (result == 0)
        NEXT(flags);
#ifdef INSTRUMENT
    else if (result == 1)
        STOP2();
    else
#endif
        EXCEP();
}
OPTYPE op_cmpsd16(struct decoded_instruction* i)
{
    int flags = i->flags, result = cmpsd16(flags);
    if (result == 0)
        NEXT(flags);
#ifdef INSTRUMENT
    else if (result == 1)
        STOP2();
    else
#endif
        EXCEP();
}
OPTYPE op_cmpsd32(struct decoded_instruction* i)
{
    int flags = i->flags, result = cmpsd32(flags);
    if (result == 0)
        NEXT(flags);
#ifdef INSTRUMENT
    else if (result == 1)
        STOP2();
    else
#endif
        EXCEP();
}
OPTYPE op_lodsb16(struct decoded_instruction* i)
{
    int flags = i->flags, result = lodsb16(flags);
    if (result == 0)
        NEXT(flags);
#ifdef INSTRUMENT
    else if (result == 1)
        STOP2();
    else
#endif
        EXCEP();
}
OPTYPE op_lodsb32(struct decoded_instruction* i)
{
    int flags = i->flags, result = lodsb32(flags);
    if (result == 0)
        NEXT(flags);
#ifdef INSTRUMENT
    else if (result == 1)
        STOP2();
    else
#endif
        EXCEP();
}
OPTYPE op_lodsw16(struct decoded_instruction* i)
{
    int flags = i->flags, result = lodsw16(flags);
    if (result == 0)
        NEXT(flags);
#ifdef INSTRUMENT
    else if (result == 1)
        STOP2();
    else
#endif
        EXCEP();
}
OPTYPE op_lodsw32(struct decoded_instruction* i)
{
    int flags = i->flags, result = lodsw32(flags);
    if (result == 0)
        NEXT(flags);
#ifdef INSTRUMENT
    else if (result == 1)
        STOP2();
    else
#endif
        EXCEP();
}
OPTYPE op_lodsd16(struct decoded_instruction* i)
{
    int flags = i->flags, result = lodsd16(flags);
    if (result == 0)
        NEXT(flags);
#ifdef INSTRUMENT
    else if (result == 1)
        STOP2();
    else
#endif
        EXCEP();
}
OPTYPE op_lodsd32(struct decoded_instruction* i)
{
    int flags = i->flags, result = lodsd32(flags);
    if (result == 0)
        NEXT(flags);
#ifdef INSTRUMENT
    else if (result == 1)
        STOP2();
    else
#endif
        EXCEP();
}

// <<< END AUTOGENERATE "string" >>>
