
#include "uwin/uwin.h"

#include <assert.h>

void* uw_cpu_alloc_context()
{
    assert("not implemented" == 0);
}
void uw_cpu_free_context(void* cpu_context)
{
    assert("not implemented" == 0);
}
void uw_cpu_panic(const char* message)
{
    assert("not implemented" == 0);
}
void uw_cpu_setup_thread(void* cpu_context, uw_target_thread_data_t *params)
{
    assert("not implemented" == 0);
}
void uw_cpu_loop(void* cpu_context)
{
    assert("not implemented" == 0);
}
