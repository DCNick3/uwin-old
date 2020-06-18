/*
 *  user32.dll implementation
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
// hacky hacks around mingw headers
#define _USER32_

#include <windows.h>
#include <ksvc.h>
#include <printf.h>


//#define HWND_DESKTOP 0
#define HWND_DUMMY_GAME    ((HWND)1)

static HANDLE    game_window_thread_handle;
static DWORD     game_window_thread_id;
static HINSTANCE game_window_instance_handle;
static DWORD     game_window_style;
static int       game_window_created;
static WNDPROC   game_window_proc;
static LPVOID    game_window_param;

static int       last_known_mouse_x = 400;
static int       last_known_mouse_y = 300;

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved) {
    kprintf("user32 init");
    return TRUE;
}

HDC WINAPI GetDC(HWND hWnd) {
    if (hWnd == 0) {
        return (HANDLE)kht_dummy_new(KHT_DUMMY_DEVICE_CONTEXT, 0);
    } else {
        kpanicf("GetDC(%d)", hWnd);
    }
}

INT WINAPI ReleaseDC(HWND hWnd, HDC hDC) {
    if (hWnd == 0) {
        void* dctx = (void*)kht_dummy_get((khandle_t)hDC, KHT_DUMMY_DEVICE_CONTEXT);
        if (dctx == NULL)
            kht_delref((khandle_t)hDC, NULL, NULL);
        return 1;
    }
    kpanicf("ReleaseDC(%p, %p)", hWnd, hDC);
}

HMENU WINAPI LoadMenuA(HINSTANCE hInstance, LPCSTR lpMenuName) {
    char* name;
    if (((uint32_t)lpMenuName) & 0xffff0000) {
        name = strdup(name);
    } else {
        asprintf(&name, "0x%08x", (unsigned)lpMenuName);
    }
    kprintf("LoadMenuA(%p, %s); raw = %p; returning a dummy", hInstance, name, lpMenuName);
    
    return (HMENU)kht_dummy_new(KHT_DUMMY_MENU, (uint32_t)name);
}

HICON WINAPI LoadIconA(HINSTANCE hInstance, LPCSTR lpIconName) {
    char* name;
    if (((uint32_t)lpIconName) & 0xffff0000) {
        name = strdup(name);
    } else {
        asprintf(&name, "0x%08x", (unsigned)lpIconName);
    }
    kprintf("LoadIconA(%p, %s); raw = %p; returning a dummy", hInstance, name, lpIconName);
    
    return (HICON)kht_dummy_new(KHT_DUMMY_ICON, (uint32_t)name);
}

#define ATOM_DUMMY 0x1234

ATOM WINAPI RegisterClassA(const WNDCLASSA *lpWndClass) {
    kprintf("RegisterClassA(%p) {name = %s} dummy stub called", lpWndClass, lpWndClass->lpszClassName);
    game_window_proc = lpWndClass->lpfnWndProc;
    return ATOM_DUMMY;
}


BOOL WINAPI AdjustWindowRect(LPRECT lpRect, DWORD dwStyle, BOOL bMenu) {
    
    if (dwStyle != (WS_VISIBLE | WS_POPUP))
        goto bad;
        
    return TRUE;
bad:
    kpanicf("AdjustWindowRect({%d %d %d %d}, %x, %d)", lpRect->left, lpRect->top, lpRect->right, lpRect->bottom, dwStyle, bMenu);
}

HWND WINAPI CreateWindowExA(DWORD dwExStyle, LPCSTR lpClassName, LPCSTR lpWindowName, DWORD dwStyle, int X, int Y, int nWidth, int nHeight, HWND hWndParent, HMENU hMenu, HINSTANCE hInstance, LPVOID lpParam) {
    kprintf("CreateWindowExA(%x, %s, %s, %x, %d, %d, %d, %d, %x, %x, %x, %x) dummy stub called", dwExStyle, lpClassName, lpWindowName, dwStyle, X, Y, nWidth, nHeight, hWndParent, hMenu, hInstance, lpParam);
    
    if (game_window_created) {
        kprintf("window already created");
        goto bad;
    }
    if (dwExStyle != WS_EX_TOPMOST)
        goto bad;
    if (game_window_proc == NULL) {
        kprintf("bad game_window_proc");
        goto bad;
    }
    if (nWidth != 800 || nHeight != 600) {
        kprintf("bad size");
        goto bad;
    }
    
    CREATESTRUCT create_struct;
    
    create_struct.lpCreateParams = lpParam;
    create_struct.hInstance = hInstance;
    create_struct.hMenu = hMenu;
    create_struct.hwndParent = hWndParent;
    create_struct.cy = nHeight;
    create_struct.cx = nWidth;
    create_struct.y = Y;
    create_struct.x = X;
    create_struct.x = X;
    create_struct.style = dwStyle;
    create_struct.lpszName = lpWindowName;
    create_struct.lpszClass = lpClassName;
    create_struct.dwExStyle = dwExStyle;
    
    game_window_thread_handle = GetCurrentThread();
    game_window_thread_id = GetCurrentThreadId();
    game_window_instance_handle = hInstance;
    game_window_style = dwStyle;
    game_window_param = lpParam;
    
    LRESULT r = SendMessage(HWND_DUMMY_GAME, WM_CREATE, 0, (LPARAM)&create_struct);
    if (r == -1) {
        kprintf("window_proc returned -1 for WM_CREATE. returning null");
        return NULL;
    }
    
    game_window_created = 1;
    
    win32_mq_subscribe_global();
    
    return HWND_DUMMY_GAME;
    
bad:
    kpanic("bad CreateWindowExA");
}

HCURSOR WINAPI LoadCursorA(HINSTANCE hInstance, LPCSTR lpCursorName) {
    char* name;
    if (((uint32_t)lpCursorName) & 0xffff0000) {
        name = strdup(name);
    } else {
        asprintf(&name, "0x%08x", (unsigned)lpCursorName);
    }
    
    return (HCURSOR)kht_dummy_new(KHT_DUMMY_CURSOR, (uint32_t)name); 
bad:
    kpanicf("LoadCursorA(%p, %p)", hInstance, lpCursorName);
}

static HCURSOR current_cursor = NULL;

HCURSOR WINAPI SetCursor(HCURSOR hCursor) {
    kprintf("SetCursor(%p) no-op stub called", hCursor);
    return __atomic_exchange_n(&current_cursor, hCursor, __ATOMIC_SEQ_CST);
}

BOOL WINAPI ClientToScreen(HWND hWnd, LPPOINT lpPoint) {
    //kprintf("ClientToScreen(%p, %p) no-op stub called", hWnd, lpPoint);
    return TRUE;
}

BOOL WINAPI ScreenToClient(HWND hWnd, LPPOINT lpPoint) {
    //kprintf("ScreenToClient(%p, %p) no-op stub called", hWnd, lpPoint);
    return TRUE;
}

static INT display_count = 0;

INT WINAPI ShowCursor(BOOL bShow) {
    if (bShow) {
        return __atomic_add_fetch(&display_count, 1, __ATOMIC_SEQ_CST);
    } else {
        return __atomic_sub_fetch(&display_count, 1, __ATOMIC_SEQ_CST);
    }
}

BOOL WINAPI IsIconic(HWND hWnd) {
    return FALSE;
}

static LONG minLONG(LONG a, LONG b) {
    if (a < b)
        return a;
    else 
        return b;
}

static LONG maxLONG(LONG a, LONG b) {
    if (a > b)
        return a;
    else
        return b;
}

BOOL WINAPI OffsetRect(LPRECT lprc, int dx, int dy) {
    lprc->left += dx;
    lprc->right += dx;
    lprc->top += dy;
    lprc->bottom += dy;
    return TRUE;
}

BOOL WINAPI IsRectEmpty(const RECT *lprc) {
    return lprc->left == lprc->right || lprc->top == lprc->bottom;
}

BOOL WINAPI UnionRect(LPRECT lprcDst, const RECT *lprcSrc1, const RECT *lprcSrc2) {
    lprcDst->left = minLONG(lprcSrc1->left, lprcSrc2->left);
    lprcDst->top = minLONG(lprcSrc1->top, lprcSrc2->top);
    lprcDst->right = maxLONG(lprcSrc1->right, lprcSrc2->right);
    lprcDst->bottom = maxLONG(lprcSrc1->bottom, lprcSrc2->bottom);
    return !IsRectEmpty(lprcDst);
}

BOOL WINAPI IntersectRect(LPRECT lprcDst, const RECT *lprcSrc1, const RECT *lprcSrc2) {
    lprcDst->left = maxLONG(lprcSrc1->left, lprcSrc2->left);
    lprcDst->top = maxLONG(lprcSrc1->top, lprcSrc2->top);
    lprcDst->right = minLONG(lprcSrc1->right, lprcSrc2->right);
    lprcDst->bottom = minLONG(lprcSrc1->bottom, lprcSrc2->bottom);

    if (lprcDst->left >= lprcDst->right || lprcDst->top >= lprcDst->bottom) {
        lprcDst->left = lprcDst->right = lprcDst->top = lprcDst->bottom = 0;
        return FALSE;
    }
    return TRUE;
}

BOOL WINAPI GetCursorPos(LPPOINT lpPoint) {
    lpPoint->x = last_known_mouse_x;
    lpPoint->y = last_known_mouse_y;
    return TRUE;
}

BOOL WINAPI SetMenu(HWND hWnd, HMENU hMenu) {
    kprintf("SetMenu(%p, %p) no-op stub called", hWnd, hMenu);
    return TRUE;
}

BOOL WINAPI DrawMenuBar(HWND hWnd) {
    kprintf("DrawMenuBar(%p) no-op stub called", hWnd);
    return TRUE;
}

HWND WINAPI GetTopWindow(HWND hWnd) {
    if (hWnd == NULL)
        return HWND_DUMMY_GAME;
    kprintf("GetTopWindow(%p) no-op stub called", hWnd);
    return NULL;
}

DWORD WINAPI GetWindowThreadProcessId(HWND hWnd, LPDWORD lpdwProcessId) {
    if (hWnd != HWND_DUMMY_GAME)
        goto bad;
    
    if (lpdwProcessId != NULL)
        *lpdwProcessId = GetCurrentProcessId();
    
    return game_window_thread_id;
    
bad:
    kpanicf("GetWindowThreadProcessId(%p, %p)", hWnd, lpdwProcessId);
}

LONG WINAPI GetWindowLongA(HWND hWnd, INT nIndex) {
    if (hWnd != HWND_DUMMY_GAME)
        goto bad;
    
    switch (nIndex) {
        case GWL_HINSTANCE:
            return (LONG)game_window_instance_handle;
        case GWL_STYLE:
            return (LONG)game_window_style;
        default:
            goto bad;
    }
    
bad:
    kpanicf("GetWindowLongA(%p, %d)", hWnd, nIndex);
}

HWND WINAPI GetWindow(HWND hWnd, UINT uCmd) {
    kprintf("GetWindow(%p, %x) err-stub called", hWnd, uCmd);
    return NULL;
}

static UINT_PTR timer_id = 1;

UINT_PTR WINAPI SetTimer(HWND hWnd, UINT_PTR nIDEvent, UINT uElapse, TIMERPROC lpTimerFunc) {
    if (hWnd != NULL || nIDEvent != 0)
        goto bad;
    
    if (uElapse < USER_TIMER_MINIMUM)
        uElapse = USER_TIMER_MINIMUM;
    if (uElapse > USER_TIMER_MAXIMUM)
        uElapse = USER_TIMER_MAXIMUM;
    
    uint32_t r = win32_timer_create(kthread_get_current_thread(), (uint32_t)hWnd, uElapse, lpTimerFunc);
    
    kprintf("SetTimer(%p, %d, %d, %p) called", hWnd, nIDEvent, uElapse, lpTimerFunc);
    
    return r;
bad:
    kpanicf("SetTimer(%p, %d, %d, %p)", hWnd, nIDEvent, uElapse, lpTimerFunc);
}

INT WINAPI LoadStringA(HINSTANCE hInstance, UINT uID, LPSTR lpBuffer, INT nBufferMax) {
    
    if (hInstance == GetModuleHandle("MSS32.DLL")) {
        kprintf("LoadStringA called on MSS32");
        const char* ret = NULL;
        switch (uID) {
            case 1:
                ret = "5.0e";
                break;
        }
        if (ret != NULL) {
            int l = strlen(ret) + 1;
            if (l > nBufferMax)
                l = nBufferMax;
            memcpy(lpBuffer, ret, l);
            if (l == nBufferMax)
                lpBuffer[l - 1] = '\0';
            return l - 1;
        }
    }
bad:
    kpanicf("LoadStringA(%p, %d, %p, %d)", hInstance, uID, lpBuffer, nBufferMax);
}

BOOL WINAPI PeekMessageA(LPMSG lpMsg, HWND hWnd, UINT wMsgFilterMin, UINT wMsgFilterMax, UINT wRemoveMsg) {
    if (hWnd != NULL)
        goto bad;
    if (wMsgFilterMin != 0 || wMsgFilterMax != 0)
        goto bad;
    if (wRemoveMsg != PM_REMOVE)
        goto bad;
    
    return win32_mq_try_pop((win32_mq_msg_t*)lpMsg) == 0;
    
bad:
    kpanicf("PeekMessageA(%p, %p, %x, %x, %x)", lpMsg, hWnd, wMsgFilterMin, wMsgFilterMax, wRemoveMsg);
}

BOOL WINAPI TranslateMessage(const MSG *lpMsg) {
    if (lpMsg->message == WM_MOUSEMOVE) {
        last_known_mouse_x = lpMsg->lParam & 0xffff;
        last_known_mouse_y = (lpMsg->lParam >> 16) & 0xffff;
    }
    
    return 0;
}

LRESULT WINAPI SendMessage(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam) {
    
    if (GetCurrentThreadId() != game_window_thread_id)
        goto bad;
    
    return game_window_proc(hWnd, Msg, wParam, lParam);
    
bad:
    kpanicf("SendMessage(%p, %x, %x, %x)", hWnd, Msg, wParam, lParam);
}

LRESULT WINAPI DispatchMessageA(const MSG *lpMsg) {
    if (GetCurrentThreadId() != game_window_thread_id)
        kpanic("DispatchMessageA in thread different from window thread");
    
    LRESULT res = game_window_proc(lpMsg->hwnd, lpMsg->message, lpMsg->wParam, lpMsg->lParam);

    //win32_mq_reply(res); // two-way communication is not needed anyways
    
    return res;
}

LRESULT WINAPI DefWindowProc(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam) {
    switch (Msg) {
        case WM_TIMER:
            if (lParam != 0) {
                ((TIMERPROC)lParam)(hWnd, Msg, wParam, GetTickCount());
            }
            break;
        default:
            kpanicf("what should I do in DefWindowProc with %p, %x, %x, %x?", hWnd, Msg, wParam, lParam);
    }
}

INT WINAPI GetMenuItemCount(HMENU hMenu) {
    kprintf("GetMenuItemCount(%p) dumb-stub called", hMenu);
    return 0;
}

SHORT WINAPI GetKeyState(INT nVirtKey) {
    //kprintf("GetKeyState(0x%x) dumb-stub called", nVirtKey);
    return 0;
}

HWND WINAPI SetCapture(HWND hWnd) {
    kprintf("SetCapture(%p) no-op stub called", hWnd);
    return NULL;
}

BOOL WINAPI ReleaseCapture() {
    kprintf("SetCapture() no-op stub called");
    return TRUE;
}

BOOL WINAPI IsWindow(HWND hWnd) {
    if (hWnd != HWND_DUMMY_GAME)
        goto bad;
    
    return TRUE;
    
bad:
    kpanicf("IsWindow(%p)", hWnd);
}
DWORD WINAPI CheckMenuItem(HMENU hMenu, UINT uIDCheckItem, UINT uCheck) {
    kprintf("CheckMenuItem(%p, %d, %x) executed as no-op", hMenu, uIDCheckItem, uCheck);
    return MF_UNCHECKED;
}

BOOL WINAPI DestroyMenu(HMENU hMenu) {
    kprintf("DestroyMenu(%p)", hMenu);
    kht_delref((khandle_t)hMenu, NULL, NULL);
    return TRUE;
}
