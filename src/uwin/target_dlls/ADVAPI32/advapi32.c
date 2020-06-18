/*
 *  advapi32.dll implementation
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

#define _ADVAPI32_
#include <ksvc.h>
#include <rpmalloc.h>
#include <printf.h>
#include <windows.h>

static const char homm3_reg_location[] = "SOFTWARE\\New World Computing\\Heroes of Might and Magic\xAE III\\1.0";

LSTATUS WINAPI RegCreateKeyA(HKEY hKey, LPCSTR lpSubKey, PHKEY phkResult) {
    if (hKey == HKEY_LOCAL_MACHINE) {
        if (strcmp(lpSubKey, homm3_reg_location) == 0) {
            *phkResult = (HKEY)kht_dummy_new(KHT_DUMMY_REG_KEY, 0);
            return ERROR_SUCCESS;
        }
    }
    kpanicf("RegCreateKeyA(%p, %s, %p)", hKey, lpSubKey, phkResult);
}

LSTATUS WINAPI RegQueryValueExA(HKEY hKey, LPCSTR lpValueName, LPDWORD lpReserved, LPDWORD lpType, LPBYTE lpData, LPDWORD lpcbData) {
    uint32_t raw_key = kht_dummy_get((khandle_t)hKey, KHT_DUMMY_REG_KEY);
    if (raw_key == 0) {
        char* val = kcfg_read_all(lpValueName);
        if (val == NULL) {
            return ERROR_FILE_NOT_FOUND;
        } else {
            size_t len = strlen(val);
            if (len >= 3) {
                if (val[0] == 'D' && val[1] == 'W' && val[2] == ':') {
                    if (*lpcbData < 4) {
                        *lpcbData = 4;
                        rpfree(val);
                        return ERROR_MORE_DATA;
                    }
                    if (lpType != NULL)
                        *lpType = REG_DWORD;
                    *(DWORD*)lpData = strtol(val + 3, NULL, 16);
                    rpfree(val);
                    return ERROR_SUCCESS;
                } else if (val[0] == 'S' && val[1] == 'Z' && val[2] == ':') {
                    if (*lpcbData < len - 3 + 1) {
                        *lpcbData = len - 3 + 1;
                        rpfree(val);
                        return ERROR_MORE_DATA;
                    }
                    if (lpType != NULL)
                        *lpType = REG_SZ;
                    memcpy(lpData, val + 3, len - 3 + 1);
                    rpfree(val);
                    return ERROR_SUCCESS;
                }
            }
        }
    }
    kpanicf("RegQueryValueExA(%p, %s, %p, %p, %p, %p); raw_key = %x", hKey, lpValueName, lpReserved, lpType, lpData, lpcbData, raw_key);
}

LSTATUS WINAPI RegOpenKeyExA(HKEY hKey, LPCSTR lpSubKey, DWORD ulOptions, REGSAM samDesired, PHKEY phkResult) {
    if (hKey == HKEY_LOCAL_MACHINE) {
        if (strcmp(lpSubKey, homm3_reg_location) == 0) {
            *phkResult = (HKEY)kht_dummy_new(KHT_DUMMY_REG_KEY, 0);
            return ERROR_SUCCESS;
        }
    }
    bad:
    kpanicf("RegOpenKeyExA(%p, %s, %x, %x, %p)", hKey, lpSubKey, ulOptions, samDesired, phkResult);
}

LSTATUS WINAPI RegSetValueExA(HKEY hKey, LPCSTR lpValueName, DWORD Reserved, DWORD dwType, const BYTE *lpData, DWORD cbData) {
    uint32_t raw_key = kht_dummy_get((khandle_t)hKey, KHT_DUMMY_REG_KEY);
    if (raw_key == 0) {
        char* result_value;
        switch (dwType) {
            case REG_DWORD:
                asprintf(&result_value, "DW:%08x", *(DWORD*)lpData);
                break;
            case REG_SZ:
                asprintf(&result_value, "SZ:%s", (char*)lpData);
                break;
            default:
                goto bad;
        }
        kcfg_write(lpValueName, result_value);
        rpfree(result_value);
        return ERROR_SUCCESS;
    }
    bad:
    kpanicf("RegSetValueExA(%p, %s, %p, %x, %p, %x); raw_key = %x", hKey, lpValueName, Reserved, dwType, lpData, cbData, raw_key);
}

LSTATUS WINAPI RegCloseKey(HKEY hKey) {
    uint32_t raw_key = kht_dummy_get((khandle_t)hKey, KHT_DUMMY_REG_KEY);
    if (raw_key == 0) {
        kcfg_commit();
        kht_delref((uint32_t)hKey, NULL, NULL);
        return ERROR_SUCCESS;
    }
    kpanicf("RegCloseKey(%p)", hKey);
}
