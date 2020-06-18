
#include "uwin/uwin.h"
#include "uwin/util/mem.h"

#include "stb_ds.h"

#include <assert.h>
#include <semaphore.h>

typedef struct uw_win32_msg_internal uw_win32_msg_internal_t;
struct uw_win32_msg_internal {
    uw_win32_mq_msg_t msg;
    //pthread_cond_t res_signal;
    //pthread_mutex_t res_mutex;
    //volatile int res_state;
    //uint32_t result;
    uw_win32_msg_internal_t *next;
};

#define RES_NOREPLY   0
#define RES_WANTREPLY 1
#define RES_REPLIED   2


struct uw_win32_mq {
    uw_win32_msg_internal_t* first;
    uw_win32_msg_internal_t* last;
    pthread_mutex_t queue_mutex;
    sem_t queue_sem;
    //uw_win32_msg_internal_t* currently_handled_message;
};

static uint32_t* global_message_subscription_list;
static pthread_mutex_t global_message_subscription_list_mut = PTHREAD_MUTEX_INITIALIZER;

#define thread_mq (uw_current_thread_data->win32_mq)

void uw_win32_mq_initialize(void) {
    global_message_subscription_list = NULL;
}

void uw_win32_mq_finalize(void) {
}

void uw_win32_mq_subscribe_global(void) {
    uw_target_thread_data_t* thread_data = uw_current_thread_data;
    uint32_t newref = uw_ht_newref(thread_data->self_handle);
    pthread_mutex_lock(&global_message_subscription_list_mut);
    stbds_arrput(global_message_subscription_list, newref);
    pthread_mutex_unlock(&global_message_subscription_list_mut);
}

void uw_win32_mq_publish_global_message(uw_gmq_msg_t* message) {
    
    uint32_t win32_message, wparam, lparam;
    
    switch (message->message) {
        
        case UW_GM_MOUSEMOVE:
        case UW_GM_MOUSELDOWN:
        case UW_GM_MOUSERDOWN:
        case UW_GM_MOUSELUP:
        case UW_GM_MOUSERUP:
            
            switch (message->message) {
                case UW_GM_MOUSEMOVE:
                    win32_message = WM_MOUSEMOVE;
                    break;
                case UW_GM_MOUSELDOWN:
                    win32_message = WM_LBUTTONDOWN;
                    break;
                case UW_GM_MOUSERDOWN:
                    win32_message = WM_RBUTTONDOWN;
                    break;
                case UW_GM_MOUSELUP:
                    win32_message = WM_LBUTTONUP;
                    break;
                case UW_GM_MOUSERUP:
                    win32_message = WM_RBUTTONUP;
                    break;
                default:
                    assert("not reached" == 0);
            }
            
            wparam = (uint32_t)(uintptr_t)message->param1;
            lparam = (uint32_t)(uintptr_t)message->param2;
            break;
        case UW_GM_THREAD_DIED:
            pthread_mutex_lock(&global_message_subscription_list_mut);
            
            for (int i = 0; i < stbds_arrlen(global_message_subscription_list); i++) {
                uw_thread_t* th = uw_ht_get_thread(global_message_subscription_list[i]);
                if (th == message->param1) {
                    stbds_arrdel(global_message_subscription_list, i);
                    uw_ht_delref(global_message_subscription_list[i], NULL, NULL);
                    i--;
                }
            }
            
            pthread_mutex_unlock(&global_message_subscription_list_mut);
            return;
        default:
            uw_log("ignoring unsupported global message: %x\n", message->message);
            return;
    }
    
    pthread_mutex_lock(&global_message_subscription_list_mut);



    for (int i = 0; i < stbds_arrlen(global_message_subscription_list); i++) {
        uw_thread_t* th = uw_ht_get_thread(global_message_subscription_list[i]);
        uw_win32_mq_post(th, 0, win32_message, wparam, lparam);
    }
    
    pthread_mutex_unlock(&global_message_subscription_list_mut);
}

uw_win32_mq_t* uw_win32_mq_create(void) {
    uw_win32_mq_t* res = uw_new(uw_win32_mq_t, 1);
    res->first = NULL;
    res->last = NULL;
    int r = pthread_mutex_init(&res->queue_mutex, NULL);
    assert(!r);
    r = sem_init(&res->queue_sem, 0, 0);
    assert(!r);
    return res;
}

void uw_win32_mq_free(uw_win32_mq_t* mq) {
    while (mq->first != NULL) {
        uw_win32_msg_internal_t* next = mq->first->next;

        uw_free(mq->first);

        mq->first = next;
    }
    pthread_mutex_destroy(&mq->queue_mutex);
    sem_destroy(&mq->queue_sem);
    uw_free(mq);
}

void uw_win32_mq_post(uw_thread_t* thread, uint32_t hwnd, uint32_t message, uint32_t wparam, uint32_t lparam) {
    uw_win32_mq_t* mq = uw_thread_get_data(thread)->win32_mq;

    uw_win32_msg_internal_t* msg = uw_new0(uw_win32_msg_internal_t, 1);
    msg->msg.hwnd = hwnd;
    msg->msg.message = message;
    msg->msg.wparam = wparam;
    msg->msg.lparam = lparam;
    msg->next = NULL;
    // win32 MSG has time and point. are they needed?

    pthread_mutex_lock(&mq->queue_mutex);
    if (mq->first == NULL) {
        mq->first = mq->last = msg;
    } else {
        mq->last->next = msg;
        mq->last = msg;
    }
    int r = sem_post(&mq->queue_sem);
    assert(!r);
    pthread_mutex_unlock(&mq->queue_mutex);
}

int32_t uw_win32_mq_try_pop(uw_win32_mq_msg_t* out_message) {
    uw_win32_mq_t* mq = thread_mq;
    // we are the only consumer, so the sem value may not decrease without us helping
    int r = sem_trywait(&mq->queue_sem);
    if (r)
        return -1;

    pthread_mutex_lock(&mq->queue_mutex);
    assert(mq->first != NULL && mq->last != NULL);
    uw_win32_msg_internal_t* res;
    if (mq->first == mq->last) {
        res = mq->first;
        mq->first = mq->last = NULL;
    } else {
        res = mq->first;
        mq->first = mq->first->next;
    }
    pthread_mutex_unlock(&mq->queue_mutex);

    memcpy(out_message, &res->msg, sizeof(uw_win32_mq_msg_t));
    uw_free(res);
    return 0;
}

void uw_win32_mq_pop(uw_win32_mq_msg_t* out_message) {
    uw_win32_mq_t* mq = thread_mq;
    // we are the only consumer, so the sem value may not decrease without us helping
    int r = sem_wait(&mq->queue_sem);
    assert(!r);

    pthread_mutex_lock(&mq->queue_mutex);
    assert(mq->first != NULL && mq->last != NULL);
    uw_win32_msg_internal_t* res;
    if (mq->first == mq->last) {
        res = mq->first;
        mq->first = mq->last = NULL;
    } else {
        res = mq->first;
        mq->first = mq->first->next;
    }
    pthread_mutex_unlock(&mq->queue_mutex);

    memcpy(out_message, &res->msg, sizeof(uw_win32_mq_msg_t));
    uw_free(res);
}

