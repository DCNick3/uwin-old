
#include "uwin/uwin.h"

#include "uwin-jit/sljit.h"
#include "uwin-jit/cpu_context.h"
#include "uwin-jit/basic_block_cache.h"

#include <cassert>

#include <iostream>

using namespace uwin::jit;

typedef sljit::sljit_uw (SLJIT_FUNC *block_fun_t)(sljit::sljit_uw);

void* uw_cpu_alloc_context()
{
    return new cpu_context();

    sljit_compiler compiler;

    uint32_t* stuff = new uint32_t[1];

    compiler.emit_enter(0, SLJIT_RET(UW) | SLJIT_ARG1(UW), 6, 6, 0, 0, 0);
    compiler.emit_op2(SLJIT_ADD32,
                      sljit_ref::reg(sljit_reg::R0),
                      sljit_ref::reg(sljit_reg::S0),
                      sljit_ref::imm(1337));
    compiler.emit_op2(SLJIT_ADD32,
                      sljit_ref::reg(sljit_reg::R0),
                      sljit_ref::reg(sljit_reg::R0),
                      sljit_ref::reg(sljit_reg::S1));
    compiler.emit_op1(SLJIT_MOV32,
                    sljit_ref::mem0(stuff),
                    sljit_ref::reg(sljit_reg::R0));
    compiler.emit_op2(SLJIT_SUB32 | SLJIT_SET_Z,
            sljit_ref::unused(),
            sljit_ref::reg(sljit_reg::R0),
            sljit_ref::imm(1460));
    compiler.emit_op1(SLJIT_MOV32,
            sljit_ref::reg(sljit_reg::R0),
            sljit_ref::imm(0));
    compiler.emit_cmov(sljit_flagtype::EQUAL,
            sljit_reg::R0,
            sljit_ref::imm(1));

    compiler.emit_return(SLJIT_MOV,
            sljit_ref::reg(sljit_reg::R0));

    auto code = compiler.generate_code<block_fun_t>();

    code.dump();

    uint32_t res = code.get_pointer()(123);

    std::cout << "Howdy?" << std::endl << "res = " << res << std::endl << "*stuff = " << *stuff << std::endl;

    delete[] stuff;

    assert("not implemented" == 0);
}
void uw_cpu_free_context(void* ctx)
{
    delete (cpu_context*)ctx;
}

void uw_cpu_panic(const char* message)
{
    assert("not implemented" == 0);
}

template<typename T>
static inline T* g2hx(uint32_t addr) {
    return static_cast<T*>(g2h(addr));
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

    context->CS() = reinterpret_cast<uintptr_t>(guest_base);
    context->DS() = reinterpret_cast<uintptr_t>(guest_base);
    context->SS() = reinterpret_cast<uintptr_t>(guest_base);
    context->ES() = reinterpret_cast<uintptr_t>(guest_base);

    uint32_t teb_base = setup_teb(params);
    context->FS() = reinterpret_cast<uintptr_t>(g2h(teb_base));

    uint32_t tls_base = params->tls_base = uw_target_map_memory_dynamic(uw_host_page_size, UW_PROT_RW);
    context->GS() = reinterpret_cast<uintptr_t>(g2h(tls_base));

    context->EAX() = params->entry_point;
    context->EBX() = params->entry_param;
    context->ESP() = setup_stack(params) + params->stack_size;
    context->EIP() = params->process->init_entry;
}

void uw_cpu_loop(void* ctx)
{
    cpu_context* context = (cpu_context*)ctx;
#pragma clang diagnostic push
#pragma ide diagnostic ignored "EndlessLoop"
    while (true) {
        uint32_t eip = context->EIP();
        auto basic_block = get_basic_block(context->static_context, eip);

        basic_block->execute(context);
    }
#pragma clang diagnostic pop

    assert("not implemented" == 0);
}
