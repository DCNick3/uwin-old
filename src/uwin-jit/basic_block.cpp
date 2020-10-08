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


#ifdef UW_USE_GDB_JIT_INTERFACE
#include <elf.h>

typedef enum
{
    JIT_NOACTION = 0,
    JIT_REGISTER_FN,
    JIT_UNREGISTER_FN
} jit_actions_t;

struct jit_code_entry
{
    struct jit_code_entry *next_entry;
    struct jit_code_entry *prev_entry;
    const char *symfile_addr;
    uint64_t symfile_size;
};

struct jit_descriptor
{
    uint32_t version;
    /* This type should be jit_actions_t, but we use uint32_t
       to be explicit about the bitwidth.  */
    uint32_t action_flag;
    struct jit_code_entry *relevant_entry;
    struct jit_code_entry *first_entry;
};

extern "C" {
/* GDB puts a breakpoint in this function.  */
void __attribute__((noinline)) __jit_debug_register_code() { };

/* Make sure to specify the version statically, because the
   debugger may check the version before we can set it.  */
struct jit_descriptor __jit_debug_descriptor = { 1, 0, 0, 0 };
};

std::vector<std::uint8_t> uwin::jit::basic_block::make_gdb_elf()
{
    // very ugly solution. Only aarch64 is supported

    // The generated elf has the following structure:
    // - elf header
    // - program header for .text
    // - section header for NULL section
    // - section header for .text
    // - section header for strings
    // - section data for .text
    // - section data for strings


    std::string null_section_name = ".null";
    std::string text_section_name = ".text";
    std::string shstrtab_section_name = ".shstrtab";
    std::vector<uint8_t> string_table;
    int null_section_name_index = string_table.size();
    string_table.insert(string_table.end(), null_section_name.c_str(), null_section_name.c_str() + null_section_name.size() + 1);
    int text_section_name_index = string_table.size();
    string_table.insert(string_table.end(), text_section_name.c_str(), text_section_name.c_str() + text_section_name.size() + 1);
    int shstrtab_section_name_index = string_table.size();
    string_table.insert(string_table.end(), shstrtab_section_name.c_str(), shstrtab_section_name.c_str() + shstrtab_section_name.size() + 1);


    Elf64_Ehdr elf_header;
    elf_header.e_ident[EI_MAG0] = ELFMAG0;
    elf_header.e_ident[EI_MAG1] = ELFMAG1;
    elf_header.e_ident[EI_MAG2] = ELFMAG2;
    elf_header.e_ident[EI_MAG3] = ELFMAG3;
    elf_header.e_ident[EI_CLASS] = ELFCLASS64;
    elf_header.e_ident[EI_DATA] = ELFDATA2LSB;
    elf_header.e_ident[EI_VERSION] = EV_CURRENT;
    elf_header.e_ident[EI_OSABI] = ELFOSABI_NONE;
    elf_header.e_ident[EI_ABIVERSION] = 0;
    elf_header.e_ident[EI_PAD] = 0;
    elf_header.e_type = ET_EXEC;
    elf_header.e_machine = EM_AARCH64;
    elf_header.e_version = EV_CURRENT;
    elf_header.e_entry = reinterpret_cast<Elf64_Addr>(xmem.rx_ptr());
    elf_header.e_phoff = sizeof(Elf64_Ehdr);
    elf_header.e_shoff = sizeof(Elf64_Ehdr) + sizeof(Elf64_Phdr);
    elf_header.e_flags = 0;
    elf_header.e_ehsize = sizeof(Elf64_Ehdr);
    elf_header.e_phentsize = sizeof(Elf64_Phdr);
    elf_header.e_phnum = 1;
    elf_header.e_shentsize = sizeof(Elf64_Shdr);
    elf_header.e_shnum = 3;
    elf_header.e_shstrndx = 2;

    Elf64_Phdr program_header;
    program_header.p_type = PT_LOAD;
    program_header.p_offset = sizeof(Elf64_Ehdr) + sizeof(Elf64_Phdr) + sizeof(Elf64_Shdr) * 3;
    program_header.p_vaddr = reinterpret_cast<Elf64_Addr>(xmem.rx_ptr());
    program_header.p_paddr = reinterpret_cast<Elf64_Addr>(xmem.rx_ptr());
    program_header.p_filesz = xmem.size();
    program_header.p_memsz = xmem.size();
    program_header.p_flags = PF_X | PF_R;
    program_header.p_align = 0;

    Elf64_Shdr null_section_header;
    null_section_header.sh_name = null_section_name_index;
    null_section_header.sh_type = SHT_NULL;
    null_section_header.sh_flags = 0;
    null_section_header.sh_addr = 0;
    null_section_header.sh_offset = 0;
    null_section_header.sh_size = 0;
    null_section_header.sh_link = 0;
    null_section_header.sh_info = 0;
    null_section_header.sh_addralign = 0;
    null_section_header.sh_entsize = 0;

    Elf64_Shdr text_section_header;
    text_section_header.sh_name = text_section_name_index;
    text_section_header.sh_type = SHT_PROGBITS;
    text_section_header.sh_flags = SHF_ALLOC | SHF_EXECINSTR;
    text_section_header.sh_addr = reinterpret_cast<Elf64_Addr>(xmem.rx_ptr());
    text_section_header.sh_offset = sizeof(Elf64_Ehdr) + sizeof(Elf64_Phdr) + sizeof(Elf64_Shdr) * 3;
    text_section_header.sh_size = xmem.size();
    text_section_header.sh_link = 0;
    text_section_header.sh_info = 0;
    text_section_header.sh_addralign = 0;
    text_section_header.sh_entsize = 0;

    Elf64_Shdr shstrtab_section_header;
    shstrtab_section_header.sh_name = shstrtab_section_name_index;
    shstrtab_section_header.sh_type = SHT_STRTAB;
    shstrtab_section_header.sh_flags = 0;
    shstrtab_section_header.sh_addr = 0;
    shstrtab_section_header.sh_offset = sizeof(Elf64_Ehdr) + sizeof(Elf64_Phdr) + sizeof(Elf64_Shdr) * 3 + xmem.size();
    shstrtab_section_header.sh_size = string_table.size();
    shstrtab_section_header.sh_link = 0;
    shstrtab_section_header.sh_info = 0;
    shstrtab_section_header.sh_addralign = 0;
    shstrtab_section_header.sh_entsize = 0;

    std::vector<uint8_t> result;
    result.insert(result.end(), reinterpret_cast<uint8_t*>(&elf_header), reinterpret_cast<uint8_t*>(&elf_header + 1));
    result.insert(result.end(), reinterpret_cast<uint8_t*>(&program_header), reinterpret_cast<uint8_t*>(&program_header + 1));
    result.insert(result.end(), reinterpret_cast<uint8_t*>(&null_section_header), reinterpret_cast<uint8_t*>(&null_section_header + 1));
    result.insert(result.end(), reinterpret_cast<uint8_t*>(&text_section_header), reinterpret_cast<uint8_t*>(&text_section_header + 1));
    result.insert(result.end(), reinterpret_cast<uint8_t*>(&shstrtab_section_header), reinterpret_cast<uint8_t*>(&shstrtab_section_header + 1));
    result.insert(result.end(), xmem.rw_ptr(), xmem.rw_ptr() + xmem.size());
    result.insert(result.end(), string_table.begin(), string_table.end());

    return result;
}

void uwin::jit::gdb_jit_register_object(std::vector<uint8_t>& vector) {
    // TODO: thread syncronization
    // TODO: rewrite as RAII class

    jit_code_entry* new_entry = new jit_code_entry();
    new_entry->next_entry = __jit_debug_descriptor.first_entry;
    new_entry->prev_entry = 0;
    new_entry->symfile_addr = reinterpret_cast<const char *>(vector.data());
    new_entry->symfile_size = vector.size();

    if (__jit_debug_descriptor.first_entry != nullptr)
        __jit_debug_descriptor.first_entry->prev_entry = new_entry;
    __jit_debug_descriptor.first_entry = new_entry;
    __jit_debug_descriptor.relevant_entry = new_entry;
    __jit_debug_descriptor.action_flag = JIT_REGISTER_FN;
    __jit_debug_register_code();
}
#endif


static __thread bool dirty = false;
static __thread std::vector<std::tuple<uint32_t, uint32_t, uint64_t>> *dirty_mem;

static __thread ZydisDecoder dec;
static __thread ZydisFormatter fmt;
static __thread ZydisDecodedInstruction instr;
static __thread uint32_t actual_used_flags;

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
#define F_PF static_cast<int>(uwin::jit::cpu_flags::PF)

const uint32_t flag_used_mask = F_SF | F_ZF | F_CF | F_OF| F_PF;
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

    if (guest_address == 0x0001081521) {
        uw_log("bonk %08x\n", (unsigned)ctx->EBX());
    }

    thread_cpu.reg32[HALFIX_EAX] = ctx->EAX();
    thread_cpu.reg32[HALFIX_EBX] = ctx->EBX();
    thread_cpu.reg32[HALFIX_ECX] = ctx->ECX();
    thread_cpu.reg32[HALFIX_EDX] = ctx->EDX();
    thread_cpu.reg32[HALFIX_ESP] = ctx->ESP();
    thread_cpu.reg32[HALFIX_EBP] = ctx->EBP();
    thread_cpu.reg32[HALFIX_ESI] = ctx->ESI();
    thread_cpu.reg32[HALFIX_EDI] = ctx->EDI();
    cpu_set_eflags((cpu_get_eflags() & flag_upd_mask) | ctx->compute_eflags());

#ifdef UW_JIT_TRACE
    uw_log("EFLAGS = %x\n", ctx->compute_eflags());
#endif

    SET_VIRT_EIP(guest_address);

    // while we are in this basic block
    bool writes_eip = false;
    do {
        ZydisDecoderDecodeBuffer(&dec, g2hx<void>(VIRT_EIP()), 100, &instr);
        for (int i = 0; i < instr.operand_count; i++) {
            if (instr.operands[i].type == ZYDIS_OPERAND_TYPE_REGISTER)
                if (instr.operands[i].reg.value == ZYDIS_REGISTER_EIP && instr.operands[i].actions & ZYDIS_OPERAND_ACTION_MASK_WRITE)
                    writes_eip = true;
        }
        cpu_run(1);
    } while (VIRT_EIP() >= guest_address && VIRT_EIP() < guest_address + size && !writes_eip);

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

    actual_used_flags = flag_used_mask;

    for (int i = 0; i < size;) {
        ZydisDecoderDecodeBuffer(&dec, g2hx<void>(guest_address + i), 100, &instr);
        i += instr.length;

        if (instr.mnemonic == ZYDIS_MNEMONIC_IMUL)
            actual_used_flags = F_OF | F_CF;

        if (instr.mnemonic == ZYDIS_MNEMONIC_SHL || instr.mnemonic == ZYDIS_MNEMONIC_SHR ||
            instr.mnemonic == ZYDIS_MNEMONIC_SAR || instr.mnemonic == ZYDIS_MNEMONIC_ROL
            || instr.mnemonic == ZYDIS_MNEMONIC_ROR || instr.mnemonic == ZYDIS_MNEMONIC_SHRD ||
            instr.mnemonic == ZYDIS_MNEMONIC_SHLD) {

            ZydisDecodedOperand op;
            if (instr.mnemonic == ZYDIS_MNEMONIC_SHL || instr.mnemonic == ZYDIS_MNEMONIC_SHR ||
                instr.mnemonic == ZYDIS_MNEMONIC_SAR || instr.mnemonic == ZYDIS_MNEMONIC_ROL
                || instr.mnemonic == ZYDIS_MNEMONIC_ROR) {
                op = instr.operands[1];
            } else {
                op = instr.operands[2];
            }

            int amnt;
            if (op.type == ZYDIS_OPERAND_TYPE_IMMEDIATE) {
                amnt = op.imm.value.u & 0x1f;
            } else {
                assert(op.type == ZYDIS_OPERAND_TYPE_REGISTER && op.reg.value == ZYDIS_REGISTER_CL);
                amnt = ctx->ECX() & 0x1f;
            }
            if (amnt != 0 && amnt != 1)
                actual_used_flags &= ~F_OF; // OF is undefined for shift amounts different from one and zero
        }

        if (instr.mnemonic == ZYDIS_MNEMONIC_BSF)
            actual_used_flags = F_ZF; // other are undefined
    }
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
                     ctx->EIP(), VIRT_EIP());
        if ((cpu_get_eflags() & actual_used_flags) != (ctx->compute_eflags() & actual_used_flags))
            mismatch(xmem, guest_address, "EFLAGS",
                    ctx->compute_eflags() & flag_used_mask, cpu_get_eflags() & flag_used_mask);
    }
}

#undef TST

#endif



#ifdef UW_JIT_GENERATE_PERF_MAP

namespace uwin {
    namespace jit {
        perf_map_writer perf_map_writer_singleton = perf_map_writer();
    }
}

#endif

