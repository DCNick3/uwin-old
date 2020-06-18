
CFLAGS := $(CFLAGS)

CFILES=init.c init.s
OTHERFILES=winmm.def
LOC_CFLAGS+=-Wl,--exclude-all-symbols
LOC_LIBS+= -lKSVC
NAME=INIT.DLL

include ../common.mak

LOC_CFLAGS+=--entry _init_entry
