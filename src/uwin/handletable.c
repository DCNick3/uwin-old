/*
 *  uwin layer handle table helper
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

#include <pthread.h>
#include <assert.h>

#include "uwin/uwin.h"
#include "uwin/util/mem.h"
#include "stb_ds.h"

// implementation is based on glib hashtable

typedef struct {
    int type;
    int refcount;
    union {
        struct {
            int type;
            uint32_t value;
        } dummy;
        void* value;
    };
} ht_entry_t;

static void destroy_obj(int type, void* obj) {
    switch(type) {
        case UW_OBJ_DUMMY:
            break;
        case UW_OBJ_SEM:
            uw_sem_destroy((uw_sem_t *) obj);
            break;
        case UW_OBJ_MUT:
            uw_mut_destroy((uw_mut_t *) obj);
            break;
        case UW_OBJ_COND:
            uw_cond_destroy((uw_cond_t *) obj);
            break;
        case UW_OBJ_FILE:
            uw_file_close((uw_file_t*) obj);
            break;
        case UW_OBJ_SURF:
            uw_ui_surf_free((uw_surf_t*) obj);
            break;
            /*
        case UW_OBJ_WAVE:
            uw_ui_wave_close((uw_wave_t*) obj);
            break;*/
        case UW_OBJ_DIR:
            uw_file_closedir((uw_dir_t*) obj);
            break;
        case UW_OBJ_THREAD:
            uw_thread_destroy((uw_thread_t*) obj);
            break;
        default:
            assert("unknown object type" == 0);
            break;
    }
}

static void ht_entry_unref(ht_entry_t* pentry) {
    pentry->refcount--;
    if (pentry->refcount == 0) {
        destroy_obj(pentry->type, pentry->value);
        uw_free(pentry);
    }
}

static pthread_mutex_t mut;
static uint32_t next_handle = TARGET_ADDRESS_SPACE_SIZE; // this will ensure that no user-space pointer will conincide with this. It makes the dummy handle system useless though... TODO: migrate
static struct { uint32_t key; void* value; } *handle_table;

void uw_ht_initialize(void)
{
    pthread_mutex_init(&mut, NULL);
    //handle_table = g_hash_table_new_full(g_direct_hash, g_direct_equal, NULL, ht_entry_free);
}

void uw_ht_finalize(void)
{
    assert("not implemented" == 0);
    //g_hash_table_unref(handle_table);
    pthread_mutex_destroy(&mut);
}

uint32_t uw_ht_put(void* obj, int type)
{
    if (obj == NULL)
        return (uint32_t)-1;

    assert(type != UW_OBJ_DUMMY);
    
    pthread_mutex_lock(&mut);
    
    ht_entry_t* pentry = uw_new(ht_entry_t, 1);
    pentry->type = type;
    pentry->refcount = 1;
    pentry->value = obj;
    
    uint32_t handle = next_handle++;
    uw_log("new handle: %08x\n", handle);
    stbds_hmput(handle_table, handle, pentry);
    
    pthread_mutex_unlock(&mut);
    return handle;
}

void* uw_ht_get(uint32_t handle, int type)
{
    pthread_mutex_lock(&mut);
    
    void* p = stbds_hmget(handle_table, handle);
    assert(p != NULL);
    
    ht_entry_t* pentry = (ht_entry_t*)p;
    assert(pentry->type == type || type == -1);
    
    void* res = pentry->value;
    
    pthread_mutex_unlock(&mut);
    return res;
}

uint32_t uw_ht_put_dummy(uint32_t obj, int type)
{
    pthread_mutex_lock(&mut);
    
    ht_entry_t* pentry = uw_new(ht_entry_t, 1);
    pentry->type = UW_OBJ_DUMMY;
    pentry->refcount = 1;
    pentry->dummy.value = obj;
    pentry->dummy.type = type;
    
    uint32_t handle = next_handle++;
    uw_log("new handle: %08x\n", handle);
    stbds_hmput(handle_table, handle, pentry);
    
    pthread_mutex_unlock(&mut);
    return handle;
}

uint32_t uw_ht_get_dummy(uint32_t handle, int type)
{
    pthread_mutex_lock(&mut);

    void* p = stbds_hmget(handle_table, handle);
    assert(p != NULL);
    
    ht_entry_t* pentry = (ht_entry_t*)p;
    assert(pentry->type == UW_OBJ_DUMMY);
    if (pentry->dummy.type != type) {
        fprintf(stderr, "Dummy handle type mismatch: got %u, expected %u\n", pentry->dummy.type, type);
        abort();
    }
    
    uint32_t res = pentry->dummy.value;
    
    pthread_mutex_unlock(&mut);
    return res;
}

int uw_ht_get_dummytype(uint32_t handle) {
    pthread_mutex_lock(&mut);

    void* p = stbds_hmget(handle_table, handle);
    assert(p != NULL);
    
    ht_entry_t* pentry = (ht_entry_t*)p;
    assert(pentry->type == UW_OBJ_DUMMY);
    int res = pentry->dummy.type;
    
    pthread_mutex_unlock(&mut);
    return res;
}


uint32_t uw_ht_newref(uint32_t handle)
{
    pthread_mutex_lock(&mut);
    
    void* p = stbds_hmget(handle_table, handle);
    assert(p != NULL);
    
    ht_entry_t* pentry = (ht_entry_t*)p;
    pentry->refcount++;
    
    uint32_t new_handle = next_handle++;
    uw_log("new handle: %08x\n", new_handle);
    stbds_hmput(handle_table, new_handle, pentry);
    
    pthread_mutex_unlock(&mut);
    return new_handle;
}

void uw_ht_delref(uint32_t handle, uint32_t* dummy_type, uint32_t* dummy_value)
{
    pthread_mutex_lock(&mut);
    
    void* p = stbds_hmget(handle_table, handle);
    assert(p != NULL);
    
    ht_entry_t* pentry = (ht_entry_t*)p;
    //pentry->refcount--; // done by destructor of handle table entry
    
    if (pentry->refcount == 1 && pentry->type == UW_OBJ_DUMMY) {
        if (dummy_type != NULL)
            *dummy_type = pentry->dummy.type;
        if (dummy_value != NULL)
            *dummy_value = pentry->dummy.value;
    } else {
        if (dummy_type != NULL)
            *dummy_type = 0;
        if (dummy_value != NULL)
            *dummy_value = 0;
    }

    ht_entry_unref(pentry);
    stbds_hmdel(handle_table, handle);
    
    pthread_mutex_unlock(&mut);
}

int uw_ht_gettype(uint32_t handle)
{
    pthread_mutex_lock(&mut);
    
    void* p = stbds_hmget(handle_table, handle);
    assert(p != NULL);
    
    ht_entry_t* pentry = (ht_entry_t*)p;
    
    int r = pentry->type;
    pthread_mutex_unlock(&mut);
    return r;
}
