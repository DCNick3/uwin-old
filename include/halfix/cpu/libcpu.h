#ifndef LIBCPU_H
#define LIBCPU_H

#ifdef __cplusplus
extern "C" {
#endif

// Standalone header that can be included to control the Halfix CPU emulator
#include <stdint.h>

typedef void (*abort_handler)(void);

typedef void *(*mem_refill_handler)(uint32_t address, int write);


// Register onabort handler
void cpu_register_onabort(abort_handler h);

// Register a handler that will be invoked when the CPU acknowledges an IRQ from the PIC/APIC
void cpu_register_pic_ack(abort_handler h);

// Register a handler that will be invoked when the FPU wishes to send IRQ13 to the CPU.
void cpu_register_fpu_irq(abort_handler h);

// Register a handler to provide the emulator with pages of physical memory
void cpu_register_mem_refill_handler(mem_refill_handler h);

// Register a handler to provide the emulator with pages of linear memory (useful for simulation of mmap in user mode)
void cpu_register_lin_refill_handler(mem_refill_handler h);

// Enable/disable APIC. Note that you will still have to provide the implementation; all it does is report that the CPU has an APIC in CPUID.
void cpu_enable_apic(int enabled);

// Raise the CPU IRQ line and put the following IRQ on the bus
void cpu_raise_irq_line(int irq);

// Lower the CPU IRQ line
void cpu_lower_irq_line(void);

// Run the CPU
int cpu_core_run(int cycles);

// Get a pointer to a bit of CPU state
void *cpu_get_state_ptr(int id);

// Get CPU state
uint32_t cpu_get_state(int id);

// Set a bit of CPU state. Returns a non-zero value if an exception occurred.
int cpu_set_state(int id, uint32_t data);


// Shortcut to initialize the CPU to 32-bit protected
void cpu_init_32bit(void);

enum {
    // Registers
    CPUPTR_GPR,
    CPUPTR_XMM,
    CPUPTR_MXCSR,

    // EFLAGS, EIP are not here since they must be accessed specially
    // CPL is not here since state_hash must be updated

    // Segment descriptor caches. These should read-only, but if you're daring enough then you can write them
    // If you want to set the registers, than run cpu_set_seg()
    CPUPTR_SEG_DESC,
    CPUPTR_SEG_LIMIT,
    CPUPTR_SEG_BASE,
    CPUPTR_SEG_ACCESS,

    // Memory type range registers (if you ever need them)
    CPUPTR_MTRR_FIXED,
    CPUPTR_MTRR_VARIABLE,
    CPUPTR_MTRR_DEFTYPE,
    CPUPTR_PAT,

    // APIC base MSR
    CPUPTR_APIC_BASE,
    CPUPTR_SYSENTER_INFO
};

enum {
    CPU_EFLAGS,
    CPU_EIP,
    CPU_LINEIP,
    CPU_PHYSEIP,
    CPU_CPL,
    CPU_SEG,
    CPU_STATE_HASH,

#define MAKE_CR_ID(id) (id << 4) | (CPU_CR)
    CPU_CR,

// Use this macro to pass a descriptor to cpu_set_state
#define MAKE_SEG_ID(id) (id << 4) | (CPU_SEG)
    CPU_A20
};


#ifdef __cplusplus
};
#endif

#endif
