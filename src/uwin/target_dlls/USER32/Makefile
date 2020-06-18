
NAME=USER32.DLL
CFILES=user32_stubs.c user32.c
OTHER_FILES=user32.def
include ../common_dll.mak

LOC_CFLAGS+= --entry _DllMain  -Wl,--out-implib,libUSER32.a
LOC_LIBS+= -lKERNEL32
