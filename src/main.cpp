
#include "uwin/uwin.h"

#include <stdio.h>
#include <stdlib.h>
#include <string>

void* guest_base;


int main(int argc, char** argv)
{
    int interactive = 1;
    std::string exec_path = "";
    for (int i = 1; i < argc; i++) {
        auto str = std::string(argv[i]);
        if (str[0] == '-' && str[1] == '-') {
            auto flag = str.substr(2);
            if (flag == "non-interactive") {
                interactive = 0;
            } else {
                fprintf(stderr, "Unknown flag: %s\n", flag.c_str());
                return -1;
            }
        } else {
            exec_path = str;
        }
    }

    if (exec_path == "") {
        fprintf(stderr, "Please, specify a program to run.\n");
        return -1;
    }


    FILE* f = fopen(exec_path.c_str(), "r");
    if (f == NULL) {
        perror("Cannot open provided program");
        return -1;
    } else
        fclose(f);

    //freopen("stderr.log", "w", stderr);

    uw_target_process_data_t* process_data = new uw_target_process_data_t();
    
    guest_base = uw_memory_map_initialize(); // reserve virtual memory

    uw_cpu_initialize();
    uw_sem_initialize();
    uw_mut_initialize();
    uw_cond_initialize();
    uw_file_initialize();
    uw_ui_initialize(interactive);
    uw_ht_initialize();
    uw_cfg_initialize();
    uw_thread_initialize();
    uw_gmq_initialize();
    uw_win32_mq_initialize();
    //uw_win32_timer_initialize();
    uw_sighandler_initialize();
    
    
    void* initial_thread_param = ldr_load_executable_and_prepare_initial_thread(exec_path.c_str(), process_data);
    ldr_print_memory_map();
    
    uw_start_initial_thread(initial_thread_param);

    while(1) {
        uw_gmq_msg_t msg;
        uw_gmq_poll(&msg);
        //uw_log("gmq message: %02x %p %p\n", msg.message, msg.param1, msg.param2);
        if (msg.message == UW_GM_QUIT)
            break;

        //if (msg.message == UW_GM_FLIP_SURFACE)
        //    uw_ui_surf_flip_primary();

        uw_win32_mq_publish_global_message(&msg);
    }
    
    fprintf(stderr, "Main function returned.\n");

    exit(0);
}
