
NAME=KERNEL32.DLL
CFILES=kernel32.c kernel32_stubs.c
OTHER_FILES=kernel32.def
include ../common_dll.mak

LOC_CFLAGS+= --entry _DllMain  -Wl,--out-implib,libKERNEL32.a
