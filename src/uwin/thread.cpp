
#include <assert.h>
#include <atomic>

#include "uwin/uwin.h"
#include "uwin/util/mem.h"

// backed by posix threads

struct uw_thread {
    uw_target_thread_data_t thread_data;
    void *cpu_context;
    pthread_t pthread;
    
    pthread_cond_t termination_cond;
    pthread_mutex_t termination_mutex;
};

typedef struct {
    uw_thread_t* thread;
    uint32_t thread_id;
    uw_target_process_data_t* process_data;
    uint32_t entry_point;
    uint32_t entry_param;
    uint32_t stack_size;
    int suspended;
    volatile int init_done;
    pthread_cond_t init_cond;
    pthread_mutex_t init_mutex;
} uw_thread_init_param_t;

__thread uw_target_thread_data_t* uw_current_thread_data;

//extern int gdbstub_port;

static void *host_thread_entry(void* param) {
    auto init_param = (uw_thread_init_param_t*)param;
    uint32_t thread_id = init_param->thread_id;
    uw_thread_t *thread = init_param->thread;
    
    uw_log("creating a new thread...\n");

    
    thread->cpu_context = uw_cpu_alloc_context(); // uw_new0(struct cpu, 1);
    int r = pthread_cond_init(&thread->termination_cond, NULL);
    assert(!r);
    r = pthread_mutex_init(&thread->termination_mutex, NULL);
    assert(!r);

    
    // set global thread-local variable
    uw_current_thread_data = &thread->thread_data;

    thread->thread_data.running = 1;
    thread->thread_data.process = init_param->process_data;
    thread->thread_data.entry_point = init_param->entry_point;
    thread->thread_data.entry_param = init_param->entry_param;
    thread->thread_data.stack_top = 0;
    thread->thread_data.stack_size = init_param->stack_size;
    thread->thread_data.thread_id = init_param->thread_id;
    thread->thread_data.thread_obj = thread;
    thread->thread_data.tls_base = 0;
    thread->thread_data.gdt_base = 0;
    thread->thread_data.teb_base = 0;
    thread->thread_data.suspend_request_count = init_param->suspended ? 1 : 0;
    thread->thread_data.suspended = 0;
    thread->thread_data.win32_mq = uw_win32_mq_create();
    r = pthread_cond_init(&thread->thread_data.suspend_cond, NULL);
    assert(!r);
    r = pthread_mutex_init(&thread->thread_data.suspend_mutex, NULL);
    assert(!r);


    uw_cpu_setup_thread(thread->cpu_context, &thread->thread_data);
    
    uw_gmq_post(UW_GM_NEWTHREAD, thread, &thread->thread_data);
    
    pthread_mutex_lock(&init_param->init_mutex);
    init_param->init_done = 1;
    pthread_cond_broadcast(&init_param->init_cond);
    pthread_mutex_unlock(&init_param->init_mutex);
    
    /*if (thread_id == UW_INITIAL_THREAD_ID) {
        if (gdbstub_port) {
            FILE* f = fopen("gdb_script.gdb", "w");
            assert(f != NULL);
            
            ldr_write_gdb_setup_script(gdbstub_port, "/home/dcnick3/trash/homm3-switch/qemu-git/windows-user/target_dlls/debug", f);
            fclose(f);
            
            if (gdbserver_start(gdbstub_port) < 0) {
                fprintf(stderr, "qemu: could not open gdbserver on %d\n",
                        gdbstub_port);
                exit(EXIT_FAILURE);
            }
        }
        
        gdb_handlesig(thread->halfix, 0);
    }*/
    
    uw_log("thread state initialized, entering cpu loop\n");

    uw_cpu_loop(thread->cpu_context);
    
    assert("not reached" == 0);
}


void uw_thread_initialize(void) {
}

void uw_thread_finalize(void) {
}

static std::atomic<uint32_t> next_thread_id(UW_INITIAL_THREAD_ID);

static uw_thread_init_param_t* create_thread_param(uw_target_process_data_t *process_data, uint32_t entry, uint32_t entry_param, uint32_t stack_size, uint32_t suspended) {
    uint32_t thread_id = next_thread_id++;
    uw_thread_init_param_t* init_param = uw_new(uw_thread_init_param_t, 1);
    init_param->thread = uw_new0(uw_thread_t, 1);
    init_param->thread_id = thread_id;
    init_param->process_data = process_data;
    init_param->entry_point = entry;
    init_param->entry_param = entry_param;
    init_param->stack_size = stack_size;
    init_param->suspended = suspended != 0;
    
    
    init_param->init_done = 0;
    int r = pthread_cond_init(&init_param->init_cond, NULL);
    assert(!r);
    r = pthread_mutex_init(&init_param->init_mutex, NULL);
    assert(!r);
    
    return init_param;
}

// unlike other threads this needs to be prepared to run, but can't be run right away, as we need to initialize tcg
// also we don't have a uw_target_process_data_t in thread_cpu yet
void* uw_create_initial_thread(uw_target_process_data_t *process_data, uint32_t entry, uint32_t entry_param, uint32_t stack_size) {
    return create_thread_param(process_data, entry, entry_param, stack_size, 0);
}

void uw_start_initial_thread(void* initial_thread_param) {
    auto* param = (uw_thread_init_param_t*)initial_thread_param;
    
    param->thread->thread_data.self_handle = uw_ht_put(param->thread, UW_OBJ_THREAD);
    
    int r = pthread_create(&param->thread->pthread, NULL, host_thread_entry, 
        param);
    assert(!r);
    
    pthread_mutex_lock(&param->init_mutex);
    while (!param->init_done)
        pthread_cond_wait(&param->init_cond, &param->init_mutex);
    pthread_mutex_unlock(&param->init_mutex);

    uw_free(param);
}

uint32_t uw_thread_create(uint32_t entry, uint32_t entry_param, uint32_t stack_size, uint32_t suspended) {
    uw_target_process_data_t* process_data = uw_current_thread_data->process;
    
    uw_thread_init_param_t* param = create_thread_param(process_data, entry, entry_param, stack_size, suspended);
    uw_thread_t* thread = param->thread;
    thread->thread_data.self_handle = uw_ht_put(thread, UW_OBJ_THREAD);
    
    int r = pthread_create(&param->thread->pthread, NULL, host_thread_entry, param);
    assert(!r);
    
    pthread_mutex_lock(&param->init_mutex);
    while (!param->init_done)
        pthread_cond_wait(&param->init_cond, &param->init_mutex);
    pthread_mutex_unlock(&param->init_mutex);

    uw_free(param);
    
    return uw_ht_newref(thread->thread_data.self_handle);
}

int32_t uw_thread_suspend(uw_thread_t* thread) {
    //pthread_mutex_lock(&thread_table_mutex);
    // don't die on me
    
    //uw_log("uw_thread_suspend(%d)\n", tid);
    
    pthread_mutex_lock(&thread->thread_data.suspend_mutex);
    int32_t r = thread->thread_data.suspend_request_count++;
    pthread_cond_broadcast(&thread->thread_data.suspend_cond);
    pthread_mutex_unlock(&thread->thread_data.suspend_mutex);

    // wait till the thread is suspended
    while (thread->thread_data.running && !thread->thread_data.suspended)
        ;
        
    //pthread_mutex_unlock(&thread_table_mutex);
    
    return r;
}

int32_t uw_thread_resume(uw_thread_t* thread) {
    pthread_mutex_lock(&thread->thread_data.suspend_mutex);
    int32_t r = thread->thread_data.suspend_request_count--;
    pthread_cond_broadcast(&thread->thread_data.suspend_cond);
    pthread_mutex_unlock(&thread->thread_data.suspend_mutex);
    return r;
}

void uw_thread_leave(void) {
    uw_thread_t *thread = uw_current_thread;
    
    // TODO: do the initial cleanup

    // allow other threads to execute
    uw_break_cpu_loop();

    pthread_mutex_lock(&thread->termination_mutex);

    thread->thread_data.running = 0;
    pthread_cond_broadcast(&thread->termination_cond);
    
    pthread_mutex_unlock(&thread->termination_mutex);
    
    uw_gmq_post(UW_GM_NEWTHREAD, thread, &thread->thread_data);
    
    uw_ht_delref(thread->thread_data.self_handle, NULL, NULL);
    
    pthread_exit(NULL);
}

uw_target_thread_data_t* uw_thread_get_data(uw_thread_t* thread) {
    return &thread->thread_data;
}

void uw_thread_wait(uw_thread_t* thread) {
    
    pthread_mutex_lock(&thread->termination_mutex);
    
    while (thread->thread_data.running)
        pthread_cond_wait(&thread->termination_cond, &thread->termination_mutex);
    
    pthread_mutex_unlock(&thread->termination_mutex);
}

void uw_thread_destroy(uw_thread_t* thread) {
    // TODO: cleanup all the stuff
}
