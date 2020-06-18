/*
 *  gdi32.dll implementation
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

#define _GDI32_

#include <windows.h>
#include <ksvc.h>


INT WINAPI GetDeviceCaps(HDC hdc, INT index) {
    //kprintf("GetDeviceCaps(%p, %d)", hdc, index);
    void* dctx = (void*)kht_dummy_get((khandle_t)hdc, KHT_DUMMY_DEVICE_CONTEXT);
    if (dctx != NULL) {
        kpanicf("GetDeviceCaps(%p, %d); dctx = %p", hdc, index, dctx);
    }
    
    
    switch (index) {
        case BITSPIXEL: return 32;
        case HORZRES: return 800;
        case VERTRES: return 600;
    }
    
    kpanicf("GetDeviceCaps(%p, %d); dctx = %p", hdc, index, dctx);
}

DWORD WINAPI GdiSetBatchLimit(DWORD dw) {
    kprintf("GdiSetBatchLimit(%d)", dw);
    
    DWORD res = (DWORD)get_teb_value(TEB_GDI_BATCH_LIMIT);
    set_teb_value(TEB_GDI_BATCH_LIMIT, (void*)dw);
    return res;
}
