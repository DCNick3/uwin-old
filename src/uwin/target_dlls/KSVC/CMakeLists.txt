 
add_dll_target(
    NAME KSVC
    SRCS ksvc.c printf.c heap/rpmalloc.c libc.c
    CFLAGS -fomit-frame-pointer -DBUILDING_KSVC
    DEPS
)
