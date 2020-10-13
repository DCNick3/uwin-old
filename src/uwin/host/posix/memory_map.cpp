/*
 *  memory mapping functions for linux host
 *
 *  Copyright (c) 2020 Nikita Strygin
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, see <http://www.gnu.org/licenses/>.
 */

#include "uwin/uwin.h"
#include "uwin/util/align.h"

#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
 
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <assert.h>

static uint32_t map_base = 0x1000000;
static void* reserved_region_address;
uint32_t uw_host_page_size;

void* uw_memory_map_initialize(void)
{
    uw_host_page_size = sysconf(_SC_PAGESIZE);
  
    reserved_region_address = mmap(NULL, TARGET_ADDRESS_SPACE_SIZE, PROT_NONE, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
    assert(reserved_region_address != MAP_FAILED);
    uw_log("target_memory_map_init() -> %p\n", reserved_region_address);
    return reserved_region_address;
}

void uw_memory_map_finalize(void)
{
    assert(munmap(reserved_region_address, TARGET_ADDRESS_SPACE_SIZE) != -1);
}

static int portable_prot_to_posix(int prot)
{
    switch (prot) {        
        case UW_PROT_R:
            return PROT_READ;
        case UW_PROT_RW:
            return PROT_READ | PROT_WRITE;
        case UW_PROT_RX:
            return PROT_READ | PROT_EXEC;
        case UW_PROT_N:
            return PROT_NONE;
        default:
            abort();
    }
}

static void check_memory_range(uint32_t address, uint32_t size)
{
    assert(UW_IS_ALIGNED(address, uw_host_page_size));
    assert(UW_IS_ALIGNED(size, uw_host_page_size));
    assert(address < TARGET_ADDRESS_SPACE_SIZE && address + size < TARGET_ADDRESS_SPACE_SIZE);
}

int uw_target_map_memory_fixed(uint32_t address, uint32_t size, int prot)
{
    check_memory_range(address, size);
    
    if (address + size > map_base)
        map_base = address + size;
    
    void* host_address = g2h(address);
    int host_prot = portable_prot_to_posix(prot);
    void* res = mmap(host_address, size, host_prot, MAP_PRIVATE | MAP_ANONYMOUS | MAP_FIXED, -1, 0);
    
    uw_log("target_map_memory_fixed(0x%lx, 0x%lx, %d)\n", (unsigned long)address, (unsigned long)size, prot);
    
    assert(res != MAP_FAILED);
    if (res == MAP_FAILED) {
        abort();
        return -1;
    }
    
    return 0;
}

uint32_t uw_target_map_memory_dynamic(uint32_t size, int prot)
{
    assert(UW_IS_ALIGNED(size, uw_host_page_size));
    uint32_t end_address = map_base + size;
    assert(end_address <= TARGET_ADDRESS_SPACE_SIZE);
    if (end_address > TARGET_ADDRESS_SPACE_SIZE)
        return (uint32_t)-1; // out of virtual memory
    
    void* host_address = g2h(map_base);
    int host_prot = portable_prot_to_posix(prot);
    void* res = mmap(host_address, size, host_prot,  MAP_PRIVATE | MAP_ANONYMOUS | MAP_FIXED, -1, 0);
    
    assert(res != MAP_FAILED);
    if (res == MAP_FAILED) {
        abort();
        return -1;
    }
    
    map_base += size;
    
    uint32_t guest_res = h2g(res);
    
    uw_log("target_map_memory_dynamic(0x%lx, %d) -> 0x%lx\n", (unsigned long)size, prot, (unsigned long)guest_res);
    
    return guest_res;
}

int uw_unmap_memory(uint32_t address, uint32_t size)
{
    check_memory_range(address, size);
    
    void* host_address = g2h(address);
    void* res = mmap(host_address, size, PROT_NONE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_FIXED, -1, 0);
    
    uw_log("target_unmap_memory(0x%lx, 0x%lx)\n", (unsigned long)address, (unsigned long)size);
    
    assert(res != MAP_FAILED);
    if (res == MAP_FAILED) {
        abort();
        return -1;
    }
    
    return 0;
}

// TODO: cooperate with jit to detect transitions from writable to executable memory
// Well, homm3 does not use self-modifying code, but emulating it in general should be made possible

int uw_set_memprotect(uint32_t address, uint32_t size, int prot)
{
    check_memory_range(address, size);
    void* host_address = g2h(map_base);
    int host_prot = portable_prot_to_posix(prot);
    int res = mprotect(host_address, size, host_prot);
    assert(res != -1);
    
    uw_log("target_set_memprotect(0x%lx, 0x%lx, %d)\n", (unsigned long)address, (unsigned long)size, prot);
    
    if (res == -1)
        return -1;
    return 0;
}
