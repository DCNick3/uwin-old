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

#include <cassert>

#include "uwin/uwin.h"
#include "uwin/handletable.h"
#include "uwin/kobj/kobj.h"
#include "uwin/kobj/kdummy.h"

#include <mutex>
#include <unordered_map>
#include <memory>

using entry = std::shared_ptr<std::unique_ptr<uwin::kobj>>;

/*
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
                //
            //case UW_OBJ_WAVE:
              //  uw_ui_wave_close((uw_wave_t*) obj);
                //break;
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
*/

static std::mutex mut;
static uint32_t next_handle = TARGET_ADDRESS_SPACE_SIZE; // this will ensure that no user-space pointer will conincide with this.
static std::unordered_map<uint32_t, entry> handle_table;

void uw_ht_initialize()
{
}

void uw_ht_finalize()
{
    assert("not implemented" == nullptr);
}

uint32_t uw_ht_put(std::unique_ptr<uwin::kobj> obj)
{
    assert(obj != nullptr);
    //if (obj == nullptr)
    //    return (uint32_t)-1;

    //assert(!obj->is<uwin::kdummy>());

    std::unique_lock<std::mutex> lk(mut);

    auto pentry = std::make_shared<std::unique_ptr<uwin::kobj>>(std::move(obj));
    
    uint32_t handle = next_handle++;
    uw_log("new handle: %08x\n", handle);
    handle_table[handle] = std::move(pentry);

    return handle;
}

uwin::kobj* uw_ht_get(uint32_t handle)
{
    std::unique_lock<std::mutex> lk(mut);

    auto p = handle_table.find(handle);
    assert(p != handle_table.end());

    auto& pentry = p->second;

    return pentry->get();
}

uint32_t uw_ht_put_dummy(uint32_t obj, int type)
{
    std::unique_lock<std::mutex> lk(mut);

    std::unique_ptr<uwin::kobj> dummy = std::make_unique<uwin::kdummy>(type, obj);
    return uw_ht_put(std::move(dummy));
}

uint32_t uw_ht_get_dummy(uint32_t handle, int type) {
    std::unique_lock<std::mutex> lk(mut);

    auto p = handle_table.find(handle);
    assert(p != handle_table.end());

    auto* ptr = p->second.get()->get();

    auto& dummy = ptr->as<uwin::kdummy>();
    if (dummy.get_type() != type) {
        fprintf(stderr, "Dummy handle type mismatch: got %u, expected %u\n", dummy.get_type(), type);
        abort();
    }

    return dummy.get_data();
}

int uw_ht_get_dummytype(uint32_t handle) {
    std::unique_lock<std::mutex> lk(mut);

    auto p = handle_table.find(handle);
    assert(p != handle_table.end());

    auto& pentry = p->second;
    return pentry->get()->as<uwin::kdummy>().get_type();
}


uint32_t uw_ht_newref(uint32_t handle) {
    std::unique_lock<std::mutex> lk(mut);

    auto p = handle_table.find(handle);
    assert(p != handle_table.end());

    auto& pentry = p->second;

    uint32_t new_handle = next_handle++;
    uw_log("new handle: %08x\n", new_handle);
    handle_table[new_handle] = pentry; // copy the shared pointer

    return new_handle;
}

void uw_ht_delref(uint32_t handle, uint32_t* dummy_type, uint32_t* dummy_value) {
    std::unique_lock<std::mutex> lk(mut);

    auto p = handle_table.find(handle);
    assert(p != handle_table.end());

    auto& pentry = p->second;
    //pentry->refcount--; // done by destructor of handle table entry

    if (dummy_type != nullptr || dummy_value != nullptr) {
        uwin::kdummy *dummy;
        if (pentry.use_count() == 1 && (dummy = pentry->get()->try_as<uwin::kdummy>()) != nullptr) {
            if (dummy_type != nullptr)
                *dummy_type = dummy->get_type();
            if (dummy_value != nullptr)
                *dummy_value = dummy->get_data();
        } else {
            if (dummy_type != nullptr)
                *dummy_type = 0;
            if (dummy_value != nullptr)
                *dummy_value = 0;
        }
    }

    handle_table.erase(handle);
}
