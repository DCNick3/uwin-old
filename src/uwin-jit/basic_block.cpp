//
// Created by dcnick3 on 6/18/20.
//

#include <cassert>

#include "uwin-jit/basic_block.h"

#include "uwin/uwin.h"

#ifdef UW_USE_JITFIX

#include "halfix/cpuapi.h"
#include "halfix/cpu/cpu.h"
#include "halfix/cpu/libcpu.h"

#include "Zydis/Zydis.h"

#undef EAX
#undef EBX
#undef ECX
#undef EDX
#undef ESI
#undef ESP
#undef EBP
#undef EDI
#undef EIP
#define HALFIX_EAX 0
#define HALFIX_ECX 1
#define HALFIX_EDX 2
#define HALFIX_EBX 3
#define HALFIX_ESP 4
#define HALFIX_EBP 5
#define HALFIX_ESI 6
#define HALFIX_EDI 7

#include <vector>
#include <tuple>

static __thread bool dirty = false;
static __thread std::vector<std::tuple<uint32_t, uint32_t, uint64_t>> *dirty_mem;

static __thread ZydisDecoder dec;
static __thread ZydisFormatter fmt;
static __thread ZydisDecodedInstruction instr;

extern "C" int cpu_interrupt(int vector, int error_code, int type, int eip_to_push) {
    dirty = true;
    SET_VIRT_EIP(eip_to_push);
    return 0;
}

extern "C" void mem_make_dirty(uint32_t addr, uint32_t size) {
    uint64_t data;
    switch (size) {
        case 8: data = *g2hx<uint8_t>(addr); break;
        case 16: data = *g2hx<uint16_t>(addr); break;
        case 32: data = *g2hx<uint32_t>(addr); break;
        default:
            assert("Unknown size" == 0);
    }
    dirty_mem->emplace_back(addr, size, data);
}

#define DESC_G_SHIFT    23
#define DESC_G_MASK     (1 << DESC_G_SHIFT)
#define DESC_B_SHIFT    22
#define DESC_B_MASK     (1 << DESC_B_SHIFT)
#define DESC_L_SHIFT    21 /* x86_64 only : 64 bit code segment */
#define DESC_L_MASK     (1 << DESC_L_SHIFT)
#define DESC_AVL_SHIFT  20
#define DESC_AVL_MASK   (1 << DESC_AVL_SHIFT)
#define DESC_P_SHIFT    15
#define DESC_P_MASK     (1 << DESC_P_SHIFT)
#define DESC_DPL_SHIFT  13
#define DESC_DPL_MASK   (3 << DESC_DPL_SHIFT)
#define DESC_S_SHIFT    12
#define DESC_S_MASK     (1 << DESC_S_SHIFT)
#define DESC_TYPE_SHIFT 8
#define DESC_TYPE_MASK  (15 << DESC_TYPE_SHIFT)
#define DESC_A_MASK     (1 << 8)

#define F_SF static_cast<int>(uwin::jit::cpu_flags::SF)
#define F_ZF static_cast<int>(uwin::jit::cpu_flags::ZF)
#define F_CF static_cast<int>(uwin::jit::cpu_flags::CF)
#define F_OF static_cast<int>(uwin::jit::cpu_flags::OF)

const uint32_t flag_used_mask = F_SF | F_ZF | F_CF | F_OF;
const uint32_t flag_upd_mask = ~flag_used_mask;

static void write_dt(void *ptr, unsigned long addr, unsigned long limit,
                     int flags)
{
    unsigned int e1, e2;
    uint32_t *p;
    e1 = (addr << 16) | (limit & 0xffff);
    e2 = ((addr >> 16) & 0xff) | (addr & 0xff000000) | (limit & 0x000f0000);
    e2 |= flags;
    p = reinterpret_cast<uint32_t*>(ptr);
    p[0] = e1;
    p[1] = e2;
}


static void init_halfix() {
    thread_cpu_ptr = new cpu();
    dirty_mem = new std::vector<std::tuple<uint32_t, uint32_t, uint64_t>>();

    cpu_init();
    cpu_init_32bit();

    thread_cpu.cr[0] = CR0_PG | CR0_WP | CR0_PE;
    thread_cpu.cr[4] = CR4_OSXMMEXCPT;

    seg_desc *gdt_table;
    thread_cpu.seg_base[SEG_GDTR] = uw_current_thread_data->gdt_base;
    thread_cpu.seg_limit[SEG_GDTR] = (sizeof(uint64_t) * TARGET_GDT_ENTRIES - 1);

    gdt_table = g2hx<seg_desc>(thread_cpu.seg_base[SEG_GDTR]);


    // code and data (full address space)
    write_dt(&gdt_table[__USER_CS >> 3], 0, 0xfffff,
             DESC_G_MASK | DESC_B_MASK | DESC_P_MASK | DESC_S_MASK |
             (3 << DESC_DPL_SHIFT) | (0xa << DESC_TYPE_SHIFT));
    write_dt(&gdt_table[__USER_DS >> 3], 0, 0xfffff,
             DESC_G_MASK | DESC_B_MASK | DESC_P_MASK | DESC_S_MASK |
             (3 << DESC_DPL_SHIFT) | (0x2 << DESC_TYPE_SHIFT));

    // TEB
    uint32_t teb_base = uw_current_thread_data->teb_base;
    write_dt(&gdt_table[__USER_FS >> 3], teb_base, 1,
             DESC_G_MASK | DESC_B_MASK | DESC_P_MASK | DESC_S_MASK |
             (3 << DESC_DPL_SHIFT) | (0x2 << DESC_TYPE_SHIFT));

    // tls! (only one page though). Can be implemented through TEB, but meh
    uint32_t tls_base = uw_current_thread_data->tls_base;
    write_dt(&gdt_table[__USER_GS >> 3], tls_base, 1,
             DESC_G_MASK | DESC_B_MASK | DESC_P_MASK | DESC_S_MASK |
             (3 << DESC_DPL_SHIFT) | (0x2 << DESC_TYPE_SHIFT));

#define LOD_SEG(seg, val) cpu_seg_load_protected(seg, val, &gdt_table[val >> 3])
    LOD_SEG(CS, __USER_CS);
    LOD_SEG(SS, __USER_DS);
    LOD_SEG(DS, __USER_DS);
    LOD_SEG(ES, __USER_DS);
    LOD_SEG(FS, __USER_FS);
    LOD_SEG(GS, __USER_GS);
#undef LOD_SEG

    dirty_mem->clear();

    ZydisDecoderInit(&dec, ZYDIS_MACHINE_MODE_LEGACY_32, ZYDIS_ADDRESS_WIDTH_32);
    ZydisFormatterInit(&fmt, ZYDIS_FORMATTER_STYLE_ATT);
}

void uwin::jit::basic_block::halfix_enter(uwin::jit::cpu_context *ctx) {
    dirty = false;
    if (thread_cpu_ptr == nullptr) {
        init_halfix();
    }

    ZydisDecoderDecodeBuffer(&dec, g2hx<void>(guest_address), 100, &instr);

    if (guest_address == 0x00626a9f) {
        uw_log("bonk\n");
    }

    thread_cpu.reg32[HALFIX_EAX] = ctx->EAX();
    thread_cpu.reg32[HALFIX_EBX] = ctx->EBX();
    thread_cpu.reg32[HALFIX_ECX] = ctx->ECX();
    thread_cpu.reg32[HALFIX_EDX] = ctx->EDX();
    thread_cpu.reg32[HALFIX_ESP] = ctx->ESP();
    thread_cpu.reg32[HALFIX_EBP] = ctx->EBP();
    thread_cpu.reg32[HALFIX_ESI] = ctx->ESI();
    thread_cpu.reg32[HALFIX_EDI] = ctx->EDI();
    cpu_set_eflags((cpu_get_eflags() & flag_upd_mask) | ctx->EFLAGS());

#ifdef UW_JIT_TRACE
    uw_log("EFLAGS = %x\n", ctx->EFLAGS());
#endif

    SET_VIRT_EIP(guest_address);

    uint32_t initial_eip = guest_address;
    while (initial_eip == VIRT_EIP())
        cpu_run(1);

    for (auto i = dirty_mem->rbegin(); i != dirty_mem->rend(); ++i ) {
        auto addr = std::get<0>(*i);
        auto sz = std::get<1>(*i);
        auto data = std::get<2>(*i);
        switch (sz) {
            case 8:
                *g2hx<uint8_t>(addr) = data;
                break;
            case 16:
                *g2hx<uint16_t>(addr) = data;
                break;
            case 32:
                *g2hx<uint32_t>(addr) = data;
                break;
            default:
                assert("Unknown size" == 0);
        }
    }

    dirty_mem->clear();
}

static void mismatch(uwin::jit::xmem_piece& xmem, uint32_t addr, const std::string& name, uint32_t has, uint32_t want) {
    char buffer[1000];
    ZydisFormatterFormatInstruction(&fmt, &instr, buffer, sizeof(buffer), addr);

    uw_log("jitfix mismatch while executing\n%08lx: %s\nregister %s value is wrong\nHave: %08lx, Want: %08lx",
           (unsigned long)addr, buffer, name.c_str(), (unsigned long)has, (unsigned long)want);

    assert(0);
}

#define TST(r) if (thread_cpu.reg32[HALFIX_ ## r] != ctx->r()) mismatch(xmem, guest_address, #r, ctx->r(), thread_cpu.reg32[HALFIX_ ## r])

void uwin::jit::basic_block::halfix_leave(uwin::jit::cpu_context *ctx) {
    uint32_t actual_used_flags = flag_used_mask;

    if (instr.mnemonic == ZYDIS_MNEMONIC_IMUL)
        actual_used_flags = F_OF | F_CF;
    //if (instr.mnemonic == ZYDIS_MNEMONIC_SAR)
    //    actual_used_flags &= ~F_CF; // see https://github.com/nepx/halfix/issues/7

    if (!dirty && instr.mnemonic != ZYDIS_MNEMONIC_CPUID) {
        if (instr.mnemonic != ZYDIS_MNEMONIC_BSF || (cpu_get_eflags() & static_cast<int>(cpu_flags::ZF)) == 0) {
            TST(EAX);
            TST(EBX);
            TST(ECX);
            TST(EDX);
            TST(ESP);
            TST(EBP);
            TST(ESI);
            TST(EDI);
        }
        if (VIRT_EIP() != ctx->EIP())
            mismatch(xmem, guest_address, "EIP",
                     ctx->EFLAGS() & flag_used_mask, cpu_get_eflags() & flag_used_mask);
        if ((cpu_get_eflags() & actual_used_flags) != (ctx->EFLAGS() & actual_used_flags))
            mismatch(xmem, guest_address, "EFLAGS",
                    ctx->EFLAGS() & flag_used_mask, cpu_get_eflags() & flag_used_mask);
    }
}

#undef TST

#endif

