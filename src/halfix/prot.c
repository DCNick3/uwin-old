// Protected mode helper functions
#include "halfix/cpu/cpu.h"

int cpu_prot_set_cr(int cr, uint32_t v)
{
    uint32_t diffxor = v ^thread_cpu.cr[cr];
    thread_cpu.cr[cr] = v;
    // TODO: #GP on invalid bits
    switch (cr) {
    case 0:
        if (diffxor & (CR0_PG | CR0_PE | CR0_WP))
            cpu_mmu_tlb_flush();
        break;
    case 3: // PDBR
        thread_cpu.cr[3] &= ~31;
        if(thread_cpu.cr[4] & CR4_PGE)
            cpu_mmu_tlb_flush_nonglobal();
        else
            cpu_mmu_tlb_flush();
        break;
    case 4:
        if (diffxor & (CR4_PGE | CR4_PAE | CR4_PSE | CR4_PCIDE | CR4_SMEP))
            cpu_mmu_tlb_flush();
    }
    return 0;
}

void cpu_prot_set_dr(int id, uint32_t val)
{
    // Setting the debug registers does nothing.
    switch (id) {
    case 0 ... 3:
        thread_cpu.dr[id] = val;
        cpu_mmu_tlb_invalidate(val);
        break;
    case 6:
        thread_cpu.dr[6] = (thread_cpu.dr[6] & 0xffff0ff0) | (val & 0xE00F);
        break;
    case 7:
        thread_cpu.dr[7] = (val & 0xffff2fff) | 0x400;
        cpu_mmu_tlb_flush();
        break;
    default:
        thread_cpu.dr[id] = val;
        break;
    }
}

// Update a few constants
void cpu_prot_update_cpl(void)
{
    if (thread_cpu.cpl == 3) {
        thread_cpu.tlb_shift_read = TLB_USER_READ;
        thread_cpu.tlb_shift_write = TLB_USER_WRITE;
    } else {
        thread_cpu.tlb_shift_read = TLB_SYSTEM_READ;
        thread_cpu.tlb_shift_write = TLB_SYSTEM_WRITE;
    }
}