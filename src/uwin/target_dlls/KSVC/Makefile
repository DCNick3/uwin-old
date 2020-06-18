
NAME=KSVC.DLL
CFILES=ksvc.c printf.c heap/rpmalloc.c libc.c
OTHER_FILES=ksvc.def

include ../common.mak

LOC_CFLAGS+=-fomit-frame-pointer -DBUILDING_KSVC -Wl,--out-implib,libKSVC.a
