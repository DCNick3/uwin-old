#include "halfix/cpu/cpu.h"
#include "halfix/cpu/opcodes.h"
#include <string.h>

static __thread struct decoded_instruction temporary_placeholder = {
    .handler = op_trace_end
};
static uint32_t hash_eip(uint32_t phys)
{
    return phys & (TRACE_INFO_ENTRIES - 1);
}

void cpu_trace_flush(void)
{
    memset(thread_cpu.trace_info, 0, sizeof(struct trace_info) * TRACE_INFO_ENTRIES);
    thread_cpu.trace_cache_usage = 0;
}

struct trace_info* cpu_trace_get_entry(uint32_t phys)
{
    struct trace_info* i = &thread_cpu.trace_info[hash_eip(phys)];
    if (i->phys != phys)
        return NULL;
    return i;
}
struct decoded_instruction* cpu_get_trace(void)
{
    // If we have gone off the page, recalculate physical EIP
    if ((thread_cpu.phys_eip ^ thread_cpu.last_phys_eip) > 4095) {
        uint32_t virt_eip = VIRT_EIP(), lin_eip = virt_eip + thread_cpu.seg_base[CS];
        uint8_t tlb_tag = thread_cpu.tlb_tags[lin_eip >> 12];
        if (TLB_ENTRY_INVALID8(lin_eip, tlb_tag, thread_cpu.tlb_shift_read) || thread_cpu.tlb_attrs[lin_eip >> 12] & TLB_ATTR_NX) {
            if (cpu_mmu_translate(lin_eip, thread_cpu.tlb_shift_read | 8))
                return &temporary_placeholder;
        }
        thread_cpu.phys_eip = PTR_TO_PHYS(thread_cpu.tlb[lin_eip >> 12] + lin_eip);
        thread_cpu.eip_phys_bias = virt_eip - thread_cpu.phys_eip;
        thread_cpu.last_phys_eip = thread_cpu.phys_eip & ~0xFFF;
    }

    // Read the trace entry.
    struct trace_info* trace = &thread_cpu.trace_info[hash_eip(thread_cpu.phys_eip)];
    // If it matches, return the associated trace
    if (trace->phys == thread_cpu.phys_eip && trace->state_hash == thread_cpu.state_hash) {
        if(trace->ptr == NULL) {
            CPU_FATAL("TRACE is NULL (internal CPU bug 1)\n");
        }
        return trace->ptr;
    }

    // Make sure that the trace cache has enough room in it.
    if ((thread_cpu.trace_cache_usage + MAX_TRACE_SIZE) >= TRACE_CACHE_SIZE) {
        // If not, flush the trace cache by clearing all trace info entries
        cpu_trace_flush();
    }

    // Translate the instructions, as needed
    struct decoded_instruction* i = &thread_cpu.trace_cache[thread_cpu.trace_cache_usage];
    thread_cpu.trace_cache_usage += cpu_decode(trace, i);
    if(i == NULL) {
        CPU_FATAL("TRACE is NULL from decode (internal CPU bug) 0\n");
    }
    return i;
}