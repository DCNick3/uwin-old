
#include "uwin/uwin.h"
#include "uwin/util/mem.h"

#include <stdio.h>
#include <stdlib.h>

void* guest_base;


int main(int argc, char** argv)
{
    if (argc < 2) {
        fprintf(stderr, "Please, specify a program to run.\n");
        return -1;
    }
    
    const char *exec_path = argv[1];

    
    uw_target_process_data_t* process_data = uw_new(uw_target_process_data_t, 1);
    
    guest_base = uw_memory_map_initialize(); // reserve virtual memory

    uw_sem_initialize();
    uw_mut_initialize();
    uw_cond_initialize();
    uw_file_initialize();
    uw_ui_initialize();
    uw_ht_initialize();
    uw_cfg_initialize();
    uw_thread_initialize();
    uw_gmq_initialize();
    uw_win32_mq_initialize();
    //uw_win32_timer_initialize();
    uw_sighandler_initialize();
    
    
    void* initial_thread_param = ldr_load_executable_and_prepare_initial_thread(exec_path, process_data);
    ldr_print_memory_map();
    
    uw_start_initial_thread(initial_thread_param);

    while(1) {
        uw_gmq_msg_t msg;
        uw_gmq_poll(&msg);
        //uw_log("gmq message: %02x %p %p\n", msg.message, msg.param1, msg.param2);
        if (msg.message == UW_GM_QUIT)
            break;
        uw_win32_mq_publish_global_message(&msg);
    }
    
    fprintf(stderr, "Main function returned.\n");
    return 0;
    
    return 0;
}
