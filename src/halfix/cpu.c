// Main CPU emulator entry point

#include "halfix/cpu/cpu.h"
#include "halfix/cpu/fpu.h"
#include "halfix/cpu/instrument.h"
#include "halfix/cpuapi.h"
#include "halfix/state.h"
#include <string.h>

__thread struct cpu *thread_cpu_ptr;

void cpu_set_a20(int a20_enabled)
{
    uint32_t old_a20_mask = thread_cpu.a20_mask;

    thread_cpu.a20_mask = -1 ^ (!a20_enabled << 20);
    if (old_a20_mask != thread_cpu.a20_mask)
        cpu_mmu_tlb_flush(); // Only clear TLB if A20 gate has changed

#ifdef INSTRUMENT
    cpu_instrument_set_a20(a20_enabled);
#endif
}

int cpu_init_mem()
{
    //halfix.mem = calloc(1, size);
    //memset(halfix.mem + 0xC0000, -1, 0x40000);
    //halfix.memory_size = size;

    thread_cpu.smc_has_code_length = (TARGET_ADDRESS_SPACE_SIZE + 4095) >> 12;
    thread_cpu.smc_has_code = calloc(4, thread_cpu.smc_has_code_length);

// It's possible that instrumentation callbacks will need a physical pointer to RAM
#ifdef INSTRUMENT
    cpu_instrument_init_mem();
#endif
    return 0;
}
int cpu_interrupts_masked(void)
{
    return thread_cpu.eflags & EFLAGS_IF;
}

itick_t cpu_get_cycles(void)
{
    return thread_cpu.cycles + (thread_cpu.cycle_offset - thread_cpu.cycles_to_run);
}

// Execute main CPU interpreter
int cpu_run(int cycles)
{
    // Reset state
    thread_cpu.cycle_offset = cycles;
    thread_cpu.cycles_to_run = cycles;
    thread_cpu.refill_counter = 0;
    thread_cpu.hlt_counter = 0;

    uint64_t begin = cpu_get_cycles();

    while (1) {
        // Check for interrupts
        if (thread_cpu.intr_line_state) {
            // Check for validity
            if (thread_cpu.eflags & EFLAGS_IF && !thread_cpu.interrupts_blocked) {
              /*
                int interrupt_id = pic_get_interrupt();
                cpu_interrupt(interrupt_id, 0, INTERRUPT_TYPE_HARDWARE, VIRT_EIP());
#ifdef INSTRUMENT
                cpu_instrument_hardware_interrupt(interrupt_id);
#endif
                halfix.exit_reason = EXIT_STATUS_NORMAL;*/
            }
        }

        // Don't continue executing if we are in a hlt state
        if (thread_cpu.exit_reason == EXIT_STATUS_HLT)
            return 0;

        if (thread_cpu.interrupts_blocked) {
            // Run one instruction
            thread_cpu.refill_counter = cycles;
            thread_cpu.cycles += cpu_get_cycles() - thread_cpu.cycles;
            thread_cpu.cycles_to_run = 1;
            thread_cpu.cycle_offset = 1;
            thread_cpu.interrupts_blocked = 0;
        }

        // Reset state as needed
        cpu_execute();

        // Move cycles forward
        thread_cpu.cycles += cpu_get_cycles() - thread_cpu.cycles;

        thread_cpu.cycles_to_run = thread_cpu.refill_counter;
        thread_cpu.refill_counter = 0;
        thread_cpu.cycle_offset = thread_cpu.cycles_to_run;

        if (!thread_cpu.cycles_to_run)
            break;
    }

    // We are here for the following three reasons:
    //  - HLT raised
    //  - A device requested a fast return
    //  - We have run "cycles" operations.
    // In the case of the former, halfix.hlt_counter will contain the number of cycles still in halfix.cycles_to_run
    int cycles_run = cpu_get_cycles() - begin;
    thread_cpu.cycle_offset = 0;
    return cycles_run;
}

void cpu_raise_intr_line(void)
{
    thread_cpu.intr_line_state = 1;
#ifdef INSTRUMENT
    cpu_instrument_set_intr_line(1, 0);
#endif
}
void cpu_lower_intr_line(void)
{
    thread_cpu.intr_line_state = 0;
#ifdef INSTRUMENT
    cpu_instrument_set_intr_line(0, 0);
#endif
}
void cpu_request_fast_return(int reason)
{
    UNUSED(reason);
    INTERNAL_CPU_LOOP_EXIT();
}
void cpu_cancel_execution_cycle(int reason)
{
    // We want to exit out of the loop entirely
    thread_cpu.exit_reason = reason;
    thread_cpu.cycles += cpu_get_cycles() - thread_cpu.cycles;
    thread_cpu.cycles_to_run = 1;
    thread_cpu.cycle_offset = 1;
    thread_cpu.refill_counter = 0;
}

// Sets CPUID information. Currently unimplemented
int cpu_set_cpuid(struct cpu_config* x)
{
    UNUSED(x);
    return 0;
}

int cpu_get_exit_reason(void)
{
    return thread_cpu.exit_reason;
}

// Tells the CPU to stop execution after it's been running a little bit. Does nothing in this case.
void cpu_set_break(void) {}

// Resets CPU
void cpu_reset(void)
{
    for (int i = 0; i < 8; i++) {
        // Clear general purpose registers
        thread_cpu.reg32[i] = 0;

        // Set control registers
        if (i == 0)
            thread_cpu.cr[0] = 0x60000010;
        else
            thread_cpu.cr[i] = 0;

        // Set segment registers
        if (i == CS)
            cpu_seg_load_real(CS, 0xF000);
        else
            cpu_seg_load_real(i, 0);

        if (i >= 6)
            thread_cpu.dr[i] = (i == 6) ? 0xFFFF0FF0 : 0x400;
        else
            thread_cpu.dr[i] = 0;
    }
    // Set EIP
    SET_VIRT_EIP(0xFFF0);

    // set CPL
    thread_cpu.cpl = 0;
    cpu_prot_update_cpl();

    // Clear EFLAGS except for reserved bits
    thread_cpu.eflags = 2;

    thread_cpu.page_attribute_tables = 0x0007040600070406LL;

    // Reset APIC MSR, if APIC is enabled
    //if (apic_is_enabled())
    //    halfix.apic_base = 0xFEE00900; // We are BSP
    //else
        thread_cpu.apic_base = 0;

    thread_cpu.mxcsr = 0x1F80;
    cpu_update_mxcsr();

    // Reset TLB
    memset(thread_cpu.tlb, 0, sizeof(void*) * (1 << 20));
    memset(thread_cpu.tlb_tags, 0xFF, 1 << 20);
    memset(thread_cpu.tlb_attrs, 0xFF, 1 << 20);
    cpu_mmu_tlb_flush();
}

int cpu_apic_connected(void)
{
    return 0; //apic_is_enabled() && (halfix.apic_base & 0x100);
}

static void cpu_state(void)
{
}

// Initializes CPU
int cpu_init(void)
{
    state_register(cpu_state);
    cpu_reset();
    fpu_init();
#ifdef INSTRUMENT
    cpu_instrument_init();
#endif
    return 0;
}

void cpu_init_dma(uint32_t page)
{
    cpu_smc_invalidate_page(page);
}

void cpu_write_mem(uint32_t addr, void* data, uint32_t length)
{
    if (length <= 4) {
        switch (length) {
        case 1:
            // TODO: atomics
            *(uint8_t*)g2h(addr) = *(uint8_t*)data;
#ifdef INSTRUMENT
            cpu_instrument_dma(addr, data, 1);
#endif
            return;
        case 2:
            *(uint16_t*)g2h(addr) = *(uint16_t*)data;
#ifdef INSTRUMENT
            cpu_instrument_dma(addr, data, 2);
#endif
            return;
        case 4:
            *(uint32_t*)g2h(addr) = *(uint32_t*)data;
#ifdef INSTRUMENT
            cpu_instrument_dma(addr, data, 4);
#endif
            return;
        }
    }
    abort();
    //memcpy(halfix.mem + addr, data, length);
#ifdef INSTRUMENT
    cpu_instrument_dma(addr, data, length);
#endif
}

void cpu_debug(void)
{
    fprintf(stderr, "EAX: %08x ECX: %08x EDX: %08x EBX: %08x\n", thread_cpu.reg32[EAX], thread_cpu.reg32[ECX], thread_cpu.reg32[EDX], thread_cpu.reg32[EBX]);
    fprintf(stderr, "ESP: %08x EBP: %08x ESI: %08x EDI: %08x\n", thread_cpu.reg32[ESP], thread_cpu.reg32[EBP], thread_cpu.reg32[ESI], thread_cpu.reg32[EDI]);
    fprintf(stderr, "EFLAGS: %08x\n", cpu_get_eflags());
    fprintf(stderr, "CS:EIP: %04x:%08x (lin: %08x) Physical EIP: %08x\n", thread_cpu.seg[CS], VIRT_EIP(), LIN_EIP(), thread_cpu.phys_eip);
    fprintf(stderr, "Translation mode: %d-bit\n", thread_cpu.state_hash ? 16 : 32);
    fprintf(stderr, "guest_base: %p Cycles to run: %d Cycles executed: %d\n", guest_base, thread_cpu.cycles_to_run, (uint32_t)cpu_get_cycles());
}
