#include "uwin/uwin.h"
#include "uwin/util/mem.h"

// implementation based on posix threads

#include <pthread.h>
#include <semaphore.h>
#include <errno.h>
#include <assert.h>

struct uw_sem {
    sem_t sem;
};

struct uw_cond {
    pthread_cond_t cond;
};

struct uw_mut {
    pthread_mutex_t mut;
};

void uw_sem_initialize(void) {}
void uw_sem_finalize(void) {}
uw_sem_t* uw_sem_alloc(int count) {
    auto* r = (uw_sem_t*)uw_malloc(sizeof(uw_sem_t));
    sem_init(&r->sem, 0, count);
    return r;
}
void uw_sem_destroy(uw_sem_t* sem) {
    sem_destroy(&sem->sem);
    uw_free(sem);
}
void uw_sem_post(uw_sem_t* sem) {
    sem_post(&sem->sem);
}
int32_t uw_sem_wait(uw_sem_t* sem, uint64_t tmout_us) {
    if (tmout_us != 0xffffffffffffffffULL) {
        uint64_t now = uw_get_time_us(),
                then = now + tmout_us;
        struct timespec tv;
        tv.tv_sec = then / 1000000;
        tv.tv_nsec = (then % 1000000) * 1000;
        int r = sem_timedwait(&sem->sem, &tv);
        if (r == 0)
            return 0;
        if (r != ETIMEDOUT) {
            //perror("sem wait error");
            assert(r == ETIMEDOUT);
        }

        //uw_log("sem timeout; tmout = %lx\n", (unsigned long)tmout_us);
        return -1;
    } else {
        sem_wait(&sem->sem);
        return 0;
    }
}

static pthread_mutexattr_t mutexattr;

void uw_mut_initialize(void) {
    pthread_mutexattr_init(&mutexattr);
    pthread_mutexattr_settype(&mutexattr, PTHREAD_MUTEX_RECURSIVE);
}
void uw_mut_finalize(void) {
    pthread_mutexattr_destroy(&mutexattr);
}
uw_mut_t* uw_mut_alloc() {
    uw_mut_t* res = uw_new(uw_mut_t, 1);
    int r = pthread_mutex_init(&res->mut, &mutexattr);
    assert(!r);
    return res;
}
void uw_mut_destroy(uw_mut_t* mut) {
    pthread_mutex_destroy(&mut->mut);
    uw_free(mut);
}
int32_t uw_mut_lock(uw_mut_t* mut, uint64_t tmout_us) {
    if (tmout_us != 0xffffffffffffffffULL) {
        uint64_t now = uw_get_time_us(),
                then = now + tmout_us;
        struct timespec tv;
        tv.tv_sec = then / 1000000;
        tv.tv_nsec = (then % 1000000) * 1000;
        int r = pthread_mutex_timedlock(&mut->mut, &tv);
        if (r == 0)
            return 0;
        if (r != ETIMEDOUT) {
            //perror("mut wait error");
            assert(r == ETIMEDOUT);
        }

        //uw_log("sem timeout; tmout = %lx\n", (unsigned long)tmout_us);
        return -1;
    } else {
        pthread_mutex_lock(&mut->mut);
        return 0;
    }
}
void uw_mut_unlock(uw_mut_t* mut) {
    pthread_mutex_unlock(&mut->mut);
}


void uw_cond_initialize(void) {}
void uw_cond_finalize(void) {}
uw_cond_t* uw_cond_alloc() {
    uw_cond_t* res = uw_new(uw_cond_t, 1);
    int r = pthread_cond_init(&res->cond, NULL);
    assert(!r);
    return res;
}
void uw_cond_destroy(uw_cond_t* cond) {
    pthread_cond_destroy(&cond->cond);
    uw_free(cond);
}
void uw_cond_signal(uw_cond_t* cond) {
    pthread_cond_signal(&cond->cond);
}
void uw_cond_broadcast(uw_cond_t* cond) {
    pthread_cond_broadcast(&cond->cond);
}
int32_t uw_cond_wait(uw_cond_t* cond, uw_mut_t* mut, uint64_t tmout_us) {
    if (tmout_us != 0xffffffffffffffffULL) {
        uint64_t now = uw_get_time_us(),
                then = now + tmout_us;
        struct timespec tv;
        tv.tv_sec = then / 1000000;
        tv.tv_nsec = (then % 1000000) * 1000;
        int r = pthread_cond_timedwait(&cond->cond, &mut->mut, &tv);
        if (r == 0)
            return 0;
        if (r != ETIMEDOUT) {
            //perror("cond wait error");
            assert(r == ETIMEDOUT);
        }

        //uw_log("sem timeout; tmout = %lx\n", (unsigned long)tmout_us);
        return -1;
    } else {
        pthread_cond_wait(&cond->cond, &mut->mut);
        return 0;
    }
}