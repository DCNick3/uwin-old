 
add_dll_target(
    NAME INIT
    SRCS init.c init.s
    CFLAGS -Wl,--exclude-all-symbols
    LFLAGS "SHELL:--entry _init_entry"
    DEPS KSVC
)
