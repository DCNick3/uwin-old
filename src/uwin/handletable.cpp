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

#include <mutex>
#include <unordered_map>
#include <memory>

struct ht_entry {
    int type;
    union {
        struct {
            int type;
            uint32_t value;
        } dummy;
        void* value;
    };

    ~ht_entry() {
        switch(type) {
            case UW_OBJ_DUMMY:
                break;
            case UW_OBJ_SEM:
                uw_sem_destroy((uw_sem_t *) value);
                break;
            case UW_OBJ_MUT:
                uw_mut_destroy((uw_mut_t *) value);
                break;
            case UW_OBJ_COND:
                uw_cond_destroy((uw_cond_t *) value);
                break;
            case UW_OBJ_FILE:
                uw_file_close((uw_file_t*) value);
                break;
            case UW_OBJ_SURF:
                uw_ui_surf_free((uw_surf_t*) value);
                break;
                /*
            case UW_OBJ_WAVE:
                uw_ui_wave_close((uw_wave_t*) obj);
                break;*/
            case UW_OBJ_DIR:
                uw_file_closedir((uw_dir_t*) value);
                break;
            case UW_OBJ_THREAD:
                uw_thread_destroy((uw_thread_t*) value);
                break;
            default:
                assert("unknown object type" == 0);
                break;
        }
    }
};


static std::mutex mut;
static uint32_t next_handle = TARGET_ADDRESS_SPACE_SIZE; // this will ensure that no user-space pointer will conincide with this. It makes the dummy handle system useless though... TODO: migrate
static std::unordered_map<uint32_t, std::shared_ptr<ht_entry>> handle_table;

void uw_ht_initialize(void)
{
}

void uw_ht_finalize(void)
{
    assert("not implemented" == 0);
}

uint32_t uw_ht_put(void* obj, int type)
{
    if (obj == NULL)
        return (uint32_t)-1;

    assert(type != UW_OBJ_DUMMY);

    std::unique_lock<std::mutex> lk(mut);

    auto pentry = std::make_shared<ht_entry>();
    pentry->type = type;
    pentry->value = obj;
    
    uint32_t handle = next_handle++;
    uw_log("new handle: %08x\n", handle);
    handle_table[handle] = std::move(pentry);

    return handle;
}

void* uw_ht_get(uint32_t handle, int type)
{
    std::unique_lock<std::mutex> lk(mut);

    auto p = handle_table.find(handle);
    assert(p != handle_table.end());

    auto& pentry = p->second;
    assert(pentry->type == type || type == -1);
    
    void* res = pentry->value;

    return res;
}

uint32_t uw_ht_put_dummy(uint32_t obj, int type)
{
    std::unique_lock<std::mutex> lk(mut);
    
    auto pentry = std::make_shared<ht_entry>();
    pentry->type = UW_OBJ_DUMMY;
    pentry->dummy.value = obj;
    pentry->dummy.type = type;
    
    uint32_t handle = next_handle++;
    uw_log("new handle: %08x\n", handle);
    handle_table[handle] = pentry;

    return handle;
}

uint32_t uw_ht_get_dummy(uint32_t handle, int type) {
    std::unique_lock<std::mutex> lk(mut);

    auto p = handle_table.find(handle);
    assert(p != handle_table.end());

    auto& pentry = p->second;
    assert(pentry->type == UW_OBJ_DUMMY);
    if (pentry->dummy.type != type) {
        fprintf(stderr, "Dummy handle type mismatch: got %u, expected %u\n", pentry->dummy.type, type);
        abort();
    }

    return pentry->dummy.value;
}

int uw_ht_get_dummytype(uint32_t handle) {
    std::unique_lock<std::mutex> lk(mut);

    auto p = handle_table.find(handle);
    assert(p != handle_table.end());

    auto& pentry = p->second;
    assert(pentry->type == UW_OBJ_DUMMY);
    return pentry->dummy.type;
}


uint32_t uw_ht_newref(uint32_t handle) {
    std::unique_lock<std::mutex> lk(mut);

    auto p = handle_table.find(handle);
    assert(p != handle_table.end());

    auto& pentry = p->second;

    uint32_t new_handle = next_handle++;
    uw_log("new handle: %08x\n", new_handle);
    handle_table[new_handle] = pentry; // copy

    return new_handle;
}

void uw_ht_delref(uint32_t handle, uint32_t* dummy_type, uint32_t* dummy_value) {
    std::unique_lock<std::mutex> lk(mut);

    auto p = handle_table.find(handle);
    assert(p != handle_table.end());

    auto& pentry = p->second;
    //pentry->refcount--; // done by destructor of handle table entry

    if (pentry.use_count() == 1 && pentry->type == UW_OBJ_DUMMY) {
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

    handle_table.erase(handle);
}

int uw_ht_gettype(uint32_t handle) {
    std::unique_lock<std::mutex> lk(mut);

    auto p = handle_table.find(handle);
    assert(p != handle_table.end());

    auto& pentry = p->second;
    
    return pentry->type;
}
