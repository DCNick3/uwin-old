
#include "uwin/uwin.h"

#include "uwin-jit/cpu_context.h"
#include "uwin-jit/basic_block_cache.h"

#include <cassert>

#include <iostream>

using namespace uwin::jit;


void* uw_cpu_alloc_context()
{
    return new cpu_context();
}
void uw_cpu_free_context(void* ctx)
{
    delete (cpu_context*)ctx;
}

void uw_cpu_panic(const char* message)
{
    assert("not implemented" == 0);
}


static uint32_t setup_teb(uw_target_thread_data_t *params) {
    params->teb_base = uw_target_map_memory_dynamic(uw_host_page_size, UW_PROT_RW);
    uint32_t *teb = g2hx<uint32_t>(params->teb_base);

    teb[UW_TEB_SEH_FRAME]       = 0xffffffff; //Current SEH frame
    teb[UW_TEB_STACK_BOTTOM]    = params->stack_top + params->stack_size;
    teb[UW_TEB_STACK_TOP]       = params->stack_top;
    teb[UW_TEB_PTEB]            = params->teb_base;
    teb[UW_TEB_INITLIST]        = params->process->library_init_list;
    teb[UW_TEB_PID]             = params->process->process_id;
    teb[UW_TEB_TID]             = params->thread_id;
    teb[UW_TEB_CMDLINE]         = params->process->command_line;
    teb[UW_TEB_LAST_ERROR]      = 0;

    // dirty stuff...
    win32_last_error = teb + UW_TEB_LAST_ERROR;

    return params->teb_base;
}

static uint32_t setup_stack(uw_target_thread_data_t *params) {
    // hacky hacks for stack protection =)
    uint32_t stack_guard_1 = uw_target_map_memory_dynamic(uw_host_page_size, UW_PROT_N);
    params->stack_top = uw_target_map_memory_dynamic(params->stack_size, UW_PROT_RW);
    uint32_t stack_guard_2 = uw_target_map_memory_dynamic(uw_host_page_size, UW_PROT_N);

    assert(stack_guard_1 != (uint32_t)-1 && params->stack_top != (uint32_t)-1 && stack_guard_2 != (uint32_t)-1);

    return params->stack_top;
}

void uw_cpu_setup_thread(void* ctx, uw_target_thread_data_t *params)
{
    cpu_context* context = (cpu_context*)ctx;

    context->static_context.guest_base = reinterpret_cast<uintptr_t>(guest_base);

    /*context->CS() = reinterpret_cast<uintptr_t>(guest_base);
    context->DS() = reinterpret_cast<uintptr_t>(guest_base);
    context->SS() = reinterpret_cast<uintptr_t>(guest_base);
    context->ES() = reinterpret_cast<uintptr_t>(guest_base);
    */

    // get one page more to be consistent with halfix memory map
    params->gdt_base = uw_target_map_memory_dynamic(uw_host_page_size, UW_PROT_RW);

    uint32_t teb_base = setup_teb(params);
    context->FS() = reinterpret_cast<uintptr_t>(g2h(teb_base));

    uint32_t tls_base = params->tls_base = uw_target_map_memory_dynamic(uw_host_page_size, UW_PROT_RW);
    context->GS() = reinterpret_cast<uintptr_t>(g2h(tls_base));

    context->EAX() = params->entry_point;
    context->EBX() = params->entry_param;
    context->ESP() = setup_stack(params) + params->stack_size;
    context->EIP() = params->process->init_entry;
}

// Spinlock based on this flag ensures that only one compiled basic block is executed concurrently
// This is required for satisfying x86 atomicity requirements on aarch64
// As homm3 (and most games of the time) are mainly single-threaded, this will not be a problem for them
static std::atomic_flag exec_lock = ATOMIC_FLAG_INIT;

void uw_cpu_loop(void* context)
{
    cpu_context* ctx = (cpu_context*)context;
#pragma clang diagnostic push
#pragma ide diagnostic ignored "EndlessLoop"
    while (true) {
        uint32_t eip = ctx->EIP();

        if (eip == 0x00626a9f) {
            uw_log("bonk\n");
        }

        auto basic_block = get_native_basic_block(ctx->static_context, eip);

        while (exec_lock.test_and_set(std::memory_order_acquire))  // acquire lock
            ; // spin


        basic_block->execute(ctx);

        exec_lock.clear(std::memory_order_release);
    }
#pragma clang diagnostic pop

    assert("not implemented" == 0);
}
