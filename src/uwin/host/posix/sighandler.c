
#include "uwin/uwin.h"

#include <ucontext.h>
#include <signal.h>
#include <assert.h>

// copied over from user-exec.c


// this is for x86_64 only
static void sighandler(int sig, siginfo_t *si, void *unused) {
    ucontext_t *u = (ucontext_t *)unused;

    uw_log("SIGSEGV while accesing %016lx\n", (uint64_t)si->si_addr);
    
    //int is_write = u->uc_mcontext.gregs[REG_TRAPNO] == 0xe ? (u->uc_mcontext.gregs[REG_ERR] >> 1) & 1 : 0;
    
    //handle_cpu_signal(pc, si, is_write, &u->uc_sigmask);
    uw_cpu_panic("Fakku");
}

void uw_sighandler_initialize(void)
{
    struct sigaction sa;
    
    sa.sa_flags = SA_SIGINFO;
    sigemptyset(&sa.sa_mask);
    sa.sa_sigaction = sighandler;
    if (sigaction(SIGSEGV, &sa, NULL) == -1)
        assert("sigaction failed" == 0);
    
}

void uw_sighandler_finalize(void)
{
    
}
