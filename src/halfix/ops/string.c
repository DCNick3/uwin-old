// String operations. Most of them are automatically generated 

#include "halfix/cpu/cpu.h"
#include "halfix/cpu/opcodes.h"
#include "halfix/cpu/ops.h"
#define repz_or_repnz(flags) (flags & (I_PREFIX_REPZ | I_PREFIX_REPNZ))
#define EXCEPTION_HANDLER return -1 // Note: -1, not 1 like most other exception handlers
#define MAX_CYCLES_TO_RUN 65536

// <<< BEGIN AUTOGENERATE "ops" >>>
int movsb16(int flags)
{
    int count = thread_cpu.reg16[CX], add = thread_cpu.eflags & EFLAGS_DF ? -1 : 1, src, ds_base = thread_cpu.seg_base[I_SEG_BASE(flags)];
    if ((unsigned int)count > MAX_CYCLES_TO_RUN)
    count = MAX_CYCLES_TO_RUN;
    if(!repz_or_repnz(flags)){
        cpu_read8(ds_base + thread_cpu.reg16[SI], src, thread_cpu.tlb_shift_read);
        cpu_write8(thread_cpu.seg_base[ES] + thread_cpu.reg16[DI], src, thread_cpu.tlb_shift_write);
        thread_cpu.reg16[SI] += add;
        thread_cpu.reg16[DI] += add;
        return 0;
    }
    for (int i = 0; i < count; i++) {
        cpu_read8(ds_base + thread_cpu.reg16[SI], src, thread_cpu.tlb_shift_read);
        cpu_write8(thread_cpu.seg_base[ES] + thread_cpu.reg16[DI], src, thread_cpu.tlb_shift_write);
        thread_cpu.reg16[SI] += add;
        thread_cpu.reg16[DI] += add;
        thread_cpu.reg16[CX]--;
        //halfix.cycles_to_run--;
    }
    return thread_cpu.reg16[CX] != 0;
}
int movsb32(int flags)
{
    int count = thread_cpu.reg32[ECX], add = thread_cpu.eflags & EFLAGS_DF ? -1 : 1, src, ds_base = thread_cpu.seg_base[I_SEG_BASE(flags)];
    if ((unsigned int)count > MAX_CYCLES_TO_RUN)
    count = MAX_CYCLES_TO_RUN;
    if(!repz_or_repnz(flags)){
        cpu_read8(ds_base + thread_cpu.reg32[ESI], src, thread_cpu.tlb_shift_read);
        cpu_write8(thread_cpu.seg_base[ES] + thread_cpu.reg32[EDI], src, thread_cpu.tlb_shift_write);
        thread_cpu.reg32[ESI] += add;
        thread_cpu.reg32[EDI] += add;
        return 0;
    }
    for (int i = 0; i < count; i++) {
        cpu_read8(ds_base + thread_cpu.reg32[ESI], src, thread_cpu.tlb_shift_read);
        cpu_write8(thread_cpu.seg_base[ES] + thread_cpu.reg32[EDI], src, thread_cpu.tlb_shift_write);
        thread_cpu.reg32[ESI] += add;
        thread_cpu.reg32[EDI] += add;
        thread_cpu.reg32[ECX]--;
        //halfix.cycles_to_run--;
    }
    return thread_cpu.reg32[ECX] != 0;
}
int movsw16(int flags)
{
    int count = thread_cpu.reg16[CX], add = thread_cpu.eflags & EFLAGS_DF ? -2 : 2, src, ds_base = thread_cpu.seg_base[I_SEG_BASE(flags)];
    if ((unsigned int)count > MAX_CYCLES_TO_RUN)
    count = MAX_CYCLES_TO_RUN;
    if(!repz_or_repnz(flags)){
        cpu_read16(ds_base + thread_cpu.reg16[SI], src, thread_cpu.tlb_shift_read);
        cpu_write16(thread_cpu.seg_base[ES] + thread_cpu.reg16[DI], src, thread_cpu.tlb_shift_write);
        thread_cpu.reg16[SI] += add;
        thread_cpu.reg16[DI] += add;
        return 0;
    }
    for (int i = 0; i < count; i++) {
        cpu_read16(ds_base + thread_cpu.reg16[SI], src, thread_cpu.tlb_shift_read);
        cpu_write16(thread_cpu.seg_base[ES] + thread_cpu.reg16[DI], src, thread_cpu.tlb_shift_write);
        thread_cpu.reg16[SI] += add;
        thread_cpu.reg16[DI] += add;
        thread_cpu.reg16[CX]--;
        //halfix.cycles_to_run--;
    }
    return thread_cpu.reg16[CX] != 0;
}
int movsw32(int flags)
{
    int count = thread_cpu.reg32[ECX], add = thread_cpu.eflags & EFLAGS_DF ? -2 : 2, src, ds_base = thread_cpu.seg_base[I_SEG_BASE(flags)];
    if ((unsigned int)count > MAX_CYCLES_TO_RUN)
    count = MAX_CYCLES_TO_RUN;
    if(!repz_or_repnz(flags)){
        cpu_read16(ds_base + thread_cpu.reg32[ESI], src, thread_cpu.tlb_shift_read);
        cpu_write16(thread_cpu.seg_base[ES] + thread_cpu.reg32[EDI], src, thread_cpu.tlb_shift_write);
        thread_cpu.reg32[ESI] += add;
        thread_cpu.reg32[EDI] += add;
        return 0;
    }
    for (int i = 0; i < count; i++) {
        cpu_read16(ds_base + thread_cpu.reg32[ESI], src, thread_cpu.tlb_shift_read);
        cpu_write16(thread_cpu.seg_base[ES] + thread_cpu.reg32[EDI], src, thread_cpu.tlb_shift_write);
        thread_cpu.reg32[ESI] += add;
        thread_cpu.reg32[EDI] += add;
        thread_cpu.reg32[ECX]--;
        //halfix.cycles_to_run--;
    }
    return thread_cpu.reg32[ECX] != 0;
}
int movsd16(int flags)
{
    int count = thread_cpu.reg16[CX], add = thread_cpu.eflags & EFLAGS_DF ? -4 : 4, src, ds_base = thread_cpu.seg_base[I_SEG_BASE(flags)];
    if ((unsigned int)count > MAX_CYCLES_TO_RUN)
    count = MAX_CYCLES_TO_RUN;
    if(!repz_or_repnz(flags)){
        cpu_read32(ds_base + thread_cpu.reg16[SI], src, thread_cpu.tlb_shift_read);
        cpu_write32(thread_cpu.seg_base[ES] + thread_cpu.reg16[DI], src, thread_cpu.tlb_shift_write);
        thread_cpu.reg16[SI] += add;
        thread_cpu.reg16[DI] += add;
        return 0;
    }
    for (int i = 0; i < count; i++) {
        cpu_read32(ds_base + thread_cpu.reg16[SI], src, thread_cpu.tlb_shift_read);
        cpu_write32(thread_cpu.seg_base[ES] + thread_cpu.reg16[DI], src, thread_cpu.tlb_shift_write);
        thread_cpu.reg16[SI] += add;
        thread_cpu.reg16[DI] += add;
        thread_cpu.reg16[CX]--;
        //halfix.cycles_to_run--;
    }
    return thread_cpu.reg16[CX] != 0;
}
int movsd32(int flags)
{
    int count = thread_cpu.reg32[ECX], add = thread_cpu.eflags & EFLAGS_DF ? -4 : 4, src, ds_base = thread_cpu.seg_base[I_SEG_BASE(flags)];
    if ((unsigned int)count > MAX_CYCLES_TO_RUN)
    count = MAX_CYCLES_TO_RUN;
    if(!repz_or_repnz(flags)){
        cpu_read32(ds_base + thread_cpu.reg32[ESI], src, thread_cpu.tlb_shift_read);
        cpu_write32(thread_cpu.seg_base[ES] + thread_cpu.reg32[EDI], src, thread_cpu.tlb_shift_write);
        thread_cpu.reg32[ESI] += add;
        thread_cpu.reg32[EDI] += add;
        return 0;
    }
    for (int i = 0; i < count; i++) {
        cpu_read32(ds_base + thread_cpu.reg32[ESI], src, thread_cpu.tlb_shift_read);
        cpu_write32(thread_cpu.seg_base[ES] + thread_cpu.reg32[EDI], src, thread_cpu.tlb_shift_write);
        thread_cpu.reg32[ESI] += add;
        thread_cpu.reg32[EDI] += add;
        thread_cpu.reg32[ECX]--;
        //halfix.cycles_to_run--;
    }
    return thread_cpu.reg32[ECX] != 0;
}
int stosb16(int flags)
{
    int count = thread_cpu.reg16[CX], add = thread_cpu.eflags & EFLAGS_DF ? -1 : 1, src = thread_cpu.reg8[AL];
    if ((unsigned int)count > MAX_CYCLES_TO_RUN)
    count = MAX_CYCLES_TO_RUN;
    if(!repz_or_repnz(flags)){
        cpu_write8(thread_cpu.seg_base[ES] + thread_cpu.reg16[DI], src, thread_cpu.tlb_shift_write);
        thread_cpu.reg16[DI] += add;
        return 0;
    }
    for (int i = 0; i < count; i++) {
        cpu_write8(thread_cpu.seg_base[ES] + thread_cpu.reg16[DI], src, thread_cpu.tlb_shift_write);
        thread_cpu.reg16[DI] += add;
        thread_cpu.reg16[CX]--;
        //halfix.cycles_to_run--;
    }
    return thread_cpu.reg16[CX] != 0;
}
int stosb32(int flags)
{
    int count = thread_cpu.reg32[ECX], add = thread_cpu.eflags & EFLAGS_DF ? -1 : 1, src = thread_cpu.reg8[AL];
    if ((unsigned int)count > MAX_CYCLES_TO_RUN)
    count = MAX_CYCLES_TO_RUN;
    if(!repz_or_repnz(flags)){
        cpu_write8(thread_cpu.seg_base[ES] + thread_cpu.reg32[EDI], src, thread_cpu.tlb_shift_write);
        thread_cpu.reg32[EDI] += add;
        return 0;
    }
    for (int i = 0; i < count; i++) {
        cpu_write8(thread_cpu.seg_base[ES] + thread_cpu.reg32[EDI], src, thread_cpu.tlb_shift_write);
        thread_cpu.reg32[EDI] += add;
        thread_cpu.reg32[ECX]--;
        //halfix.cycles_to_run--;
    }
    return thread_cpu.reg32[ECX] != 0;
}
int stosw16(int flags)
{
    int count = thread_cpu.reg16[CX], add = thread_cpu.eflags & EFLAGS_DF ? -2 : 2, src = thread_cpu.reg16[AX];
    if ((unsigned int)count > MAX_CYCLES_TO_RUN)
    count = MAX_CYCLES_TO_RUN;
    if(!repz_or_repnz(flags)){
        cpu_write16(thread_cpu.seg_base[ES] + thread_cpu.reg16[DI], src, thread_cpu.tlb_shift_write);
        thread_cpu.reg16[DI] += add;
        return 0;
    }
    for (int i = 0; i < count; i++) {
        cpu_write16(thread_cpu.seg_base[ES] + thread_cpu.reg16[DI], src, thread_cpu.tlb_shift_write);
        thread_cpu.reg16[DI] += add;
        thread_cpu.reg16[CX]--;
        //halfix.cycles_to_run--;
    }
    return thread_cpu.reg16[CX] != 0;
}
int stosw32(int flags)
{
    int count = thread_cpu.reg32[ECX], add = thread_cpu.eflags & EFLAGS_DF ? -2 : 2, src = thread_cpu.reg16[AX];
    if ((unsigned int)count > MAX_CYCLES_TO_RUN)
    count = MAX_CYCLES_TO_RUN;
    if(!repz_or_repnz(flags)){
        cpu_write16(thread_cpu.seg_base[ES] + thread_cpu.reg32[EDI], src, thread_cpu.tlb_shift_write);
        thread_cpu.reg32[EDI] += add;
        return 0;
    }
    for (int i = 0; i < count; i++) {
        cpu_write16(thread_cpu.seg_base[ES] + thread_cpu.reg32[EDI], src, thread_cpu.tlb_shift_write);
        thread_cpu.reg32[EDI] += add;
        thread_cpu.reg32[ECX]--;
        //halfix.cycles_to_run--;
    }
    return thread_cpu.reg32[ECX] != 0;
}
int stosd16(int flags)
{
    int count = thread_cpu.reg16[CX], add = thread_cpu.eflags & EFLAGS_DF ? -4 : 4, src = thread_cpu.reg32[EAX];
    if ((unsigned int)count > MAX_CYCLES_TO_RUN)
    count = MAX_CYCLES_TO_RUN;
    if(!repz_or_repnz(flags)){
        cpu_write32(thread_cpu.seg_base[ES] + thread_cpu.reg16[DI], src, thread_cpu.tlb_shift_write);
        thread_cpu.reg16[DI] += add;
        return 0;
    }
    for (int i = 0; i < count; i++) {
        cpu_write32(thread_cpu.seg_base[ES] + thread_cpu.reg16[DI], src, thread_cpu.tlb_shift_write);
        thread_cpu.reg16[DI] += add;
        thread_cpu.reg16[CX]--;
        //halfix.cycles_to_run--;
    }
    return thread_cpu.reg16[CX] != 0;
}
int stosd32(int flags)
{
    int count = thread_cpu.reg32[ECX], add = thread_cpu.eflags & EFLAGS_DF ? -4 : 4, src = thread_cpu.reg32[EAX];
    if ((unsigned int)count > MAX_CYCLES_TO_RUN)
    count = MAX_CYCLES_TO_RUN;
    if(!repz_or_repnz(flags)){
        cpu_write32(thread_cpu.seg_base[ES] + thread_cpu.reg32[EDI], src, thread_cpu.tlb_shift_write);
        thread_cpu.reg32[EDI] += add;
        return 0;
    }
    for (int i = 0; i < count; i++) {
        cpu_write32(thread_cpu.seg_base[ES] + thread_cpu.reg32[EDI], src, thread_cpu.tlb_shift_write);
        thread_cpu.reg32[EDI] += add;
        thread_cpu.reg32[ECX]--;
        //halfix.cycles_to_run--;
    }
    return thread_cpu.reg32[ECX] != 0;
}
int scasb16(int flags)
{
    int count = thread_cpu.reg16[CX], add = thread_cpu.eflags & EFLAGS_DF ? -1 : 1;
    uint8_t dest = thread_cpu.reg8[AL], src;
    if ((unsigned int)count > MAX_CYCLES_TO_RUN)
    count = MAX_CYCLES_TO_RUN;
    switch(flags >> I_PREFIX_SHIFT & 3){
        case 0:
        cpu_read8(thread_cpu.seg_base[ES] + thread_cpu.reg16[DI], src, thread_cpu.tlb_shift_read);
            thread_cpu.reg16[DI] += add;
            thread_cpu.lr = (int8_t)(dest - src);
            thread_cpu.lop2 = src;
            thread_cpu.laux = SUB8;
        return 0;
        case 1: // REPZ
        for (int i = 0; i < count; i++) {
            cpu_read8(thread_cpu.seg_base[ES] + thread_cpu.reg16[DI], src, thread_cpu.tlb_shift_read);
            thread_cpu.reg16[DI] += add;
            thread_cpu.reg16[CX]--;
            // XXX don't set this every time
            thread_cpu.lr = (int8_t)(dest - src);
            thread_cpu.lop2 = src;
            thread_cpu.laux = SUB8;
            //halfix.cycles_to_run--;
            if(src != dest) return 0;
        }
        return thread_cpu.reg16[CX] != 0;
        case 2: // REPNZ
        for (int i = 0; i < count; i++) {
            cpu_read8(thread_cpu.seg_base[ES] + thread_cpu.reg16[DI], src, thread_cpu.tlb_shift_read);
            thread_cpu.reg16[DI] += add;
            thread_cpu.reg16[CX]--;
            // XXX don't set this every time
            thread_cpu.lr = (int8_t)(dest - src);
            thread_cpu.lop2 = src;
            thread_cpu.laux = SUB8;
            //halfix.cycles_to_run--;
            if(src == dest) return 0;
        }
        return thread_cpu.reg16[CX] != 0;
    }
    CPU_FATAL("unreachable");
}
int scasb32(int flags)
{
    int count = thread_cpu.reg32[ECX], add = thread_cpu.eflags & EFLAGS_DF ? -1 : 1;
    uint8_t dest = thread_cpu.reg8[AL], src;
    if ((unsigned int)count > MAX_CYCLES_TO_RUN)
    count = MAX_CYCLES_TO_RUN;
    switch(flags >> I_PREFIX_SHIFT & 3){
        case 0:
        cpu_read8(thread_cpu.seg_base[ES] + thread_cpu.reg32[EDI], src, thread_cpu.tlb_shift_read);
            thread_cpu.reg32[EDI] += add;
            thread_cpu.lr = (int8_t)(dest - src);
            thread_cpu.lop2 = src;
            thread_cpu.laux = SUB8;
        return 0;
        case 1: // REPZ
        for (int i = 0; i < count; i++) {
            cpu_read8(thread_cpu.seg_base[ES] + thread_cpu.reg32[EDI], src, thread_cpu.tlb_shift_read);
            thread_cpu.reg32[EDI] += add;
            thread_cpu.reg32[ECX]--;
            // XXX don't set this every time
            thread_cpu.lr = (int8_t)(dest - src);
            thread_cpu.lop2 = src;
            thread_cpu.laux = SUB8;
            //halfix.cycles_to_run--;
            if(src != dest) return 0;
        }
        return thread_cpu.reg32[ECX] != 0;
        case 2: // REPNZ
        for (int i = 0; i < count; i++) {
            cpu_read8(thread_cpu.seg_base[ES] + thread_cpu.reg32[EDI], src, thread_cpu.tlb_shift_read);
            thread_cpu.reg32[EDI] += add;
            thread_cpu.reg32[ECX]--;
            // XXX don't set this every time
            thread_cpu.lr = (int8_t)(dest - src);
            thread_cpu.lop2 = src;
            thread_cpu.laux = SUB8;
            //halfix.cycles_to_run--;
            if(src == dest) return 0;
        }
        return thread_cpu.reg32[ECX] != 0;
    }
    CPU_FATAL("unreachable");
}
int scasw16(int flags)
{
    int count = thread_cpu.reg16[CX], add = thread_cpu.eflags & EFLAGS_DF ? -2 : 2;
    uint16_t dest = thread_cpu.reg16[AX], src;
    if ((unsigned int)count > MAX_CYCLES_TO_RUN)
    count = MAX_CYCLES_TO_RUN;
    switch(flags >> I_PREFIX_SHIFT & 3){
        case 0:
        cpu_read16(thread_cpu.seg_base[ES] + thread_cpu.reg16[DI], src, thread_cpu.tlb_shift_read);
            thread_cpu.reg16[DI] += add;
            thread_cpu.lr = (int16_t)(dest - src);
            thread_cpu.lop2 = src;
            thread_cpu.laux = SUB16;
        return 0;
        case 1: // REPZ
        for (int i = 0; i < count; i++) {
            cpu_read16(thread_cpu.seg_base[ES] + thread_cpu.reg16[DI], src, thread_cpu.tlb_shift_read);
            thread_cpu.reg16[DI] += add;
            thread_cpu.reg16[CX]--;
            // XXX don't set this every time
            thread_cpu.lr = (int16_t)(dest - src);
            thread_cpu.lop2 = src;
            thread_cpu.laux = SUB16;
            //halfix.cycles_to_run--;
            if(src != dest) return 0;
        }
        return thread_cpu.reg16[CX] != 0;
        case 2: // REPNZ
        for (int i = 0; i < count; i++) {
            cpu_read16(thread_cpu.seg_base[ES] + thread_cpu.reg16[DI], src, thread_cpu.tlb_shift_read);
            thread_cpu.reg16[DI] += add;
            thread_cpu.reg16[CX]--;
            // XXX don't set this every time
            thread_cpu.lr = (int16_t)(dest - src);
            thread_cpu.lop2 = src;
            thread_cpu.laux = SUB16;
            //halfix.cycles_to_run--;
            if(src == dest) return 0;
        }
        return thread_cpu.reg16[CX] != 0;
    }
    CPU_FATAL("unreachable");
}
int scasw32(int flags)
{
    int count = thread_cpu.reg32[ECX], add = thread_cpu.eflags & EFLAGS_DF ? -2 : 2;
    uint16_t dest = thread_cpu.reg16[AX], src;
    if ((unsigned int)count > MAX_CYCLES_TO_RUN)
    count = MAX_CYCLES_TO_RUN;
    switch(flags >> I_PREFIX_SHIFT & 3){
        case 0:
        cpu_read16(thread_cpu.seg_base[ES] + thread_cpu.reg32[EDI], src, thread_cpu.tlb_shift_read);
            thread_cpu.reg32[EDI] += add;
            thread_cpu.lr = (int16_t)(dest - src);
            thread_cpu.lop2 = src;
            thread_cpu.laux = SUB16;
        return 0;
        case 1: // REPZ
        for (int i = 0; i < count; i++) {
            cpu_read16(thread_cpu.seg_base[ES] + thread_cpu.reg32[EDI], src, thread_cpu.tlb_shift_read);
            thread_cpu.reg32[EDI] += add;
            thread_cpu.reg32[ECX]--;
            // XXX don't set this every time
            thread_cpu.lr = (int16_t)(dest - src);
            thread_cpu.lop2 = src;
            thread_cpu.laux = SUB16;
            //halfix.cycles_to_run--;
            if(src != dest) return 0;
        }
        return thread_cpu.reg32[ECX] != 0;
        case 2: // REPNZ
        for (int i = 0; i < count; i++) {
            cpu_read16(thread_cpu.seg_base[ES] + thread_cpu.reg32[EDI], src, thread_cpu.tlb_shift_read);
            thread_cpu.reg32[EDI] += add;
            thread_cpu.reg32[ECX]--;
            // XXX don't set this every time
            thread_cpu.lr = (int16_t)(dest - src);
            thread_cpu.lop2 = src;
            thread_cpu.laux = SUB16;
            //halfix.cycles_to_run--;
            if(src == dest) return 0;
        }
        return thread_cpu.reg32[ECX] != 0;
    }
    CPU_FATAL("unreachable");
}
int scasd16(int flags)
{
    int count = thread_cpu.reg16[CX], add = thread_cpu.eflags & EFLAGS_DF ? -4 : 4;
    uint32_t dest = thread_cpu.reg32[EAX], src;
    if ((unsigned int)count > MAX_CYCLES_TO_RUN)
    count = MAX_CYCLES_TO_RUN;
    switch(flags >> I_PREFIX_SHIFT & 3){
        case 0:
        cpu_read32(thread_cpu.seg_base[ES] + thread_cpu.reg16[DI], src, thread_cpu.tlb_shift_read);
            thread_cpu.reg16[DI] += add;
            thread_cpu.lr = (int32_t)(dest - src);
            thread_cpu.lop2 = src;
            thread_cpu.laux = SUB32;
        return 0;
        case 1: // REPZ
        for (int i = 0; i < count; i++) {
            cpu_read32(thread_cpu.seg_base[ES] + thread_cpu.reg16[DI], src, thread_cpu.tlb_shift_read);
            thread_cpu.reg16[DI] += add;
            thread_cpu.reg16[CX]--;
            // XXX don't set this every time
            thread_cpu.lr = (int32_t)(dest - src);
            thread_cpu.lop2 = src;
            thread_cpu.laux = SUB32;
            //halfix.cycles_to_run--;
            if(src != dest) return 0;
        }
        return thread_cpu.reg16[CX] != 0;
        case 2: // REPNZ
        for (int i = 0; i < count; i++) {
            cpu_read32(thread_cpu.seg_base[ES] + thread_cpu.reg16[DI], src, thread_cpu.tlb_shift_read);
            thread_cpu.reg16[DI] += add;
            thread_cpu.reg16[CX]--;
            // XXX don't set this every time
            thread_cpu.lr = (int32_t)(dest - src);
            thread_cpu.lop2 = src;
            thread_cpu.laux = SUB32;
            //halfix.cycles_to_run--;
            if(src == dest) return 0;
        }
        return thread_cpu.reg16[CX] != 0;
    }
    CPU_FATAL("unreachable");
}
int scasd32(int flags)
{
    int count = thread_cpu.reg32[ECX], add = thread_cpu.eflags & EFLAGS_DF ? -4 : 4;
    uint32_t dest = thread_cpu.reg32[EAX], src;
    if ((unsigned int)count > MAX_CYCLES_TO_RUN)
    count = MAX_CYCLES_TO_RUN;
    switch(flags >> I_PREFIX_SHIFT & 3){
        case 0:
        cpu_read32(thread_cpu.seg_base[ES] + thread_cpu.reg32[EDI], src, thread_cpu.tlb_shift_read);
            thread_cpu.reg32[EDI] += add;
            thread_cpu.lr = (int32_t)(dest - src);
            thread_cpu.lop2 = src;
            thread_cpu.laux = SUB32;
        return 0;
        case 1: // REPZ
        for (int i = 0; i < count; i++) {
            cpu_read32(thread_cpu.seg_base[ES] + thread_cpu.reg32[EDI], src, thread_cpu.tlb_shift_read);
            thread_cpu.reg32[EDI] += add;
            thread_cpu.reg32[ECX]--;
            // XXX don't set this every time
            thread_cpu.lr = (int32_t)(dest - src);
            thread_cpu.lop2 = src;
            thread_cpu.laux = SUB32;
            //halfix.cycles_to_run--;
            if(src != dest) return 0;
        }
        return thread_cpu.reg32[ECX] != 0;
        case 2: // REPNZ
        for (int i = 0; i < count; i++) {
            cpu_read32(thread_cpu.seg_base[ES] + thread_cpu.reg32[EDI], src, thread_cpu.tlb_shift_read);
            thread_cpu.reg32[EDI] += add;
            thread_cpu.reg32[ECX]--;
            // XXX don't set this every time
            thread_cpu.lr = (int32_t)(dest - src);
            thread_cpu.lop2 = src;
            thread_cpu.laux = SUB32;
            //halfix.cycles_to_run--;
            if(src == dest) return 0;
        }
        return thread_cpu.reg32[ECX] != 0;
    }
    CPU_FATAL("unreachable");
}
int insb16(int flags)
{
    UNUSED(flags);
    abort();
}
int insb32(int flags)
{
    UNUSED(flags);
    abort();
}
int insw16(int flags)
{
    UNUSED(flags);
    abort();
}
int insw32(int flags)
{
    UNUSED(flags);
    abort();
}
int insd16(int flags)
{
    UNUSED(flags);
    abort();
}
int insd32(int flags)
{
    UNUSED(flags);
    abort();
}
int outsb16(int flags)
{
    UNUSED(flags);
    abort();
}
int outsb32(int flags)
{
    UNUSED(flags);
    abort();
}
int outsw16(int flags)
{
    UNUSED(flags);
    abort();
}
int outsw32(int flags)
{
    UNUSED(flags);
    abort();
}
int outsd16(int flags)
{
    UNUSED(flags);
    abort();
}
int outsd32(int flags)
{
    UNUSED(flags);
    abort();
}
int cmpsb16(int flags)
{
    int count = thread_cpu.reg16[CX], add = thread_cpu.eflags & EFLAGS_DF ? -1 : 1, seg_base = thread_cpu.seg_base[I_SEG_BASE(flags)];
    uint8_t dest, src;
    if ((unsigned int)count > MAX_CYCLES_TO_RUN)
    count = MAX_CYCLES_TO_RUN;
    switch(flags >> I_PREFIX_SHIFT & 3){
        case 0:
        cpu_read8(seg_base + thread_cpu.reg16[SI], dest, thread_cpu.tlb_shift_read);
        cpu_read8(thread_cpu.seg_base[ES] + thread_cpu.reg16[DI], src, thread_cpu.tlb_shift_read);
            thread_cpu.reg16[DI] += add;
            thread_cpu.reg16[SI] += add;
            thread_cpu.lr = (int8_t)(dest - src);
            thread_cpu.lop2 = src;
            thread_cpu.laux = SUB8;
        return 0;
        case 1: // REPZ
        for (int i = 0; i < count; i++) {
            cpu_read8(seg_base + thread_cpu.reg16[SI], dest, thread_cpu.tlb_shift_read);
            cpu_read8(thread_cpu.seg_base[ES] + thread_cpu.reg16[DI], src, thread_cpu.tlb_shift_read);
            thread_cpu.reg16[DI] += add;
            thread_cpu.reg16[SI] += add;
            thread_cpu.reg16[CX]--;
            // XXX don't set this every time
            thread_cpu.lr = (int8_t)(dest - src);
            thread_cpu.lop2 = src;
            thread_cpu.laux = SUB8;
            //halfix.cycles_to_run--;
            if(src != dest) return 0;
        }
        return thread_cpu.reg16[CX] != 0;
        case 2: // REPNZ
        for (int i = 0; i < count; i++) {
            cpu_read8(seg_base + thread_cpu.reg16[SI], dest, thread_cpu.tlb_shift_read);
            cpu_read8(thread_cpu.seg_base[ES] + thread_cpu.reg16[DI], src, thread_cpu.tlb_shift_read);
            thread_cpu.reg16[DI] += add;
            thread_cpu.reg16[SI] += add;
            thread_cpu.reg16[CX]--;
            // XXX don't set this every time
            thread_cpu.lr = (int8_t)(dest - src);
            thread_cpu.lop2 = src;
            thread_cpu.laux = SUB8;
            //halfix.cycles_to_run--;
            if(src == dest) return 0;
        }
        return thread_cpu.reg16[CX] != 0;
    }
    CPU_FATAL("unreachable");
}
int cmpsb32(int flags)
{
    int count = thread_cpu.reg32[ECX], add = thread_cpu.eflags & EFLAGS_DF ? -1 : 1, seg_base = thread_cpu.seg_base[I_SEG_BASE(flags)];
    uint8_t dest, src;
    if ((unsigned int)count > MAX_CYCLES_TO_RUN)
    count = MAX_CYCLES_TO_RUN;
    switch(flags >> I_PREFIX_SHIFT & 3){
        case 0:
        cpu_read8(seg_base + thread_cpu.reg32[ESI], dest, thread_cpu.tlb_shift_read);
        cpu_read8(thread_cpu.seg_base[ES] + thread_cpu.reg32[EDI], src, thread_cpu.tlb_shift_read);
            thread_cpu.reg32[EDI] += add;
            thread_cpu.reg32[ESI] += add;
            thread_cpu.lr = (int8_t)(dest - src);
            thread_cpu.lop2 = src;
            thread_cpu.laux = SUB8;
        return 0;
        case 1: // REPZ
        for (int i = 0; i < count; i++) {
            cpu_read8(seg_base + thread_cpu.reg32[ESI], dest, thread_cpu.tlb_shift_read);
            cpu_read8(thread_cpu.seg_base[ES] + thread_cpu.reg32[EDI], src, thread_cpu.tlb_shift_read);
            thread_cpu.reg32[EDI] += add;
            thread_cpu.reg32[ESI] += add;
            thread_cpu.reg32[ECX]--;
            // XXX don't set this every time
            thread_cpu.lr = (int8_t)(dest - src);
            thread_cpu.lop2 = src;
            thread_cpu.laux = SUB8;
            //halfix.cycles_to_run--;
            if(src != dest) return 0;
        }
        return thread_cpu.reg32[ECX] != 0;
        case 2: // REPNZ
        for (int i = 0; i < count; i++) {
            cpu_read8(seg_base + thread_cpu.reg32[ESI], dest, thread_cpu.tlb_shift_read);
            cpu_read8(thread_cpu.seg_base[ES] + thread_cpu.reg32[EDI], src, thread_cpu.tlb_shift_read);
            thread_cpu.reg32[EDI] += add;
            thread_cpu.reg32[ESI] += add;
            thread_cpu.reg32[ECX]--;
            // XXX don't set this every time
            thread_cpu.lr = (int8_t)(dest - src);
            thread_cpu.lop2 = src;
            thread_cpu.laux = SUB8;
            //halfix.cycles_to_run--;
            if(src == dest) return 0;
        }
        return thread_cpu.reg32[ECX] != 0;
    }
    CPU_FATAL("unreachable");
}
int cmpsw16(int flags)
{
    int count = thread_cpu.reg16[CX], add = thread_cpu.eflags & EFLAGS_DF ? -2 : 2, seg_base = thread_cpu.seg_base[I_SEG_BASE(flags)];
    uint16_t dest, src;
    if ((unsigned int)count > MAX_CYCLES_TO_RUN)
    count = MAX_CYCLES_TO_RUN;
    switch(flags >> I_PREFIX_SHIFT & 3){
        case 0:
        cpu_read16(seg_base + thread_cpu.reg16[SI], dest, thread_cpu.tlb_shift_read);
        cpu_read16(thread_cpu.seg_base[ES] + thread_cpu.reg16[DI], src, thread_cpu.tlb_shift_read);
            thread_cpu.reg16[DI] += add;
            thread_cpu.reg16[SI] += add;
            thread_cpu.lr = (int16_t)(dest - src);
            thread_cpu.lop2 = src;
            thread_cpu.laux = SUB16;
        return 0;
        case 1: // REPZ
        for (int i = 0; i < count; i++) {
            cpu_read16(seg_base + thread_cpu.reg16[SI], dest, thread_cpu.tlb_shift_read);
            cpu_read16(thread_cpu.seg_base[ES] + thread_cpu.reg16[DI], src, thread_cpu.tlb_shift_read);
            thread_cpu.reg16[DI] += add;
            thread_cpu.reg16[SI] += add;
            thread_cpu.reg16[CX]--;
            // XXX don't set this every time
            thread_cpu.lr = (int16_t)(dest - src);
            thread_cpu.lop2 = src;
            thread_cpu.laux = SUB16;
            //halfix.cycles_to_run--;
            if(src != dest) return 0;
        }
        return thread_cpu.reg16[CX] != 0;
        case 2: // REPNZ
        for (int i = 0; i < count; i++) {
            cpu_read16(seg_base + thread_cpu.reg16[SI], dest, thread_cpu.tlb_shift_read);
            cpu_read16(thread_cpu.seg_base[ES] + thread_cpu.reg16[DI], src, thread_cpu.tlb_shift_read);
            thread_cpu.reg16[DI] += add;
            thread_cpu.reg16[SI] += add;
            thread_cpu.reg16[CX]--;
            // XXX don't set this every time
            thread_cpu.lr = (int16_t)(dest - src);
            thread_cpu.lop2 = src;
            thread_cpu.laux = SUB16;
            //halfix.cycles_to_run--;
            if(src == dest) return 0;
        }
        return thread_cpu.reg16[CX] != 0;
    }
    CPU_FATAL("unreachable");
}
int cmpsw32(int flags)
{
    int count = thread_cpu.reg32[ECX], add = thread_cpu.eflags & EFLAGS_DF ? -2 : 2, seg_base = thread_cpu.seg_base[I_SEG_BASE(flags)];
    uint16_t dest, src;
    if ((unsigned int)count > MAX_CYCLES_TO_RUN)
    count = MAX_CYCLES_TO_RUN;
    switch(flags >> I_PREFIX_SHIFT & 3){
        case 0:
        cpu_read16(seg_base + thread_cpu.reg32[ESI], dest, thread_cpu.tlb_shift_read);
        cpu_read16(thread_cpu.seg_base[ES] + thread_cpu.reg32[EDI], src, thread_cpu.tlb_shift_read);
            thread_cpu.reg32[EDI] += add;
            thread_cpu.reg32[ESI] += add;
            thread_cpu.lr = (int16_t)(dest - src);
            thread_cpu.lop2 = src;
            thread_cpu.laux = SUB16;
        return 0;
        case 1: // REPZ
        for (int i = 0; i < count; i++) {
            cpu_read16(seg_base + thread_cpu.reg32[ESI], dest, thread_cpu.tlb_shift_read);
            cpu_read16(thread_cpu.seg_base[ES] + thread_cpu.reg32[EDI], src, thread_cpu.tlb_shift_read);
            thread_cpu.reg32[EDI] += add;
            thread_cpu.reg32[ESI] += add;
            thread_cpu.reg32[ECX]--;
            // XXX don't set this every time
            thread_cpu.lr = (int16_t)(dest - src);
            thread_cpu.lop2 = src;
            thread_cpu.laux = SUB16;
            //halfix.cycles_to_run--;
            if(src != dest) return 0;
        }
        return thread_cpu.reg32[ECX] != 0;
        case 2: // REPNZ
        for (int i = 0; i < count; i++) {
            cpu_read16(seg_base + thread_cpu.reg32[ESI], dest, thread_cpu.tlb_shift_read);
            cpu_read16(thread_cpu.seg_base[ES] + thread_cpu.reg32[EDI], src, thread_cpu.tlb_shift_read);
            thread_cpu.reg32[EDI] += add;
            thread_cpu.reg32[ESI] += add;
            thread_cpu.reg32[ECX]--;
            // XXX don't set this every time
            thread_cpu.lr = (int16_t)(dest - src);
            thread_cpu.lop2 = src;
            thread_cpu.laux = SUB16;
            //halfix.cycles_to_run--;
            if(src == dest) return 0;
        }
        return thread_cpu.reg32[ECX] != 0;
    }
    CPU_FATAL("unreachable");
}
int cmpsd16(int flags)
{
    int count = thread_cpu.reg16[CX], add = thread_cpu.eflags & EFLAGS_DF ? -4 : 4, seg_base = thread_cpu.seg_base[I_SEG_BASE(flags)];
    uint32_t dest, src;
    if ((unsigned int)count > MAX_CYCLES_TO_RUN)
    count = MAX_CYCLES_TO_RUN;
    switch(flags >> I_PREFIX_SHIFT & 3){
        case 0:
        cpu_read32(seg_base + thread_cpu.reg16[SI], dest, thread_cpu.tlb_shift_read);
        cpu_read32(thread_cpu.seg_base[ES] + thread_cpu.reg16[DI], src, thread_cpu.tlb_shift_read);
            thread_cpu.reg16[DI] += add;
            thread_cpu.reg16[SI] += add;
            thread_cpu.lr = (int32_t)(dest - src);
            thread_cpu.lop2 = src;
            thread_cpu.laux = SUB32;
        return 0;
        case 1: // REPZ
        for (int i = 0; i < count; i++) {
            cpu_read32(seg_base + thread_cpu.reg16[SI], dest, thread_cpu.tlb_shift_read);
            cpu_read32(thread_cpu.seg_base[ES] + thread_cpu.reg16[DI], src, thread_cpu.tlb_shift_read);
            thread_cpu.reg16[DI] += add;
            thread_cpu.reg16[SI] += add;
            thread_cpu.reg16[CX]--;
            // XXX don't set this every time
            thread_cpu.lr = (int32_t)(dest - src);
            thread_cpu.lop2 = src;
            thread_cpu.laux = SUB32;
            //halfix.cycles_to_run--;
            if(src != dest) return 0;
        }
        return thread_cpu.reg16[CX] != 0;
        case 2: // REPNZ
        for (int i = 0; i < count; i++) {
            cpu_read32(seg_base + thread_cpu.reg16[SI], dest, thread_cpu.tlb_shift_read);
            cpu_read32(thread_cpu.seg_base[ES] + thread_cpu.reg16[DI], src, thread_cpu.tlb_shift_read);
            thread_cpu.reg16[DI] += add;
            thread_cpu.reg16[SI] += add;
            thread_cpu.reg16[CX]--;
            // XXX don't set this every time
            thread_cpu.lr = (int32_t)(dest - src);
            thread_cpu.lop2 = src;
            thread_cpu.laux = SUB32;
            //halfix.cycles_to_run--;
            if(src == dest) return 0;
        }
        return thread_cpu.reg16[CX] != 0;
    }
    CPU_FATAL("unreachable");
}
int cmpsd32(int flags)
{
    int count = thread_cpu.reg32[ECX], add = thread_cpu.eflags & EFLAGS_DF ? -4 : 4, seg_base = thread_cpu.seg_base[I_SEG_BASE(flags)];
    uint32_t dest, src;
    if ((unsigned int)count > MAX_CYCLES_TO_RUN)
    count = MAX_CYCLES_TO_RUN;
    switch(flags >> I_PREFIX_SHIFT & 3){
        case 0:
        cpu_read32(seg_base + thread_cpu.reg32[ESI], dest, thread_cpu.tlb_shift_read);
        cpu_read32(thread_cpu.seg_base[ES] + thread_cpu.reg32[EDI], src, thread_cpu.tlb_shift_read);
            thread_cpu.reg32[EDI] += add;
            thread_cpu.reg32[ESI] += add;
            thread_cpu.lr = (int32_t)(dest - src);
            thread_cpu.lop2 = src;
            thread_cpu.laux = SUB32;
        return 0;
        case 1: // REPZ
        for (int i = 0; i < count; i++) {
            cpu_read32(seg_base + thread_cpu.reg32[ESI], dest, thread_cpu.tlb_shift_read);
            cpu_read32(thread_cpu.seg_base[ES] + thread_cpu.reg32[EDI], src, thread_cpu.tlb_shift_read);
            thread_cpu.reg32[EDI] += add;
            thread_cpu.reg32[ESI] += add;
            thread_cpu.reg32[ECX]--;
            // XXX don't set this every time
            thread_cpu.lr = (int32_t)(dest - src);
            thread_cpu.lop2 = src;
            thread_cpu.laux = SUB32;
            //halfix.cycles_to_run--;
            if(src != dest) return 0;
        }
        return thread_cpu.reg32[ECX] != 0;
        case 2: // REPNZ
        for (int i = 0; i < count; i++) {
            cpu_read32(seg_base + thread_cpu.reg32[ESI], dest, thread_cpu.tlb_shift_read);
            cpu_read32(thread_cpu.seg_base[ES] + thread_cpu.reg32[EDI], src, thread_cpu.tlb_shift_read);
            thread_cpu.reg32[EDI] += add;
            thread_cpu.reg32[ESI] += add;
            thread_cpu.reg32[ECX]--;
            // XXX don't set this every time
            thread_cpu.lr = (int32_t)(dest - src);
            thread_cpu.lop2 = src;
            thread_cpu.laux = SUB32;
            //halfix.cycles_to_run--;
            if(src == dest) return 0;
        }
        return thread_cpu.reg32[ECX] != 0;
    }
    CPU_FATAL("unreachable");
}
int lodsb16(int flags)
{
    int count = thread_cpu.reg16[CX], add = thread_cpu.eflags & EFLAGS_DF ? -1 : 1, seg_base = thread_cpu.seg_base[I_SEG_BASE(flags)];
    if ((unsigned int)count > MAX_CYCLES_TO_RUN)
    count = MAX_CYCLES_TO_RUN;
    if(!repz_or_repnz(flags)){
        cpu_read8(seg_base + thread_cpu.reg16[SI], thread_cpu.reg8[AL], thread_cpu.tlb_shift_read);
        thread_cpu.reg16[SI] += add;
        return 0;
    }
    for (int i = 0; i < count; i++) {
        cpu_read8(seg_base + thread_cpu.reg16[SI], thread_cpu.reg8[AL], thread_cpu.tlb_shift_read);
        thread_cpu.reg16[SI] += add;
        thread_cpu.reg16[CX]--;
        //halfix.cycles_to_run--;
    }
    return thread_cpu.reg16[CX] != 0;
}
int lodsb32(int flags)
{
    int count = thread_cpu.reg32[ECX], add = thread_cpu.eflags & EFLAGS_DF ? -1 : 1, seg_base = thread_cpu.seg_base[I_SEG_BASE(flags)];
    if ((unsigned int)count > MAX_CYCLES_TO_RUN)
    count = MAX_CYCLES_TO_RUN;
    if(!repz_or_repnz(flags)){
        cpu_read8(seg_base + thread_cpu.reg32[ESI], thread_cpu.reg8[AL], thread_cpu.tlb_shift_read);
        thread_cpu.reg32[ESI] += add;
        return 0;
    }
    for (int i = 0; i < count; i++) {
        cpu_read8(seg_base + thread_cpu.reg32[ESI], thread_cpu.reg8[AL], thread_cpu.tlb_shift_read);
        thread_cpu.reg32[ESI] += add;
        thread_cpu.reg32[ECX]--;
        //halfix.cycles_to_run--;
    }
    return thread_cpu.reg32[ECX] != 0;
}
int lodsw16(int flags)
{
    int count = thread_cpu.reg16[CX], add = thread_cpu.eflags & EFLAGS_DF ? -2 : 2, seg_base = thread_cpu.seg_base[I_SEG_BASE(flags)];
    if ((unsigned int)count > MAX_CYCLES_TO_RUN)
    count = MAX_CYCLES_TO_RUN;
    if(!repz_or_repnz(flags)){
        cpu_read16(seg_base + thread_cpu.reg16[SI], thread_cpu.reg16[AX], thread_cpu.tlb_shift_read);
        thread_cpu.reg16[SI] += add;
        return 0;
    }
    for (int i = 0; i < count; i++) {
        cpu_read16(seg_base + thread_cpu.reg16[SI], thread_cpu.reg16[AX], thread_cpu.tlb_shift_read);
        thread_cpu.reg16[SI] += add;
        thread_cpu.reg16[CX]--;
        //halfix.cycles_to_run--;
    }
    return thread_cpu.reg16[CX] != 0;
}
int lodsw32(int flags)
{
    int count = thread_cpu.reg32[ECX], add = thread_cpu.eflags & EFLAGS_DF ? -2 : 2, seg_base = thread_cpu.seg_base[I_SEG_BASE(flags)];
    if ((unsigned int)count > MAX_CYCLES_TO_RUN)
    count = MAX_CYCLES_TO_RUN;
    if(!repz_or_repnz(flags)){
        cpu_read16(seg_base + thread_cpu.reg32[ESI], thread_cpu.reg16[AX], thread_cpu.tlb_shift_read);
        thread_cpu.reg32[ESI] += add;
        return 0;
    }
    for (int i = 0; i < count; i++) {
        cpu_read16(seg_base + thread_cpu.reg32[ESI], thread_cpu.reg16[AX], thread_cpu.tlb_shift_read);
        thread_cpu.reg32[ESI] += add;
        thread_cpu.reg32[ECX]--;
        //halfix.cycles_to_run--;
    }
    return thread_cpu.reg32[ECX] != 0;
}
int lodsd16(int flags)
{
    int count = thread_cpu.reg16[CX], add = thread_cpu.eflags & EFLAGS_DF ? -4 : 4, seg_base = thread_cpu.seg_base[I_SEG_BASE(flags)];
    if ((unsigned int)count > MAX_CYCLES_TO_RUN)
    count = MAX_CYCLES_TO_RUN;
    if(!repz_or_repnz(flags)){
        cpu_read32(seg_base + thread_cpu.reg16[SI], thread_cpu.reg32[EAX], thread_cpu.tlb_shift_read);
        thread_cpu.reg16[SI] += add;
        return 0;
    }
    for (int i = 0; i < count; i++) {
        cpu_read32(seg_base + thread_cpu.reg16[SI], thread_cpu.reg32[EAX], thread_cpu.tlb_shift_read);
        thread_cpu.reg16[SI] += add;
        thread_cpu.reg16[CX]--;
        //halfix.cycles_to_run--;
    }
    return thread_cpu.reg16[CX] != 0;
}
int lodsd32(int flags)
{
    int count = thread_cpu.reg32[ECX], add = thread_cpu.eflags & EFLAGS_DF ? -4 : 4, seg_base = thread_cpu.seg_base[I_SEG_BASE(flags)];
    if ((unsigned int)count > MAX_CYCLES_TO_RUN)
    count = MAX_CYCLES_TO_RUN;
    if(!repz_or_repnz(flags)){
        cpu_read32(seg_base + thread_cpu.reg32[ESI], thread_cpu.reg32[EAX], thread_cpu.tlb_shift_read);
        thread_cpu.reg32[ESI] += add;
        return 0;
    }
    for (int i = 0; i < count; i++) {
        cpu_read32(seg_base + thread_cpu.reg32[ESI], thread_cpu.reg32[EAX], thread_cpu.tlb_shift_read);
        thread_cpu.reg32[ESI] += add;
        thread_cpu.reg32[ECX]--;
        //halfix.cycles_to_run--;
    }
    return thread_cpu.reg32[ECX] != 0;
}

// <<< END AUTOGENERATE "ops" >>>
